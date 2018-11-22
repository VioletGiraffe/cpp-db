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
		_dbStoragePath{databaseFolderPath}
	{
		_storageFile.setFileName(qStrFromStdStrU8(databaseFolderPath + '/' + collectionName));
		assert_r(_storageFile.open(QFile::ReadWrite));

		_index.load(databaseFolderPath);
	}

	~Collection()
	{
		_index.store(_dbStoragePath);
	}

	using Record = std::tuple<Fields...>;

	template <auto N>
	using FieldTypeById = typename std::tuple_element<static_cast<size_t>(N), Record>::type::ValueType;

	void insert(const Record& value) {

	}

	template <auto... queryFields>
	void insert(const Record& value) {

	}

	template <auto fieldTypeEnumValue>
	std::vector<Record> find(const FieldTypeById<fieldTypeEnumValue>& queryValue) {
		return {};
	}

private:
	const std::string _dbStoragePath;

	Index _index;
	QFile _storageFile;
};
