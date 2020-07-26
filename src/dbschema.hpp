#pragma once

#include "parameter_pack/parameter_pack_helpers.hpp"

template <class RecordType>
struct DbSchema
{
	using Record = RecordType;
	using Fields = typename RecordType::Fields;

	static constexpr size_t fieldsCount_v = sizeof...(Fields);

	template <auto id>
	using FieldById_t = typename Record::template FieldById_t<id>;

	template <size_t index>
	using FieldByIndex_t = typename Record::FieldTypesPack::template Type<index>;
};
