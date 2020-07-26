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

public:
	template <class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::Insert> = true>
	static [[nodiscard]] bool serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept;

	template <class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::Find> = true>
	static [[nodiscard]] bool serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept;

	template <class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::UpdateFull> = true>
	static [[nodiscard]] bool serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept;

	template <class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::AppendToArray> = true>
	static [[nodiscard]] bool serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept;

	template <class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::Delete> = true>
	static [[nodiscard]] bool serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept;

	template <typename Receiver, typename StorageImplementation>
	static [[nodiscard]] bool deserialize(StorageIO<StorageImplementation>& io, Receiver&& receiver) noexcept;

private:
	using OpSerializer = RecordSerializer<Record>;

	template <size_t First, size_t Last, typename Functor, typename ArgsList>
	static constexpr void constructFindOperationType([[maybe_unused]] Functor&& f, const size_t nFields, ArgsList) noexcept
	{
		if constexpr (First < Last)
		{
			using FieldType = typename DbSchema<Record>::template FieldById_t<First>;
			using List = typename ArgsList::template Append<FieldType>;
			f(value_as_type<First>{});
			if (First == nFields - 1)
			{
			}
			else
				constructFindOperationType<First + 1, Last, Functor>(std::forward<Functor>(f), nFields, List{});
		}
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
	constexpr_for<0, nFields>([&](auto&& i) {
		const auto& field = std::get<i>(op._fields);
		using FieldType = remove_cv_and_reference_t<decltype(field)>;
		static_assert(FieldType::id <= 255);

		if (!io.write(static_cast<uint8_t>(FieldType::id)))
			return false;
	});

	// And now the values
	constexpr_for<0, nFields>([&](auto&& i) {
		const auto& field = std::get<i>(op._fields);
		if (!io.writeField(field))
			return false;
	});

	return true;
}

template <typename... RecordParams>
template<class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::UpdateFull>>
bool Serializer<DbRecord<RecordParams...>>::serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept
{
	return false;
}

template <typename... RecordParams>
template<class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::AppendToArray>>
bool Serializer<DbRecord<RecordParams...>>::serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept
{
	return false;
}

template <typename... RecordParams>
template<class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::Delete>>
bool Serializer<DbRecord<RecordParams...>>::serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept
{
	return false;
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
	case OpCode::Find:
	{
		uint8_t nFields = 0;
		if (!io.read(nFields))
			return false;

		uint8_t ids[256] = { 0 };
		for (size_t i = 0; i < nFields; ++i)
		{
			if (!io.read(ids[i]))
				return false;
		}

		constexpr_for<0, 255>([&](auto index) -> bool {
			constexpr const int i = index;
			using FieldType = typename DbSchema<Record>::template FieldById_t<i>;
			using List = ArgumentsList<>::Append<FieldType>;
			return i < nFields;
		});

		return true;
	}
	case OpCode::UpdateFull:
		break;
	case OpCode::AppendToArray:
		break;
	case OpCode::Delete:
		break;
	}

	assert_unconditional_r("Uknown op code read from storage: " + std::to_string(static_cast<uint8_t>(opCode)));
	return false;
}

}