#pragma once
#include "dbfield.hpp"
#include "dbfilegaps.hpp"
#include "dbindex.hpp"
#include "dbstorage.hpp"

#include "parameter_pack/parameter_pack_helpers.hpp"

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
public:
	Collection(const std::string& collectionName, const std::string& databaseFolderPath) :
		_dbStoragePath{databaseFolderPath},
		_collectionName{collectionName}
	{
		_storageFile.setFileName(qStrFromStdStrU8(databaseFolderPath + '/' + collectionName));
		assert_r(_storageFile.open(QFile::ReadWrite));

		_index.load(indexStorageFolderPath());
	}

	~Collection()
	{
		_index.store(indexStorageFolderPath());
	}

	std::string indexStorageFolderPath() const
	{
		return _dbStoragePath + "/" + _collectionName + "_index/";
	}

	using Record = std::tuple<Fields...>;

	template <typename Functor, auto... queryFieldIds>
	void insert(Functor&& predicate, bool updateIfExists = false) {

	}

	// TODO: add default functor for one value (no filter)
	template <auto... queryFieldIds, typename Functor, typename... ValueTypes>
	std::vector<Record> find(Functor&& predicate, ValueTypes... values) {
		static_assert(((Index::template hasIndex<queryFieldIds>()) && ...), "Attempting to query on an un-indexed field!");
		static_assert(sizeof...(queryFieldIds) == sizeof...(ValueTypes));

		std::vector<Record> results;

		static_for<0, sizeof...(ValueTypes)>([](int i) {
			//auto v = pack::value_by_index<i>(values);
		});

		return results;
	}

private:
	const std::string _dbStoragePath;
	const std::string _collectionName;

	Index _index;
	QFile _storageFile;
};
