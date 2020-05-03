#include "3rdparty/catch2/catch.hpp"
#include "dbstorage.hpp"
#include "storage/storage_qt.hpp"
#include "random/randomnumbergenerator.h"

#include <vector>

TEST_CASE("DbStorage - basic functionality", "[dbstorage]") {
	try {
		using F64 = Field<uint64_t, 1>;
		using F16 = Field<int16_t, 2>;
		using Record = DbRecord<NoTombstone, F64, F16>;
		DBStorage<io::QMemoryDeviceAdapter, Record> storage;

		using RNG64 = RNG<uint64_t>;
		using RNG16s = RNG<int16_t>;

		storage.openStorageFile({});

		constexpr size_t N = 10000;

		std::vector<Record> reference;
		reference.reserve(N);

		StorageLocation offset{ 0 };
		REQUIRE(offset == 0);
		static_assert(Record::staticFieldsSize() == 10);
		static_assert(Record::allFieldsHaveStaticSize());
		REQUIRE(Record{}.totalSize() == Record::staticFieldsSize());

		for (size_t i = 0; i < N; ++i)
		{
			const auto& newRecord = reference.emplace_back(RNG64::next(), RNG16s::next());
			REQUIRE(storage.writeRecord(newRecord, offset));

			offset.location += newRecord.totalSize();
		}

		offset.location = 0;

		for (size_t i = 0; i < N; ++i)
		{
			Record r;
			REQUIRE(storage.readRecord(r, offset.location));
			REQUIRE(r == reference[i]);

			offset.location += r.totalSize();
		}
	}
	catch (...) {
		FAIL();
	}
}
