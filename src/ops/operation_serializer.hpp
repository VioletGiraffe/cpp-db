#pragma once

#include "../dbrecord.hpp"
#include "../dbops.hpp"
#include "../storage/storage_io_interface.hpp"
#include "../serialization/dbrecord-serializer.hpp"

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
};

template <typename... RecordParams>
template<class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::Insert>>
bool Serializer<DbRecord<RecordParams...>>::serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept
{
	if (!io.write(Operation::op))
		return false;

	static_assert(std::is_same_v<Record, remove_cv_and_reference_t<decltype(op._record)>>);
	using OpSerializer = RecordSerializer<Record>;
	return OpSerializer::serialize(op._record, io);
}

template <typename... RecordParams>
template<class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::Find>>
bool Serializer<DbRecord<RecordParams...>>::serialize(const Operation& op, StorageIO<StorageImplementation>& io) noexcept
{
	return false;
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

		receiver(Op{ std::move(r) });
		return true;
	}
	case OpCode::Find:
		break;
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