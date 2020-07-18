#pragma once

#include "../dbschema.hpp"
#include "../dbops.hpp"
#include "utility/data_buffer.hpp"

namespace Operation {

template <class>
class Serializer;

template <class RecordType>
class Serializer<DbSchema<RecordType>>
{
	using Schema = DbSchema<RecordType>;

public:
	template <class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::Insert> = true>
	static [[nodiscard]] bool serialize(const Operation& op);

	template <class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::Find> = true>
	static [[nodiscard]] bool serialize(const Operation& op);

	template <class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::UpdateFull> = true>
	static [[nodiscard]] bool serialize(const Operation& op);

	template <class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::AppendToArray> = true>
	static [[nodiscard]] bool serialize(const Operation& op);

	template <class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::Delete> = true>
	static [[nodiscard]] bool serialize(const Operation& op);

	template <typename Receiver>
	static bool deserialize(std::span<const char8_t> data, Receiver&& receiver);
};

template <class RecordType>
template<class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::Insert>>
bool Serializer<DbSchema<RecordType>>::serialize(const Operation& op)
{

	return false;
}

template <class RecordType>
template<class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::Find>>
bool Serializer<DbSchema<RecordType>>::serialize(const Operation& op)
{
	return false;
}

template <class RecordType>
template<class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::UpdateFull>>
bool Serializer<DbSchema<RecordType>>::serialize(const Operation& op)
{
	return false;
}

template <class RecordType>
template<class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::AppendToArray>>
bool Serializer<DbSchema<RecordType>>::serialize(const Operation& op)
{
	return false;
}

template <class RecordType>
template<class Operation, typename StorageImplementation, sfinae<Operation::op == OpCode::Delete>>
bool Serializer<DbSchema<RecordType>>::serialize(const Operation& op)
{
	return false;
}

template<class RecordType>
template<typename Receiver>
bool Serializer<DbSchema<RecordType>>::deserialize(std::span<const char8_t> data, Receiver&& receiver)
{
	return false;
}

}