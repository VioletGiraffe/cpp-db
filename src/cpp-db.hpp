#pragma once

#include "assert/advanced_assert.h"

#include <QFile>

#include <string>
#include <tuple>
#include <vector>

enum class TestCollectionFields {
	Id,
	Text
};

template <typename T, auto fieldId>
struct Field {
	using ValueType = T;
	static constexpr auto id = fieldId;

	T value;
};

template <class... Fields>
class Collection
{
public:
	Collection(const std::string& collectionName, const std::string& databaseFilePath) {
		_dbFile.setFileName(QString::fromStdString(databaseFilePath + '/' + collectionName));
		assert_r(_dbFile.open(QFile::ReadWrite));
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
	QFile _dbFile;
};

