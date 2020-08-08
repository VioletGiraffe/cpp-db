#pragma once

#include "../dbrecord.hpp"
#include "../dbops.hpp"
#include "../dbschema.hpp"
#include "../storage/storage_io_interface.hpp"
#include "../serialization/dbrecord-serializer.hpp"

#include "tuple/tuple_helpers.hpp"
#include "utility/template_magic.hpp"

namespace Operation {

template <class T>
class Serializer {
	static_assert(false_v<T>, "This shouldn't be instantiated - check the template parameter list for errors!");
};

template <typename... RecordParams>
class Serializer<DbRecord<RecordParams...>>
{
	using Record = DbRecord<RecordParams...>;
	using Schema = DbSchema<Record>;

public:
	template <class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::Insert> = true>
	[[nodiscard]] static bool serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept;

	template <class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::Find> = true>
	[[nodiscard]] static bool serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept;

	template <class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::UpdateFull> = true>
	[[nodiscard]] static bool serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept;

	template <class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::AppendToArray> = true>
	[[nodiscard]] static bool serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept;

	template <class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::Delete> = true>
	[[nodiscard]] static bool serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept;

	template <typename Receiver, typename StorageImplementation>
	[[nodiscard]] static bool deserialize(StorageIO<StorageImplementation>& io, Receiver&& receiver) noexcept;

private:
	using OpSerializer = RecordSerializer<Record>;

	template <size_t fieldsCount, size_t currentFieldIndex = 0, typename Functor, typename TypesPack = type_pack<>>
	static constexpr void constructFindOperationType([[maybe_unused]] Functor&& receiver, const uint8_t(&fieldIds)[256]) noexcept
	{
		if constexpr (currentFieldIndex == fieldsCount)
		{
			using OpType = typename TypesPack::template Construct<Operation::Find>;
			receiver(OpType{});
			return; // Terminate iteration - all IDs processed
		}

		// Get the compile-time ID of the current field
		constexpr_for<0, Schema::fieldsCount_v>([&](auto i) {
			using FieldType = typename Schema::template FieldByIndex_t<i>;
			if (fieldIds[currentFieldIndex] == FieldType::id)
				constructFindOperationType<fieldsCount, currentFieldIndex + 1, Functor, typename TypesPack::template Append<FieldType>>(std::forward<Functor>(receiver), fieldIds);
		});
	}
};

template <typename... RecordParams>
template<class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::Insert>>
bool Serializer<DbRecord<RecordParams...>>::serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept
{
	if (!io.write(Operation::op))
		return false;

	static_assert(std::is_same_v<Record, remove_cv_and_reference_t<decltype(op._record)>>);
	return OpSerializer::serialize(op._record, io);
}

template <typename... RecordParams>
template<class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::Find>>
bool Serializer<DbRecord<RecordParams...>>::serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept
{
	if (!io.write(Operation::op))
		return false;

	// This op holds a bunch of fields to search on.

	// First, write the # of fields.
	constexpr auto nFields = std::tuple_size_v<decltype(op._fields)>;
	static_assert(nFields <= 255);
	if (!io.write(static_cast<uint8_t>(nFields)))
		return false;

	// Now writing IDs of each the field in order.
	constexpr_for<0, nFields>([&](auto i) {
		const auto& field = std::get<i.to_value()>(op._fields);
		using FieldType = remove_cv_and_reference_t<decltype(field)>;
		static_assert(FieldType::id <= 255);

		if (!io.write(static_cast<uint8_t>(FieldType::id)))
			return false;
	});

	// And now the values
	constexpr_for<0, nFields>([&](auto i) {
		const auto& field = std::get<i.to_value()>(op._fields);
		if (!io.writeField(field))
			return false;
	});

	return true;
}

template <typename... RecordParams>
template<class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::UpdateFull>>
bool Serializer<DbRecord<RecordParams...>>::serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept
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
	if (!OpSerializer::serialize(op.record, io))
		return false;

	return io.write(op.keyValue);
}

template <typename... RecordParams>
template<class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::AppendToArray>>
bool Serializer<DbRecord<RecordParams...>>::serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept
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

	static_assert(std::is_same_v<Record, remove_cv_and_reference_t<decltype(op.record)>>);
	if (!OpSerializer::serialize(op.record, io))
		return false;

	if (!io.write(op.keyValue))
		return false;

	if constexpr (op.insertIfNotPresent())
		return io.write(op.record);
	else
		return io.write(op.array);
}

template <typename... RecordParams>
template<class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::Delete>>
bool Serializer<DbRecord<RecordParams...>>::serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept
{
	if (!io.write(Operation::op))
		return false;

	constexpr auto keyFieldId = Operation::KeyField::id;
	static_assert (keyFieldId >= 0 && keyFieldId <= 255);
	if (!io.write(static_cast<uint8_t>(keyFieldId)))
		return false;

	return io.write(op.keyValue);
}

template <typename... RecordParams>
template<typename Receiver, typename StorageImplementation>
bool Serializer<DbRecord<RecordParams...>>::deserialize(StorageIO<StorageImplementation>& io, Receiver&& receiver) noexcept
{
	OpCode opCode = OpCode::Invalid;
	if (!io.read(opCode))
		return false;

	switch (opCode)
	{
	case OpCode::Insert:
	{
		using Op = Operation::Insert<Record>;
		Record r;
		if (!RecordSerializer<Record>::deserialize(r, io))
			return false;

		receiver.operator()(Op{ std::move(r) });
		return true;
	}
//	case OpCode::Find:
//	{
//		uint8_t nFields = 0;
//		if (!io.read(nFields))
//			return false;

//		assert_and_return_r(nFields > 0 && nFields <= Operation::Find<>::maxFieldCount, false);

//		uint8_t ids[256] = { 0 };
//		for (size_t i = 0; i < nFields; ++i)
//		{
//			if (!io.read(ids[i]))
//				return false;
//		}

//		//constexpr_from_runtime_value<1, Operation::Find<>::maxFieldCount>(nFields, [&](auto nFieldsConstexpr){
//			constructFindOperationType<3>([&](auto opPrototype) {
//				using OpType = decltype(opPrototype);
//				receiver.operator()(OpType{});
//			}, ids);
//		//});

//		return true;
//	}
	case OpCode::UpdateFull:
	{
		uint8_t keyFieldId;
		if (!io.read(keyFieldId))
			return false;

		bool insertIfNotPresent;
		if (!io.read(insertIfNotPresent))
			return false;

		Record r;
		if (!RecordSerializer<Record>::deserialize(r, io))
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
					receiver.operator()(Op{ std::move(r), std::move(keyFieldValue) });
				}
				else
				{
					using Op = Operation::UpdateFull<Record, KeyField, false>;
					receiver.operator()(Op{ std::move(r), std::move(keyFieldValue) });
				}
				success = true;
			}
		});

		return success;
	}
	case OpCode::AppendToArray:
		break;
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
				receiver.operator()(Op{ std::move(keyFieldValue) });
				success = true;
			}
		});

		return success;
	}
	case OpCode::Invalid:
		break;
	}

	assert_unconditional_r("Uknown op code read from storage: " + std::to_string(static_cast<uint8_t>(opCode)));
	return false;
}

}
