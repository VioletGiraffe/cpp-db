#include "3rdparty/catch2/catch.hpp"
#include "dbwal.hpp"
#include "storage/storage_qt.hpp"

TEST_CASE("DbWAL basics", "[dbwal]")
{
	using F64 = Field<uint64_t, 1>;
	using F16 = Field<int16_t, 2>;
	using Record = DbRecord<Tombstone <F16, uint16_t{ 0xDEAD }> , F64, F16>;

	DbWAL<Record, io::QFileAdapter> wal;
	REQUIRE(wal.openLogFile("wal.dat"));
}