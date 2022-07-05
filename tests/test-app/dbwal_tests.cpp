#include "3rdparty/catch2/catch.hpp"
#include "dbwal.hpp"
#include "storage/storage_static_buffer.hpp"

TEST_CASE("DbWAL basics", "[dbwal]")
{
	using F64 = Field<uint64_t, 1>;
	using F16 = Field<int16_t, 2>;
	using Tomb16 = Tombstone < F16, uint16_t{ 0xDEAD } > ;

	using FArray = Field<uint32_t, 3, true>;
	using RecordWithArray = DbRecord<Tomb16, F16, FArray>;

	DbWAL<RecordWithArray, io::StaticBufferAdapter<4096>> wal;
	REQUIRE(wal.openLogFile({}));
	REQUIRE(wal.clearLog());

	size_t unfinishedOpsCount = 0;
	REQUIRE(wal.verifyLog([&](auto&& /*op*/) {
		++unfinishedOpsCount;
	}));
	REQUIRE(unfinishedOpsCount == 0);

	Operation::AppendToArray<RecordWithArray, F16, FArray, false> opAppend(uint16_t{0}, std::vector{uint32_t{42}});
	REQUIRE(wal.registerOperation(0 /* op ID */, opAppend));

	REQUIRE(wal.closeLogFile());

	REQUIRE(wal.openLogFile({}));

	unfinishedOpsCount = 0;
	REQUIRE(wal.verifyLog([&](auto&& op) {
		++unfinishedOpsCount;

		using OpType = std::remove_cvref_t<decltype(op)>;
		constexpr bool sameType = std::is_same_v<decltype(opAppend), OpType>;
		REQUIRE(sameType);
		if constexpr (sameType)
		{
			REQUIRE(opAppend.array == op.array);
			REQUIRE(opAppend.keyValue == op.keyValue);
			REQUIRE(opAppend.updatedArray() == op.updatedArray());
			REQUIRE(opAppend.insertIfNotPresent() == op.insertIfNotPresent());
		}
	}));

	// One operation, because we never indicated that it has completed
	REQUIRE(unfinishedOpsCount == 1);
	REQUIRE(wal.closeLogFile());
}
