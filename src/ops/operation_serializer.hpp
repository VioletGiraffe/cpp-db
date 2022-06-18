#pragma once

#include "../dbrecord.hpp"
#include "../dbops.hpp"
#include "../dbschema.hpp"
#include "../storage/storage_io_interface.hpp"
#include "../serialization/dbrecord-serializer.hpp"

#include "tuple/tuple_helpers.hpp"
#include "utility/template_magic.hpp"

#include <array>

namespace Operation {

template <class Record> // TODO: concept 
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

	template <class StorageAdapter, typename Receiver>
	[[nodiscard]] static bool deserialize(StorageIO<StorageAdapter>& io, Receiver&& receiver) noexcept;

private:
	using RecordSerializer = DbRecordSerializer<Record>;

	template <typename Functor, size_t currentIndex = 0, typename TypesPack = type_pack<>, size_t N>
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
					receiver(type_wrapper<FindOpType>{});
				}
				else
				{
					if constexpr (currentIndex + 1 < Operation::Find<>::maxFieldCount) // Keep recursing
						constructFindOperationType<Functor, currentIndex + 1, UpdatedTypesPack>(std::forward<Functor>(receiver), fieldIds, nFieldIds);
				}
			}
		});
	}
};

template <class Record>
template<class Operation, class StorageAdapter, sfinae<Operation::op == OpCode::Insert>>
bool Serializer<Record>::serialize(const Operation& op, StorageIO<StorageAdapter>& io) noexcept
{
	if (!io.write(Operation::op))
		return false;

	static_assert(std::is_same_v<Record, remove_cv_and_reference_t<decltype(op._record)>>);
	return RecordSerializer::serialize(op._record, io);
}

template <class Record>
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

template <class Record>
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

template <class Record>
template<class Operation, class StorageAdapter, sfinae<Operation::op == OpCode::AppendToArray>>
bool Serializer<Record>::serialize(const Operation& op, StorageIO<StorageAdapter>& io) noexcept
{
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

template <class Record>
template<class Operation, class StorageAdapter, sfinae<Operation::op == OpCode::Delete>>
bool Serializer<Record>::serialize(const Operation& op, StorageIO<StorageAdapter>& io) noexcept
{
	if (!io.write(Operation::op))
		return false;

	constexpr auto keyFieldId = Operation::KeyField::id;
	static_assert (keyFieldId >= 0 && keyFieldId <= 255);
	if (!io.write(static_cast<uint8_t>(keyFieldId)))
		return false;

	return io.write(op.keyValue);
}

template <class Record>
template <class StorageAdapter, typename Receiver>
bool Serializer<Record>::deserialize(StorageIO<StorageAdapter>& io, Receiver&& receiver) noexcept
{
	OpCode opCode{};
	if (!io.read(opCode))
		return false;

	switch (opCode)
	{
	case OpCode::Insert:
	{
		using Op = Operation::Insert<Record>;
		Record r;
		if (!RecordSerializer::deserialize(r, io))
			return false;

		receiver(Op{ std::move(r) });
		return true;
	}
	case OpCode::Find:
	{
		uint8_t nFields = 0;
		if (!io.read(nFields))
			return false;

		static constexpr size_t maxFindOperationFieldCount = Operation::Find<>::maxFieldCount;
		assert_and_return_r(nFields > 0 && nFields <= maxFindOperationFieldCount, false);

		std::array<uint8_t, maxFindOperationFieldCount> ids;
		for (size_t i = 0; i < nFields; ++i)
		{
			if (!io.read(ids[i]))
				return false;
		}

		bool success = true;
		constructFindOperationType([&](auto OpTypeWrapper) {
			using OpType = typename decltype(OpTypeWrapper)::type;
			typename OpType::TupleOfFields fieldsTuple;
			constexpr_for_fold<0, std::tuple_size_v<typename OpType::TupleOfFields>>([&]<auto I>() {
				auto& field = std::get<I>(fieldsTuple);
				if (success && !io.readField(field))
				{
					success = false;
					return;
				}
			});

			if (success)
				receiver(OpType{std::move(fieldsTuple)});

		}, ids, nFields);

		return success;
	}
	case OpCode::UpdateFull:
	{
		uint8_t keyFieldId;
		if (!io.read(keyFieldId))
			return false;

		bool insertIfNotPresent;
		if (!io.read(insertIfNotPresent))
			return false;

		Record r;
		if (!RecordSerializer::deserialize(r, io))
			return false;

		bool success = false;
		constexpr_for_fold<0, Schema::fieldsCount_v>([&]<auto I>() {
			using KeyField = typename Schema::template FieldByIndex_t<I>;
			if (KeyField::id == keyFieldId)
			{
				typename KeyField::ValueType keyFieldValue;
				if (!io.read(keyFieldValue))
					return;

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
	case OpCode::AppendToArray:
	{
		uint8_t keyFieldId;
		if (!io.read(keyFieldId))
			return false;

		uint8_t arrayFieldId;
		if (!io.read(arrayFieldId))
			return false;

		bool insertIfNotPresent;
		if (!io.read(insertIfNotPresent))
			return false;

		bool success = false;
		// Searching the compile-time value for keyFieldId
		constexpr_for_fold<0, Schema::fieldsCount_v>([&]<auto KeyFieldIndex>() {
			using KeyField = typename Schema::template FieldByIndex_t<KeyFieldIndex>;
			if (KeyField::id == keyFieldId)
			{
				typename KeyField::ValueType keyFieldValue;
				if (!io.read(keyFieldValue))
					return;

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
								if (!RecordSerializer::deserialize(r, io))
									return;

								using Op = Operation::AppendToArray<Record, KeyField, ArrayField, true>;
								receiver(Op{ std::move(keyFieldValue), std::move(r) });
							}
							else
							{
								typename ArrayField::ValueType array;
								if (!io.read(array))
									return;

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
	case OpCode::Delete:
	{
		uint8_t keyFieldId;
		if (!io.read(keyFieldId))
			return false;

		bool success = false;
		constexpr_for_fold<0, Schema::fieldsCount_v>([&]<auto I>() {
			using KeyField = typename Schema::template FieldByIndex_t<I>;
			if (KeyField::id == keyFieldId)
			{
				typename KeyField::ValueType keyFieldValue;
				if (!io.read(keyFieldValue))
					return;

				using Op = Operation::Delete<Record, KeyField>;
				receiver(Op{ std::move(keyFieldValue) });
				success = true;
			}
		});

		return success;
	}
	}

	assert_unconditional_r("Uknown op code read from storage: " + std::to_string(static_cast<uint8_t>(opCode)));
	return false;
}

}
