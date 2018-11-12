#pragma once

#include <map>
#include <stdint.h>
#include <tuple>

template <class IndexedField>
using DbIndex = std::multimap<typename IndexedField::ValueType /* field value */, uint64_t /* record location */>;

template <class... IndexedFields>
class Indices
{
public:
	template <class Field>
	static constexpr bool hasIndex() {
		return ((IndexedFields::id == Field::id) || ...);
	}

private:
	std::tuple<DbIndex<IndexedFields>...> _indices;
};
