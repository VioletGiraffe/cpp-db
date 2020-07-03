#pragma once

#include "parameter_pack/parameter_pack_helpers.hpp"

template <class RecordType>
struct DbSchema
{
	using Record = RecordType;
	using Fields = typename RecordType::Fields;

	template <auto id>
	using FieldById_t = typename Record::template FieldById_t<id>;
};
