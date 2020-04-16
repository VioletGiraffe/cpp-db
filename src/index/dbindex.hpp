#pragma once

#include "../dbfield.hpp"
#include "../dbstorage.hpp"
#include "utility/template_magic.hpp"
#include "assert/advanced_assert.h"

#include <iterator>
#include <map>
#include <optional>
#include <stdint.h>
#include <vector>

template <typename IndexedField>
class DbIndex
{
	static_assert(IndexedField::isField());

public:
	using FieldValueType = typename IndexedField::ValueType;
	using key_type = FieldValueType;
	using mapped_type = StorageLocation;

	std::vector<StorageLocation> findValueLocations(FieldValueType value) const noexcept
	{
		const auto [begin, end] = _index.equal_range(std::move(value));
		std::vector<StorageLocation> locations;
		// TODO: reserve(std::distance(begin, end))
		for (auto it = begin; it != end; ++it)
			locations.emplace_back(it->second);

		return locations;
	}

	// Returns false if this value-location pair is already registered (no duplicate will be added), otherwise true
	bool addLocationForValue(FieldValueType value, StorageLocation location) noexcept
	{
		// Disallow duplicate value-location pairs; only let the same value be registered at different locations.
		const auto [begin, end] = _index.equal_range(value);
		for (auto it = begin; it != end; ++it)
		{
			if (it->second == location)
				return false;
		}

#ifdef _DEBUG
		// Checking that the supplied location is globally unique
		for (auto&& item : _index)
			assert_r(item.second != location);
#endif

		_index.emplace(std::move(value), std::move(location));
		return true;
	}

	bool removeValueLocation(const FieldValueType& value, const StorageLocation location) noexcept
	{
		const auto [begin, end] = _index.equal_range(value); // TODO: std::move?
		if (begin != end)
		{
			// Among all records for this value, looking for the particular requested location
			const auto target = std::find_if(begin, end, [&](auto&& item) { return item.second == location; });
			if (target == end)
				return false;


			// The check in addLocationForValue ensures that there can be no more than one of any value+location pair.
			// Still, double-checking for sanity.
			assert_debug_only(std::find_if(std::next(target), end, [&](auto&& item) { return item.second == location; }) == end);

			_index.erase(target);
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

	auto begin() const noexcept
	{
		return _index.begin();
	}

	auto end() const noexcept
	{
		return _index.end();
	}

	size_t size() const noexcept
	{
		return _index.size();
	}

	bool empty() const noexcept
	{
		return _index.empty();
	}

#ifdef CATCH_CONFIG_MAIN
	void clear()
	{
		_index.clear();
	}
#endif

private:
	std::multimap<FieldValueType /* field value */, StorageLocation /* record location */> _index;
};
