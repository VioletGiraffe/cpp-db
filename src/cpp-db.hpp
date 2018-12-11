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
	static constexpr bool checkFieldOrder()
	{
		if constexpr (sizeof...(Fields) == 1)
			return true;

		bool success = true;
		static_for<1, sizeof...(Fields)>([&success](auto i) {
			using type0 = pack::type_by_index<decltype(i)::value - 1, Fields...>;
			using type1 = pack::type_by_index<decltype(i)::value, Fields...>;

			success = success && !(type0::hasStaticSize() == false && type1::hasStaticSize() == true);
		});

		return success;
	}

	static_assert(checkFieldOrder(), "All the fields with static size must be listed before all the fields with runtime size!");

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
	template <auto queryFieldId>
	std::vector<Record> find(const FieldTypeById_t<queryFieldId, Fields...>& value) {
		static_assert(Index::template hasIndex<queryFieldId>(), "Attempting to query on an un-indexed field!");

		std::vector<Record> results;

		const auto locations = _index.template find<queryFieldId>(value);

		return results;
	}

private:
	const std::string _dbStoragePath;
	const std::string _collectionName;

	Index _index;
	QFile _storageFile;
};
