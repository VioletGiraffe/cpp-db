#pragma once

#include "dbindex.hpp"
#include "dbfield.hpp"
#include "dbstorage.hpp"
#include "index_helpers.hpp"
#include "utility/constexpr_algorithms.hpp"
#include "assert/advanced_assert.h"
#include "tuple/tuple_helpers.hpp"
#include "container/string_helpers.hpp"

#include <QFile>

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
	using FieldValueTypeById_t = typename FieldTypeById_t<id, IndexedFields...>::ValueType;

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
	void removeAllEntriesByValue(const typename FieldTypeById_t<id, IndexedFields...>::ValueType& value)
	{
		const auto range = indexForField<id>().equal_range(value);
		indexForField<id>().erase(range.first, range.last);
	}

	template <auto id>
	void registerValueLocation(const typename FieldTypeById_t<id, IndexedFields...>::ValueType& value, const uint64_t location) const
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
	tuple::for_each(_indices, [this, &success, &indexStorageFolder](const auto& item) {
		if (!success)
		{
			success = false;
			return;
		}

		const auto filePath = QString::fromStdString(indexStorageFolder) + "/" + QString(typeid(item).name()).remove(':').remove('.') + ".index";
		QFile file(filePath);
		success = file.open(QFile::WriteOnly);
		assert_and_return_r(success, );

		for (const auto& indexEntry : item)
		{
			if (!StorageIO::write(indexEntry.first, file) || file.write(reinterpret_cast<const char*>(&indexEntry.second), sizeof(indexEntry.second)) != sizeof(indexEntry.second))
			{
				success = false;
				return;
			}
		}
	});

	assert_r(success);
	return success;
}

template <class... IndexedFields>
bool Indices<IndexedFields...>::load(const std::string& indexStorageFolder)
{
	bool success = true;
	tuple::for_each(_indices, [this, &success, &indexStorageFolder](auto& item) {
		if (!success)
		{
			success = false;
			return;
		}

		const auto filePath = QString::fromStdString(indexStorageFolder) + "/" + QString(typeid(item).name()).remove(':').remove('.') + ".index";
		QFile file(filePath);
		success = file.open(QFile::ReadOnly);
		assert_and_return_r(success, );

		while (!file.atEnd())
		{
			using IndexMultiMapType = std::decay_t<decltype(item)>;
			auto field = typename IndexMultiMapType::key_type{};
			auto offset = typename IndexMultiMapType::mapped_type{0};
			if (!StorageIO::read(field, file) || file.read(reinterpret_cast<char*>(&offset), sizeof(offset)) != sizeof(offset))
			{
				success = false;
				return;
			}

			item.emplace(field, offset);
		}
	});

	assert_r(success);
	return success;
}
