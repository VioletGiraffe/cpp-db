#pragma once

#include "dbindex.hpp"
#include "../dbfield.hpp"
#include "index_persistence.hpp"
#include "../index_helpers.hpp"

#include "utility/constexpr_algorithms.hpp"
#include "assert/advanced_assert.h"
#include "tuple/tuple_helpers.hpp"
#include "container/string_helpers.hpp"

#include <map>
#include <stdint.h>
#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

template <class... IndexedFields>
class Indices
{
public:
	template <auto id>
	using FieldValueTypeById = typename FieldValueTypeById_t<id, IndexedFields...>::ValueType;

	template <auto id, typename U>
	std::vector<uint64_t> find(U&& value) const
	{
		const auto range = indexForField<id>().equal_range(std::forward<U>(value));
		std::vector<uint64_t> result;
		for (auto it = range.first; it != range.second; ++it)
			result.push_back(it->second);

		return result;
	}

	template <auto id>
	void removeAllEntriesByValue(const FieldValueTypeById<id>& value)
	{
		const auto range = indexForField<id>().equal_range(value);
		indexForField<id>().erase(range.first, range.last);
	}

	template <auto id>
	void registerValueLocation(const FieldValueTypeById<id>& value, const uint64_t location) const
	{
		// Disallow duplicate value-location pairs; only let the same value be registered at different locations.
		const auto range = indexForField<id>().equal_range(value);
		for (auto it = range.first; it != range.second; ++it)
		{
			if (it->second == location)
				return;
		}

		indexForField<id>().emplace(value, location);
	}

	template <auto id>
	static constexpr bool hasIndex()
	{
		return ((IndexedFields::id == id) || ...);
	}

	template <auto id>
	constexpr auto& indexForField()
	{
		constexpr auto fieldTupleIndex = detail::indexByFieldId<id, IndexedFields...>();
		return std::get<fieldTupleIndex>(_indices);
	}

	template <auto id>
	constexpr const auto& indexForField() const
	{
		constexpr auto fieldTupleIndex = detail::indexByFieldId<id, IndexedFields...>();
		return std::get<fieldTupleIndex>(_indices);
	}

	bool load(const std::string& indexStorageFolder);
	bool store(const std::string& indexStorageFolder);

private:
	std::tuple<DbIndex<IndexedFields>...> _indices;
};

template <class... IndexedFields>
bool Indices<IndexedFields...>::store(const std::string& indexStorageFolder)
{
	bool success = true;
	tuple::for_each(_indices, [this, &success, &indexStorageFolder](const auto& index) {
		if (!Index::store(index, indexStorageFolder))
		{
			success = false;
			return;
		}
	});

	assert_r(success);
	return success;
}

template <class... IndexedFields>
bool Indices<IndexedFields...>::load(const std::string& indexStorageFolder)
{
	bool success = true;
	tuple::for_each(_indices, [this, &success, &indexStorageFolder](auto& index) {
		if (!Index::load(index, indexStorageFolder))
		{
			success = false;
			return;
		}
	});

	assert_r(success);
	return success;
}
