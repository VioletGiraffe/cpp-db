#pragma once

template <typename Field>
concept FieldType = requires(Field)
{
	requires Field::isField;
};

template <typename Record>
concept RecordType = requires(Record)
{
	requires Record::isRecord;
};
