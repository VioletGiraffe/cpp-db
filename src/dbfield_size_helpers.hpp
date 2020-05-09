#pragma once

#include "assert/advanced_assert.h"
#include "utility/extra_type_traits.hpp"

#include <numeric>
#include <limits>
#include <stdint.h>
#include <string>
#include <vector>

inline size_t valueSize(const std::string& value) noexcept
{
	assert_debug_only(value.size() < std::numeric_limits<uint32_t>::max());
	return sizeof(uint32_t) /* 4 bytes for storing the string's length */ + value.size();
}

template <typename T>
size_t valueSize(const std::vector<T>& v) noexcept
{
	assert_debug_only(v.size() < std::numeric_limits<uint32_t>::max());
	static_assert(!std::is_reference_v<T> && !std::is_pointer_v<T>);

	if constexpr (is_trivially_serializable_v<T>)
		return sizeof(uint32_t) /* 4 bytes for storing the string's length */ + v.size() * sizeof(T);
	else
		return sizeof(uint32_t) /* 4 bytes for storing the string's length */ + std::accumulate(v.cbegin(), v.cend(), size_t{ 0 }, [](auto&& acc, auto&& item) { return acc + ::valueSize(item); });
}