#pragma once

#include "../dbstorage.hpp"

#include <map>
#include <optional>

template <FieldType IndexedField>
class DbIndex
{
public:
	using key_type = typename IndexedField::ValueType;
	using location_type = PageNumber;

	[[nodiscard]] std::optional<location_type> findKey(const key_type& value) const noexcept
	{
		const auto it = _index.find(value);
		return it != _index.end() ? it->second : std::optional<location_type>{};
	}

	// Returns false if this value-location pair is already registered (no duplicate will be added), otherwise true
	bool addLocationForKey(key_type value, location_type pgN) noexcept
	{
		// Duplicate keys not allowed!
		const auto result = _index.emplace(std::move(value), std::move(pgN));
		return result.second == true; // insertion occurred
	}

	// Removes every occurrence of 'value', returns the number of removed items
	size_t removeKey(const key_type& value) noexcept
	{
		return _index.erase(value);
	}

	[[nodiscard]] auto begin() const noexcept
	{
		return _index.begin();
	}

	[[nodiscard]] auto end() const noexcept
	{
		return _index.end();
	}

	[[nodiscard]] size_t size() const noexcept
	{
		return _index.size();
	}

	[[nodiscard]] bool empty() const noexcept
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
	std::map<key_type /* key value */, location_type> _index;
};
