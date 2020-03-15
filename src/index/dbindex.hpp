#pragma once
#include "dbfield.hpp"
#include "dbstorage.hpp"
#include "utility/template_magic.hpp"
#include "assert/advanced_assert.h"

#include <map>
#include <optional>
#include <stdint.h>
#include <vector>

template <typename IndexedField>
class DbIndex
{
	using FieldValueType = typename IndexedField::ValueType;

	static_assert(IndexedField::isField());

public:
	std::vector<StorageLocation> findValueLocations(FieldValueType value) const noexcept
	{
		const auto [begin, end] = _index.equal_range(std::move(value));
		std::vector<StorageLocation> locations;
		for (auto it = begin; it != end; ++it)
			locations.emplace_back(it->second);

		return locations;
	}

	bool addLocationForValue(FieldValueType value, StorageLocation location) noexcept
	{
		return _index.emplace(std::move(value), std::move(location)).second;
	}

	bool removeValueLocation(const FieldValueType& value, const StorageLocation location) noexcept
	{
		const auto [begin, end] = _index.equal_range(value); // TODO: std::move?
		
		if (begin != end)
		{
			// No more than one item with the same location and same value
			assert_debug_only(std::count(begin, end, value) <= 1); // TODO: add the same check, but for all values (no duplicate locations)
			_index.erase(begin);
			return true;
		}
		else
			return false;
	}

	// Removes every occurrence of 'value', returns the number of removed items
	size_t removeAllValueLocations(FieldValueType value) noexcept
	{
		return _index.erase(std::move(value));
	}

	auto begin() const {
		return _index.begin();
	}

	auto end() const {
		return _index.end();
	}

#ifdef CATCH_CONFIG_MAIN
	const std::multimap <FieldValueType, StorageLocation>& contents() const {
		return _index;
	}
#endif

private:
	std::multimap<FieldValueType /* field value */, StorageLocation /* record location */> _index;
};
