#pragma once

#include "../dbschema.hpp"
#include "utility/data_buffer.hpp"

namespace Operation {

template <class>
class Serializer;

template <class RecordType>
class Serializer<DbSchema<RecordType>>
{
	using Schema = DbSchema<RecordType>;

public:
	template <class OpType>
	data_buffer serialize(OpType&& op) const;

	template <typename Receiver>
	bool deserialize(std::span<const char8_t> data, Receiver&& receiver);
};

template<class RecordType>
template<class OpType>
data_buffer Serializer<DbSchema<RecordType>>::serialize(OpType&& op) const
{
	return {};
}

template<class RecordType>
template<typename Receiver>
bool Serializer<DbSchema<RecordType>>::deserialize(std::span<const char8_t> data, Receiver&& receiver)
{

}


}
