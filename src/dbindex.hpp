#pragma once

#include <map>
#include <stdint.h>

template <class IndexedField>
struct DbIndex
{
private:
	std::multimap<typename IndexedField::ValueType /* field value */, uint64_t /* record location */> _index;
};
