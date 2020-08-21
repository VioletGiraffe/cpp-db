#pragma once

template <class RecordType>
struct DbSchema
{
	using Record = RecordType;
	using Fields = typename RecordType::FieldTypesPack;

	static constexpr size_t fieldsCount_v = Fields::type_count;

	template <auto id>
	using FieldById_t = typename Record::template FieldById_t<id>;

	template <int index>
	using FieldByIndex_t = typename Fields::template Type<index>;
};
