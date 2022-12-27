#pragma once

#include <concepts>

template <typename Record>
concept RecordConcept = requires(Record)
{
	requires Record::isRecord == true;
};