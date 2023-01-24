#pragma once

#include "dbindex.hpp"
#include "../dbfield.hpp"
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
	template <auto id>
	using FieldValueTypeById = FieldValueTypeById_t<id, IndexedFields...>;

public:
	using location_type = typename DbIndex<pack::first_type<IndexedFields...>>::location_type;

	template <auto id, typename U>
	std::optional<location_type> findKey(const U& key) const
	{
		return indexForField<id>().findKey(key);
	}

	template <auto id>
	bool addLocationForKey(FieldValueTypeById<id> key, location_type location)
	{
		return indexForField<id>().addLocationForKey(std::move(key), std::move(location));
	}

	template <auto id>
	bool removeKey(const FieldValueTypeById<id>& key) noexcept
	{
		return indexForField<id>().removeKey(key);
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

	template <class FileAdapter>
	bool load(const std::string& indexStorageFolder);
	template <class FileAdapter>
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
template <class FileAdapter>
bool Indices<IndexedFields...>::store(const std::string& indexStorageFolder)
{
	bool success = true;
	tuple::for_each(_indices, [this, &success, &indexStorageFolder](const auto& index) {
		if (!Index::store<FileAdapter>(index, indexStorageFolder))
		{
			success = false;
			return;
		}
	});

	assert_r(success);
	return success;
}

template <class... IndexedFields>
template <class FileAdapter>
bool Indices<IndexedFields...>::load(const std::string& indexStorageFolder)
{
	bool success = true;
	tuple::for_each(_indices, [this, &success, &indexStorageFolder](auto& index) {
		if (!Index::load<FileAdapter>(index, indexStorageFolder))
		{
			success = false;
			return;
		}
	});

	assert_r(success);
	return success;
}
