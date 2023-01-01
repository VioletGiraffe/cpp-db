#pragma once

#include "../dbfield.hpp"
#include "../dbstorage.hpp"
#include "utility/template_magic.hpp"
#include "assert/advanced_assert.h"

#include <map>
#include <optional>

template <typename IndexedField>
class DbIndex
{
	static_assert(IndexedField::isField());

public:
	using key_type = typename IndexedField::ValueType;

	std::optional<PageNumber> findValueLocation(const key_type& value) const noexcept
	{
		const auto it = _index.find(value);
		return it != _index.end() ? it->second : std::optional<PageNumber>{};
	}

	// Returns false if this value-location pair is already registered (no duplicate will be added), otherwise true
	bool addLocationForKey(key_type value, PageNumber pgN) noexcept
	{
		// Duplicate keys not allowed!
		const auto result = _index.emplace(std::move(value), std::move(pgN));
		return result.second == true; // insertion occurred
	}

	// Removes every occurrence of 'value', returns the number of removed items
	size_t removeAllValueLocations(const key_type& value) noexcept
	{
		return _index.erase(value);
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

#ifdef TEST_CASE
	void clear()
	{
		_index.clear();
	}
#endif

private:
	std::map<key_type /* key value */, PageNumber> _index;
};
