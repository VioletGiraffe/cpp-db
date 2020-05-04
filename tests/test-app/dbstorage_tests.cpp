#include "3rdparty/catch2/catch.hpp"
#include "dbstorage.hpp"
#include "storage/storage_qt.hpp"
#include "random/randomnumbergenerator.h"

#include <vector>

static std::string randomString(size_t n)
{
	std::string str;
	str.reserve(n);
	for (size_t i = 0; i < n; ++i)
		str.push_back(static_cast<char>(RandomChar::next()));

	return str;
}

TEST_CASE("DbStorage - basic functionality, static record size", "[dbstorage]") {
	try {
		using F64 = Field<uint64_t, 1>;
		using F16 = Field<int16_t, 2>;
		using Record = DbRecord<NoTombstone, F64, F16>;
		DBStorage<io::QMemoryDeviceAdapter, Record> storage;

		using RNG64 = RNG<uint64_t>;
		using RNG16s = RNG<int16_t>;

		storage.openStorageFile({});

		constexpr size_t N = 100000;

		std::vector<Record> reference;
		reference.reserve(N);

		StorageLocation offset{ 0 };
		REQUIRE(offset == 0);
		static_assert(Record::staticFieldsSize() == 10);
		static_assert(Record::allFieldsHaveStaticSize());
		REQUIRE(Record{}.totalSize() == Record::staticFieldsSize());

		bool success = true;
		for (size_t i = 0; i < N; ++i)
		{
			const auto& newRecord = reference.emplace_back(RNG64::next(), RNG16s::next());
			if (!storage.writeRecord(newRecord, offset))
			{
				success = false;
				break;
			}

			offset.location += newRecord.totalSize();
		}

		REQUIRE(success);
		offset.location = 0;

		for (size_t i = 0; i < N; ++i)
		{
			Record r;
			if (!storage.readRecord(r, offset) || !(r == reference[i]))
			{
				success = false;
				break;
			}

			offset.location += r.totalSize();
		}

		REQUIRE(success);
	}
	catch (...) {
		FAIL();
	}
}

TEST_CASE("DbStorage - basic functionality, dynamic record sizes", "[dbstorage]") {
	try {
		using FS1 = Field<std::string, 1>;
		using FS2 = Field<std::string, 2>;
		using Record = DbRecord<NoTombstone, FS1, FS2>;
		DBStorage<io::QMemoryDeviceAdapter, Record> storage;

		storage.openStorageFile({});

		constexpr size_t N = 10000;

		std::vector<Record> reference;
		reference.reserve(N);

		StorageLocation offset{ 0 };
		REQUIRE(offset == 0);

		bool success = true;
		for (size_t i = 0; i < N; ++i)
		{
			const auto& newRecord = reference.emplace_back(randomString(3), randomString(8));
			if (!storage.writeRecord(newRecord, offset))
			{
				success = false;
				break;
			}

			offset.location += newRecord.totalSize();
		}

		REQUIRE(success);
		offset.location = 0;

		for (size_t i = 0; i < N; ++i)
		{
			Record r;
			if (!storage.readRecord(r, offset) || !(r == reference[i]))
			{
				success = false;
				break;
			}

			offset.location += r.totalSize();
		}

		REQUIRE(success);
	}
	catch (...) {
		FAIL();
	}
}

