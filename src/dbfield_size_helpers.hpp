#pragma once

#include "assert/advanced_assert.h"
#include "dbfield.hpp"

#include <limits>
#include <stdint.h>

template<>
size_t valueSize<std::string>(const std::string& value) {
	assert_debug_only(value.size() < std::numeric_limits<uint32_t>::max());
	return sizeof(uint32_t) /* 4 bytes for storing the string's length */ + value.size();
}
