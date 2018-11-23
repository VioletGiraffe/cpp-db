#pragma once
#include "dbfield.hpp"
#include "dbfilegaps.hpp"
#include "dbindex.hpp"
#include "dbstorage.hpp"

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

	template <auto fieldId>
	std::vector<Record> find(const FieldTypeById_t<fieldId, Fields...>& queryValue) {
		return {};
	}

private:
	const std::string _dbStoragePath;
	const std::string _collectionName;

	Index _index;
	QFile _storageFile;
};
