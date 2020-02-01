#pragma once
#include "dbfield.hpp"
#include "dbrecord.hpp"
#include "dbfilegaps.hpp"
#include "dbindex.hpp"
#include "dbstorage.hpp"

#include "parameter_pack/parameter_pack_helpers.hpp"
#include "assert/advanced_assert.h"

#include <string>
#include <tuple>
#include <vector>

enum class TestCollectionFields {
	Id,
	Text
};

template <class Index, class... Fields>
class Collection
{
	static constexpr size_t dynamicFieldCount()
	{
		size_t dynamicFieldCount = 0;
		static_for<0, sizeof...(Fields)>([&dynamicFieldCount](auto i) {
			using FieldType = pack::type_by_index<decltype(i)::value, Fields...>;
			if constexpr (FieldType::hasStaticSize() == false)
				++dynamicFieldCount;
		});

		return dynamicFieldCount;
	}

	static_assert(dynamicFieldCount() <= 1, "No more than one dynamic field is allowed!");

public:
	using Record = std::tuple<Fields...>;

public:
	Collection(const std::string& collectionName, const std::string& databaseFolderPath) :
		_dbStoragePath{databaseFolderPath},
		_collectionName{collectionName}
	{
		assert_r(_storage.openStorageFile(databaseFolderPath + '/' + collectionName));

		assert_r(_index.load(indexStorageFolderPath()));
	}

	~Collection()
	{
		_index.store(indexStorageFolderPath());
	}

	std::string indexStorageFolderPath() const
	{
		return _dbStoragePath + "/" + _collectionName + "_index/";
	}

	template <typename Functor, auto... queryFieldIds>
	void insert(Functor&& predicate, bool updateIfExists = false)
	{

	}

	// TODO: add default functor for one value (no filter)
	template <auto queryFieldId>
	std::vector<Record> find(const FieldValueTypeById_t<queryFieldId, Fields...>& value) {
		static_assert(Index::template hasIndex<queryFieldId>(), "Attempting to query on an un-indexed field!");

		std::vector<Record> results;

		const auto locations = _index.template find<queryFieldId>(value);
		for (const uint64_t location : locations)
			results.emplace_back(_storage.readRecord(location));

		return results;
	}

private:
	DBStorage<Fields...> _storage;

	const std::string _dbStoragePath;
	const std::string _collectionName;

	Index _index;
};
