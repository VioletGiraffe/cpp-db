#pragma once

#include <concepts>

template <typename Record>
concept RecordConcept = requires(Record)
{
	{Record::isRecord} -> std::convertible_to<bool>;
};