#pragma once
#include "utility/constexpr_algorithms.hpp"
#include "tuple/tuple_helpers.hpp"
#include "index_helpers.hpp"

#include <map>
#include <stdint.h>
#include <tuple>
#include <vector>

template <class IndexedField>
using DbIndex = std::multimap<typename IndexedField::ValueType /* field value */, uint64_t /* record location */>;

template <class... IndexedFields>
class Indices
{
public:
	template <class Field>
	std::vector<uint64_t> find(const typename Field::ValueType& value)
	{
		return {};
	}

	template <auto id>
	static constexpr bool hasIndex()
	{
		return ((IndexedFields::id == id) || ...);
	}

	template <auto id>
	constexpr auto& index()
	{
		constexpr auto fieldTupleIndex = detail::indexByFieldId<id, IndexedFields...>();
		return std::get<fieldTupleIndex>(_indices);
	}

private:
	std::tuple<DbIndex<IndexedFields>...> _indices;
};
