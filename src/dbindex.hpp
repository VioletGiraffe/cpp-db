#pragma once
#include "dbfield.hpp"
#include "dbstorage.hpp"
#include "utility/template_magic.hpp"

#include <map>
#include <optional>
#include <stdint.h>
#include <vector>

template <class IndexedField>
class DbIndex
{
	using FieldValueType = typename IndexedField::ValueType;

	static_assert(IndexedField::is_field_v);

public:
	std::vector<DBStorage::StorageLocation> findRecordsByFieldValue(FieldValueType value)
	{
		const auto range = _index.equal_range(std::move(value));
		return {};
	}

private:
	std::multimap<FieldValueType /* field value */, DBStorage::StorageLocation /* record location */> _index;
};
