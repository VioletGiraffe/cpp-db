#include "3rdparty/catch2/catch.hpp"
#include "dbwal.hpp"
#include "storage/storage_qt.hpp"

TEST_CASE("DbWAL basics", "[dbwal]")
{
	using F64 = Field<uint64_t, 1>;
	using F16 = Field<int16_t, 2>;
	using Tomb16 = Tombstone < F16, uint16_t{ 0xDEAD } > ;
	using Record = DbRecord<Tomb16, F64, F16>;

	using FArray = Field<uint32_t, 3, true>;
	using RecordWithArray = DbRecord<Tomb16, F16, FArray>;

	DbWAL<Record, io::QFileAdapter> wal;
	REQUIRE(wal.openLogFile("wal.dat"));
	REQUIRE(wal.clearLog());

	size_t brokenOpsCount = 0;
	REQUIRE(wal.verifyLog([&](auto&& /*op*/) {
		++brokenOpsCount;
	}));
	REQUIRE(brokenOpsCount == 0);

	Operation::AppendToArray<RecordWithArray, F16, FArray, false> opAppend(uint16_t{0}, std::vector{uint32_t{42}});
	REQUIRE(wal.registerOperation(0 /* op ID */, opAppend));

	REQUIRE(wal.closeLogFile());

	REQUIRE(wal.openLogFile("wal.dat"));

	brokenOpsCount = 0;
	REQUIRE(wal.verifyLog([&](auto&& /*op*/) {
		++brokenOpsCount;
	}));
	REQUIRE(brokenOpsCount == 0);
	REQUIRE(wal.closeLogFile());
}
