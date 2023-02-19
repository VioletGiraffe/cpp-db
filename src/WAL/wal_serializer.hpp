#pragma once

#include "wal_data_types.hpp"
#include "../dbrecord.hpp"
#include "../dbops.hpp"
#include "../dbschema.hpp"
#include "../storage/storage_io_interface.hpp"
#include "../serialization/dbrecord-serializer.hpp"
#include "../utils/dbutilities.hpp"

#include "tuple/tuple_helpers.hpp"
#include "utility/template_magic.hpp"

#include <array>

namespace WAL {

template <RecordType Record>
class Serializer
{
	using Schema = DbSchema<Record>;

public:
	template <class Operation, class StorageAdapter, sfinae<Operation::op == OpCode::Insert> = true>
	[[nodiscard]] static bool serialize(const Operation& op, StorageIO<StorageAdapter>& io) noexcept;

	template <class Operation, class StorageAdapter, sfinae<Operation::op == OpCode::Find> = true>
	[[nodiscard]] static bool serialize(const Operation& op, StorageIO<StorageAdapter>& io) noexcept;

	template <class Operation, class StorageAdapter, sfinae<Operation::op == OpCode::UpdateFull> = true>
	[[nodiscard]] static bool serialize(const Operation& op, StorageIO<StorageAdapter>& io) noexcept;

	template <class Operation, class StorageAdapter, sfinae<Operation::op == OpCode::AppendToArray> = true>
	[[nodiscard]] static bool serialize(const Operation& op, StorageIO<StorageAdapter>& io) noexcept;

	template <class Operation, class StorageAdapter, sfinae<Operation::op == OpCode::Delete> = true>
	[[nodiscard]] static bool serialize(const Operation& op, StorageIO<StorageAdapter>& io) noexcept;

	template <class MarkerStruct, class StorageAdapter, sfinae<MarkerStruct::markerID != 0> = true>
	[[nodiscard]] static bool serialize(const MarkerStruct& op, StorageIO<StorageAdapter>& io) noexcept;

	template <class StorageAdapter, typename Receiver>
	[[nodiscard]] static bool deserialize(StorageIO<StorageAdapter>& io, Receiver&& receiver) noexcept;
	
	template <class StorageAdapter>
	[[nodiscard]] static bool isOperationCompletionMarker(StorageIO<StorageAdapter>& io) noexcept;

private:
	using RecordSerializer = DbRecordSerializer<Record>;

