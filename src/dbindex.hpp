#pragma once
#include "dbfield.hpp"

#include <stdint.h>

template <class IndexedField>
using DbIndex = std::multimap<IndexedField/* field value */, uint64_t /* record location */>;
