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
	template <class Operation, sfinae<Operation::op == OpCode::Insert> = true>
	static [[nodiscard]] data_buffer serialize(const Operation& op);

	template <class Operation, sfinae<Operation::op == OpCode::Find> = true>
	static [[nodiscard]] data_buffer serialize(const Operation& op);

	template <class Operation, sfinae<Operation::op == OpCode::UpdateFull> = true>
	static [[nodiscard]] data_buffer serialize(const Operation& op);

	template <class Operation, sfinae<Operation::op == OpCode::AppendToArray> = true>
	static [[nodiscard]] data_buffer serialize(const Operation& op);

	template <class Operation, sfinae<Operation::op == OpCode::Delete> = true>
	static [[nodiscard]] data_buffer serialize(const Operation& op);

	template <typename Receiver>
	static bool deserialize(std::span<const char8_t> data, Receiver&& receiver);
};

template <class RecordType>
template<class Operation, sfinae<Operation::op == OpCode::Insert>>
data_buffer Serializer<DbSchema<RecordType>>::serialize(const Operation& op)
{
	return false;
}

template <class RecordType>
template<class Operation, sfinae<Operation::op == OpCode::Find>>
data_buffer Serializer<DbSchema<RecordType>>::serialize(const Operation& op)
{
	return false;
}

template <class RecordType>
template<class Operation, sfinae<Operation::op == OpCode::UpdateFull>>
data_buffer Serializer<DbSchema<RecordType>>::serialize(const Operation& op)
{
	return false;
}

template <class RecordType>
template<class Operation, sfinae<Operation::op == OpCode::AppendToArray>>
data_buffer Serializer<DbSchema<RecordType>>::serialize(const Operation& op)
{
	return false;
}

template <class RecordType>
template<class Operation, sfinae<Operation::op == OpCode::Delete>>
data_buffer Serializer<DbSchema<RecordType>>::serialize(const Operation& op)
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
