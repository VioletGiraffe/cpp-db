#pragma once

#include "dbindex.hpp"
#include "../dbfield.hpp"
#include "../storage/storage_qt.hpp"
#include "index_persistence.hpp"
#include "../index_helpers.hpp"

#include "utility/constexpr_algorithms.hpp"
#include "assert/advanced_assert.h"
#include "tuple/tuple_helpers.hpp"

#include <string>
#include <tuple>
#include <type_traits>
#include <vector>

template <class... IndexedFields>
class Indices
{
public:
	template <auto id>
	using FieldValueTypeById = FieldValueTypeById_t<id, IndexedFields...>;

	template <auto id, typename U>
	std::vector<StorageLocation> findValueLocations(U&& value) const
	{
		return indexForField<id>().findValueLocations(std::forward<U>(value));
	}

	template <auto id>
	bool addLocationForValue(FieldValueTypeById<id> value, StorageLocation location)
	{
		return indexForField<id>().addLocationForValue(std::move(value), std::move(location));
	}

	template <auto id>
	bool removeValueLocation(FieldValueTypeById<id> value, const StorageLocation location) noexcept
	{
		return indexForField<id>().removeValueLocation(std::move(value), std::move(location));
	}

	template <auto id>
	size_t removeAllValueLocations(FieldValueTypeById<id> value)
	{
		return indexForField<id>().removeAllValueLocations(std::move(value));
	}

	template <auto id>
	static consteval bool hasIndex()
	{
		return ((IndexedFields::id == id) || ...);
	}

	template <typename FieldType>
	static consteval bool hasIndex()
	{
		return hasIndex<FieldType::id>();
	}

	template <auto id>
	constexpr auto& indexForField()
	{
		constexpr auto fieldTupleIndex = detail::indexByFieldId<id, IndexedFields...>;
		return std::get<fieldTupleIndex>(_indices);
	}

	template <auto id>
	constexpr const auto& indexForField() const
	{
		constexpr auto fieldTupleIndex = detail::indexByFieldId<id, IndexedFields...>;
		return std::get<fieldTupleIndex>(_indices);
	}

	bool load(const std::string& indexStorageFolder);
	bool store(const std::string& indexStorageFolder);

private:
	static consteval bool sanityCheck()
	{
		bool success = true;
		pack::for_type<IndexedFields...>([&]<class T>() {
			if constexpr (pack::type_count<T, IndexedFields...>() != 1)
				success = false;
		});

		return success;
	}

	static_assert(sanityCheck(), "Indices<...> sanity check failed");

private:
	std::tuple<DbIndex<IndexedFields>...> _indices;
};

template <class... IndexedFields>
bool Indices<IndexedFields...>::store(const std::string& indexStorageFolder)
{
	bool success = true;
	tuple::for_each(_indices, [this, &success, &indexStorageFolder](const auto& index) {
		if (!Index::store<io::QFileAdapter>(index, indexStorageFolder))
		{
			success = false;
			return;
		}
	});

	assert_r(success);
	return success;
}

template <class... IndexedFields>
bool Indices<IndexedFields...>::load(const std::string& indexStorageFolder)
{
	bool success = true;
	tuple::for_each(_indices, [this, &success, &indexStorageFolder](auto& index) {
		if (!Index::load<io::QFileAdapter>(index, indexStorageFolder))
		{
			success = false;
			return;
		}
	});

	assert_r(success);
	return success;
}
