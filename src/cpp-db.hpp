#pragma once
#include "dbfield.hpp"
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
	static constexpr bool checkFieldOrder()
	{
		if constexpr (sizeof...(Fields) == 1)
			return true;

		bool fixedSizeFieldsBeforeDynamicFields = true;
		static_for<1, sizeof...(Fields)>([&fixedSizeFieldsBeforeDynamicFields](auto i) {
			using Type0 = pack::type_by_index<decltype(i)::value - 1, Fields...>;
			using Type1 = pack::type_by_index<decltype(i)::value, Fields...>;

			fixedSizeFieldsBeforeDynamicFields = fixedSizeFieldsBeforeDynamicFields && !(Type0::hasStaticSize() == false && Type1::hasStaticSize() == true);
		});

		return fixedSizeFieldsBeforeDynamicFields;
	}

	static constexpr size_t dynamicFieldCount()
	{
		size_t dynamicFieldCount = 0;
		static_for<0, sizeof...(Fields)>([&dynamicFieldCount](auto i) {
			using FieldType = pack::type_by_index<decltype(i)::value, Fields...>;
			if constexpr (FieldType::hasStaticSize() == false)
				++dynamicFieldCount;
		});

		return dynamicFieldCount <= 1;
	}

	static_assert(checkFieldOrder(), "All the fields with static size must be listed before all the fields with runtime size!");
	static_assert(dynamicFieldCount() <= 1, "No more than one dynamic field is allowed!");

public:
	using Record = std::tuple<Fields...>;

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

	template <typename Functor, auto... queryFieldIds>
	void insert(Functor&& predicate, bool updateIfExists = false)
	{

	}

	// TODO: add default functor for one value (no filter)
	template <auto queryFieldId>
	std::vector<Record> find(const FieldTypeById_t<queryFieldId, Fields...>& value) {
		static_assert(Index::template hasIndex<queryFieldId>(), "Attempting to query on an un-indexed field!");

		std::vector<Record> results;

		const auto locations = _index.template find<queryFieldId>(value);

		return results;
	}

private:
	Record readRecord(uint64_t recordStartLocation)
	{
		assert_r(_storageFile.seek(recordStartLocation));
		if constexpr (dynamicFieldCount() == 0)
		{

		}
		else
		{

		}
	}

private:
	const std::string _dbStoragePath;
	const std::string _collectionName;

	Index _index;
	QFile _storageFile;
};
