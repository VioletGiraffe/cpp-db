#pragma once
#include "dbfield.hpp"
#include "utility/template_magic.hpp"

#include <map>
#include <optional>
#include <stdint.h>
#include <vector>

template <class IndexedField>
class DbIndex
{
	using ValueType = typename IndexedField::ValueType;

	static_assert(IndexedField::is_field_v);

public:
	std::vector<uint64_t> findValue(ValueType value)
	{
		return {};
	}

private:
	std::multimap<ValueType /* field value */, uint64_t /* record location */> _index;
};