	template <typename Functor, size_t currentIndex = 0, typename TypesPack = type_pack<Record>, size_t N>
	static constexpr void constructFindOperationType([[maybe_unused]] Functor&& receiver, const std::array<uint8_t, N>& fieldIds, const size_t nFieldIds) noexcept
	{
		// Get the compile-time ID of the current type
		constexpr_for_fold<0, Schema::Fields::type_count>([&]<auto I>() {
			using Type = typename Schema::template FieldByIndex_t<I>;
			if (Type::id == fieldIds[currentIndex])
			{
				using UpdatedTypesPack = typename TypesPack::template Append<Type>;
				if ((currentIndex + 1) == nFieldIds)
				{
					// All done - compose the final type and exit.
					using FindOpType = typename UpdatedTypesPack::template Construct<Operation::Find>;
					receiver.template operator()<FindOpType>();
				}
				else
				{
					if constexpr (currentIndex + 1 < Operation::Find<Record>::maxFieldCount) // Keep recursing
						constructFindOperationType<Functor, currentIndex + 1, UpdatedTypesPack>(std::forward<Functor>(receiver), fieldIds, nFieldIds);
				}
			}
		});
	}
};

template <RecordType Record>
template<class Operation, class StorageAdapter, sfinae<Operation::op == OpCode::Insert>>
bool Serializer<Record>::serialize(const Operation& op, StorageIO<StorageAdapter>& io) noexcept
{
	if (!io.write(Operation::op))
		return false;

	static_assert(std::is_same_v<Record, remove_cv_and_reference_t<decltype(op._record)>>);
	return RecordSerializer::serialize(op._record, io);
}

template<RecordType Record>
template<class MarkerStruct, class StorageAdapter, sfinae<MarkerStruct::markerID != 0>>
inline bool Serializer<Record>::serialize(const MarkerStruct& marker, StorageIO<StorageAdapter>& io) noexcept
{
	if (!io.write(marker.markerID))
		return false;

	if (!io.write(marker.status))
		return false;

	return true;
}

template <RecordType Record>
template<class Operation, class StorageAdapter, sfinae<Operation::op == OpCode::Find>>
bool Serializer<Record>::serialize(const Operation& op, StorageIO<StorageAdapter>& io) noexcept
{
	if (!io.write(Operation::op))
		return false;

	// This op holds a bunch of fields to search on.

	// First, write the # of fields.
	constexpr auto nFields = std::tuple_size_v<decltype(op._fields)>;
	static_assert(nFields <= 255);
	if (!io.write(static_cast<uint8_t>(nFields)))
		return false;

	bool success = true;
	// Now writing IDs of each the field in order.
	constexpr_for_fold<0, nFields>([&]<auto I>() {
		const auto& field = std::get<I>(op._fields);
		using FieldType = remove_cv_and_reference_t<decltype(field)>;
		static_assert(!FieldType::isArray(), "Does this make sense?");
		static_assert(FieldType::id <= 255);

		if (success && !io.write(static_cast<uint8_t>(FieldType::id)))
			success = false;
	});

	if (!success)
		return false;

	// And now the values
	constexpr_for_fold<0, nFields>([&]<auto I>() {
		const auto& field = std::get<I>(op._fields);
		if (success && !io.writeField(field))
			success = false;
	});

	return success;
}

template <RecordType Record>
template<class Operation, class StorageAdapter, sfinae<Operation::op == OpCode::UpdateFull>>
bool Serializer<Record>::serialize(const Operation& op, StorageIO<StorageAdapter>& io) noexcept
{
	if (!io.write(Operation::op))
		return false;

	constexpr auto keyFieldId = Operation::KeyField::id;
	static_assert (keyFieldId >= 0 && keyFieldId <= 255);
	if (!io.write(static_cast<uint8_t>(keyFieldId)))
		return false;

	if (!io.write(op.insertIfNotPresent()))
		return false;

	static_assert(std::is_same_v<Record, remove_cv_and_reference_t<decltype(op.record)>>);
	if (!RecordSerializer::serialize(op.record, io))
		return false;

	return io.write(op.keyValue);
}

template <RecordType Record>
template<class Operation, class StorageAdapter, sfinae<Operation::op == OpCode::AppendToArray>>
bool Serializer<Record>::serialize(const Operation& op, StorageIO<StorageAdapter>& io) noexcept
{
	static_assert(std::is_same_v<typename Operation::RecordType, Record>, "The operation uses incompatible record type!");

	if (!io.write(Operation::op))
		return false;

	constexpr auto keyFieldId = Operation::KeyField::id;
	static_assert (keyFieldId >= 0 && keyFieldId <= 255);
	if (!io.write(static_cast<uint8_t>(keyFieldId)))
		return false;

	constexpr auto arrayFieldId = Operation::ArrayField::id;
	static_assert (arrayFieldId >= 0 && arrayFieldId <= 255);
	if (!io.write(static_cast<uint8_t>(arrayFieldId)))
		return false;

	if (!io.write(op.insertIfNotPresent()))
		return false;

	if (!io.write(op.keyValue))
		return false;

	if constexpr (op.insertIfNotPresent())
		return RecordSerializer::serialize(op.record, io);
	else
		return io.write(op.array);
}

template <RecordType Record>
template<class Operation, class StorageAdapter, sfinae<Operation::op == OpCode::Delete>>
bool Serializer<Record>::serialize(const Operation& op, StorageIO<StorageAdapter>& io) noexcept
{
	static_assert(std::is_same_v<Record, typename Operation::RecordType>, "The operation record type doesn't match Serializer type!!!");

	if (!io.write(Operation::op))
		return false;

	constexpr auto keyFieldId = Operation::KeyField::id;
	static_assert (keyFieldId >= 0 && keyFieldId <= 255);
	if (!io.write(static_cast<uint8_t>(keyFieldId)))
		return false;

	return io.write(op.keyValue);
}

template <RecordType Record>
template <class StorageAdapter, typename Receiver>
bool Serializer<Record>::deserialize(StorageIO<StorageAdapter>& io, Receiver&& receiver) noexcept
{
	static_assert(sizeof(OpCode) == sizeof(OperationCompletedMarker::markerID));
	std::underlying_type_t<OpCode> entryType;
	assert_and_return_r(io.read(entryType), false);

	switch (entryType)
	{
		case (decltype(entryType)) OpCode::Insert:
		{
			using Op = Operation::Insert<Record>;
			Record r;
			assert_and_return_r(RecordSerializer::deserialize(r, io), false);

			receiver(Op{ std::move(r) });
			return true;
		}
		case (decltype(entryType)) OpCode::Find:
		{
			uint8_t nFields = 0;
			assert_and_return_r(io.read(nFields), false);

			static constexpr size_t maxFindOperationFieldCount = Operation::Find<Record>::maxFieldCount;
			assert_and_return_r(nFields > 0 && nFields <= maxFindOperationFieldCount, false);

			std::array<uint8_t, maxFindOperationFieldCount> ids;
			for (size_t i = 0; i < nFields; ++i)
			{
				assert_and_return_r(io.read(ids[i]), false);
			}

			bool success = true;
			constructFindOperationType([&]<class OpType>() {
				typename OpType::TupleOfFields fieldsTuple;
				constexpr_for_fold<0, std::tuple_size_v<typename OpType::TupleOfFields>>([&]<auto I>() {
					auto& field = std::get<I>(fieldsTuple);
					if (success && !io.readField(field))
					{
						assert_unconditional_r("io.readField(field) failed!");
						success = false;
						return;
					}
				});

				if (success)
					receiver(OpType{std::move(fieldsTuple)});

			}, ids, nFields);

			return success;
		}
		case (decltype(entryType)) OpCode::UpdateFull:
		{
			uint8_t keyFieldId;
			assert_and_return_r(io.read(keyFieldId), false);

			bool insertIfNotPresent;
			assert_and_return_r(io.read(insertIfNotPresent), false);

			Record r;
			assert_and_return_r(RecordSerializer::deserialize(r, io), false);

			bool success = false;
			constexpr_for_fold<0, Schema::fieldsCount_v>([&]<auto I>() {
				using KeyField = typename Schema::template FieldByIndex_t<I>;
				if (KeyField::id == keyFieldId)
				{
					typename KeyField::ValueType keyFieldValue;
					assert_and_return_r(io.read(keyFieldValue), );

					if (insertIfNotPresent)
					{
						using Op = Operation::UpdateFull<Record, KeyField, true>;
						receiver(Op{ std::move(r), std::move(keyFieldValue) });
					}
					else
					{
						using Op = Operation::UpdateFull<Record, KeyField, false>;
						receiver(Op{ std::move(r), std::move(keyFieldValue) });
					}
					success = true;
				}
			});

			return success;
		}
		case (decltype(entryType)) OpCode::AppendToArray:
		{
			uint8_t keyFieldId;
			assert_and_return_r(io.read(keyFieldId), false);

			uint8_t arrayFieldId;
			assert_and_return_r(io.read(arrayFieldId), false);

			bool insertIfNotPresent;
			assert_and_return_r(io.read(insertIfNotPresent), false);

			bool success = false;
			// Searching the compile-time value for keyFieldId
			constexpr_for_fold<0, Schema::fieldsCount_v>([&]<auto KeyFieldIndex>() {
				using KeyField = typename Schema::template FieldByIndex_t<KeyFieldIndex>;
				if (KeyField::id == keyFieldId)
				{
					typename KeyField::ValueType keyFieldValue;
					assert_and_return_r(io.read(keyFieldValue), );

					// Searching the compile-time value for arrayFieldId
					constexpr_for_fold<0, Schema::fieldsCount_v>([&]<auto ArrayFieldIndex>() {
						using ArrayField = typename Schema::template FieldByIndex_t<ArrayFieldIndex>;
						if constexpr (ArrayField::isArray())
						{
							if (ArrayField::id == arrayFieldId)
							{
								if (insertIfNotPresent)
								{
									Record r;
									assert_and_return_r(RecordSerializer::deserialize(r, io), );

									using Op = Operation::AppendToArray<Record, KeyField, ArrayField, true>;
									receiver(Op{ std::move(keyFieldValue), std::move(r) });
								}
								else
								{
									typename ArrayField::ValueType array;
									assert_and_return_r(io.read(array), );

									using Op = Operation::AppendToArray<Record, KeyField, ArrayField, false>;
									receiver(Op{ std::move(keyFieldValue), std::move(array) });
								}

								success = true;
							}
						}
					});
				}
			});

			return success;
		}
		case (decltype(entryType)) OpCode::Delete:
		{
			uint8_t keyFieldId;
			assert_and_return_r(io.read(keyFieldId), false);

			bool success = false;
			constexpr_for_fold<0, Schema::fieldsCount_v>([&]<auto I>() {
				using KeyField = typename Schema::template FieldByIndex_t<I>;
				if (KeyField::id == keyFieldId)
				{
					typename KeyField::ValueType keyFieldValue;
					assert_and_return_r(io.read(keyFieldValue), );

					using Op = Operation::Delete<Record, KeyField>;
					receiver(Op{ std::move(keyFieldValue) });
					success = true;
				}
			});

			return success;
		}
		// Markers are currently handled and skipped by the WAL itself
		/*case OperationCompletedMarker::markerID:
		{
			OperationCompletedMarker marker;
			assert_and_return_r(io.read(marker.status), false);

			receiver(std::move(marker));
			return true;
		}*/
	}

	fatalAbort("Uknown entry type encountered in the log: " + std::to_string(entryType));
	return false;
}

template<RecordType Record>
template<class StorageAdapter>
inline bool Serializer<Record>::isOperationCompletionMarker(StorageIO<StorageAdapter>& io) noexcept
{
	const auto pos = io.pos();

	std::remove_const_t<decltype(OperationCompletedMarker::markerID)> marker = 0;
	assert_and_return_r(io.read(marker), false);
	assert_r(io.seek(pos)); // Restoring the original position

	return marker == OperationCompletedMarker::markerID;
}

} // namespace
