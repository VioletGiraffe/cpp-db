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
	template <auto id, typename FirstField, typename... OtherFields>
	struct FieldTypeById {
		using Type = std::conditional_t<FirstField::id == id, FirstField, FieldTypeById<id, OtherFields...>>;
	};

public:
	template <auto id>
	std::vector<uint64_t> find(const typename FieldTypeById<id, IndexedFields...>::Type::ValueType& value) const
	{
		const auto range = index<id>().equal_range(value);
		std::vector<uint64_t> result;
		for (auto it = range.first; it != range.second; ++it)
			result.push_back(it->second);

		return result;
	}

	template <auto id>
	void removeAllEntriesByValue(const typename FieldTypeById<id, IndexedFields...>::Type::ValueType& value) const
	{
		index<id>().erase(range.first, range.last);
	}

	template <auto id>
	void registerValueLocation(const typename FieldTypeById<id, IndexedFields...>::Type::ValueType& value, const uint64_t location) const
	{
		// Disallow duplicate value-location pairs; only let the same value be registered at different locations.
		const auto range = index<id>().equal_range(value);
		for (auto it = range.first; it != range.second; ++it)
		{
			if (it->second == location)
				return;
		}

		index<id>().emplace(value, location);
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

	template <auto id>
	constexpr auto& index() const
	{
		constexpr auto fieldTupleIndex = detail::indexByFieldId<id, IndexedFields...>();
		return std::get<fieldTupleIndex>(_indices);
	}

private:
	std::tuple<DbIndex<IndexedFields>...> _indices;
};
