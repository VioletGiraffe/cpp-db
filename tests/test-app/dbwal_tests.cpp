#include "3rdparty/catch2/catch.hpp"
#include "dbwal.hpp"
#include "storage/storage_static_buffer.hpp"

TEST_CASE("DbWAL basics and normal operation", "[dbwal]")
{
	using F64 = Field<uint64_t, 1>;
	using F16 = Field<int16_t, 2>;

	using FArray = Field<uint32_t, 3, true>;
	using RecordWithArray = DbRecord<F16, FArray>;

	io::StaticBufferAdapter<4096> buffer;
	DbWAL<RecordWithArray, decltype(buffer)> wal{ buffer };
	REQUIRE(wal.openLogFile({}));
	REQUIRE(wal.clearLog());

	size_t unfinishedOpsCount = 0;
	REQUIRE(wal.verifyLog([&](auto&& /*op*/) {
		++unfinishedOpsCount;
	}));
	REQUIRE(unfinishedOpsCount == 0);

	Operation::AppendToArray<RecordWithArray, F16, FArray, false> opAppend(uint16_t{0}, std::vector{uint32_t{42}});
	constexpr auto opID = 0xFFFFFFFF123456;
	REQUIRE(wal.registerOperation(opAppend));

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
			// TODO: this crashes intellisense
			//REQUIRE(opAppend.insertIfNotPresent() == op.insertIfNotPresent());
		}
	}));

	// One operation, because we never indicated that it has completed
	REQUIRE(unfinishedOpsCount == 1);

#ifndef _DEBUG
	REQUIRE(wal.updateOpStatus(0, OpStatus::Successful) == false); // No such ID
#endif
	REQUIRE(wal.updateOpStatus(opID, OpStatus::Successful)); // No such ID
#ifndef _DEBUG
	REQUIRE(wal.updateOpStatus(opID, OpStatus::Successful) == false); // No longer pending!
#endif

	REQUIRE(wal.clearLog());

	unfinishedOpsCount = 0;
	REQUIRE(wal.verifyLog([&](auto&& /*op*/) {
		++unfinishedOpsCount;
	}));
	REQUIRE(unfinishedOpsCount == 0);

	REQUIRE(wal.closeLogFile());
}

/* Entry structure :

   -------------------------------------------------------------------------------------------------
  |            Field                |           Size              |        Offset from start        |
  |---------------------------------|-----------------------------|---------------------------------|
  | Complete entry SIZE (inc. hash) | sizeof(EntrySizeType) bytes |                         0 bytes |
  | Operation STATUS                |                     1 byte  | sizeof(EntrySizeType)     bytes |
  | Serialized operation DATA       |         var. length         | sizeof(EntrySizeType) + 1 bytes |
  | HASH of the whole entry data    |      sizeof(HashType) bytes | [SIZE] - sizeof(HashType) bytes |
   -------------------------------------------------------------------------------------------------

   NOTE: hash is always calculated with STATUS == PENDING. It's not updated when the final status is patched in.

*/

//TEST_CASE("DbWAL - verifying storage format", "[dbwal]")
//{
//	using F64 = Field<uint64_t, 1>;
//	using F16 = Field<int16_t, 2>;
//
//	using FArray = Field<uint32_t, 3, true>;
//	using RecordWithArray = DbRecord<F16, FArray>;
//
//	DbWAL<RecordWithArray, io::StaticBufferAdapter<4096>> wal;
//	REQUIRE(wal.openLogFile({}));
//	REQUIRE(wal.clearLog());
//
//	Operation::AppendToArray<RecordWithArray, F16, FArray, false> opAppend(uint16_t{ 0 }, std::vector{ uint32_t{42} });
//	constexpr auto opID = 0xFFFFFFFF123456;
//	REQUIRE(wal.registerOperation(opID, opAppend));
//
//	REQUIRE(wal.closeLogFile());
//
//	wal.
//}
