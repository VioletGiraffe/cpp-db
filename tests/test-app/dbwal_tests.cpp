#include "3rdparty/catch2/catch.hpp"
#include "dbwal.hpp"
#include "storage/storage_static_buffer.hpp"

#include <array>

#ifdef _WIN32
#include <crtdbg.h>

#define IGNORE_ASSERTION(...) do { [[maybe_unused]] const auto prevMode = _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_DEBUG); __VA_ARGS__; _CrtSetReportMode(_CRT_ASSERT, prevMode); } while(0)
#else
#define IGNORE_ASSERTION(...) __VA_ARGS__
#endif

TEST_CASE("DbWAL basics", "[dbwal]")
{
	using F16 = Field<int16_t, 2>;

	using FArray = Field<uint32_t, 3, true>;
	using RecordWithArray = DbRecord<F16, FArray>;

	io::StaticBufferAdapter<8192> buffer;
	DbWAL<RecordWithArray, decltype(buffer)> wal{ buffer };
	REQUIRE(wal.openLogFile({}));
	REQUIRE(buffer.size() == 0);

	REQUIRE(wal.clearLog());
	REQUIRE(buffer.size() == 0);

	size_t unfinishedOpsCount = 0;
	REQUIRE(wal.verifyLog([&](auto&& /*op*/) {
		++unfinishedOpsCount;
	}));
	REQUIRE(unfinishedOpsCount == 0);
	REQUIRE(buffer.size() == 0);

	Operation::AppendToArray<RecordWithArray, F16, FArray, false> opAppend(uint16_t{0}, std::vector{uint32_t{42}});
	REQUIRE(wal.registerOperation(opAppend));
	const auto bufferSize = buffer.size();
	REQUIRE(bufferSize > 0);

	REQUIRE(wal.closeLogFile());
	REQUIRE(buffer.size() > 0);

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

	REQUIRE(buffer.size() == bufferSize);

	// One operation, because we never indicated that it has completed
	REQUIRE(unfinishedOpsCount == 1);

	REQUIRE(wal.clearLog());
	REQUIRE(buffer.size() == 0);

	unfinishedOpsCount = 0;
	REQUIRE(wal.verifyLog([&](auto&& /*op*/) {
		++unfinishedOpsCount;
	}));
	REQUIRE(unfinishedOpsCount == 0);

	// Test updating op status
	const auto id = wal.registerOperation(opAppend);
	REQUIRE(id);
	REQUIRE(buffer.pos() == 4096);

	// Rewind for verification without closing the log
	buffer.seek(0);
	REQUIRE(wal.verifyLog([&](auto&& /*op*/) {
		++unfinishedOpsCount;
	}));
 	REQUIRE(unfinishedOpsCount == 1);

	buffer.seek(4096);
	REQUIRE(wal.updateOpStatus(*id, WAL::OpStatus::Successful));
	IGNORE_ASSERTION(REQUIRE(wal.updateOpStatus(*id + 1, WAL::OpStatus::Successful) == false)); // No such ID
	unfinishedOpsCount = 0;
	buffer.seek(0);
	REQUIRE(wal.verifyLog([&](auto&& /*op*/) {
		++unfinishedOpsCount;
	}));
	REQUIRE(unfinishedOpsCount == 0);

	buffer.clear();
	IGNORE_ASSERTION(REQUIRE(wal.updateOpStatus(*id, WAL::OpStatus::Successful) == false)); // No longer pending

	// Closing the log
	REQUIRE(wal.closeLogFile());
	REQUIRE(buffer.size() == 4096);
}

TEST_CASE("DbWAL: registering multiple operations - single thread", "[dbwal]")
{
	try {
		using F64 = Field<uint64_t, 1>;
		using F16 = Field<int16_t, 2>;
		using FString = Field<std::string, 3 >;
		using FArray = Field<uint32_t, 4, true>;

		using RecordWithArray = DbRecord<F64, F16, FString, FArray>;
		RecordWithArray r1(1'000'000'000'000ULL, int16_t{ -32700 }, "Hello!", std::vector<uint32_t>{ {123, 456, 789} });
		RecordWithArray r2(1'111'111'111'000ULL, int16_t{ -27000 }, "World!", std::vector<uint32_t>{ {990, 1990, 19990} });

		Operation::Insert<RecordWithArray> opInsert(r1);
		Operation::AppendToArray<RecordWithArray, F16, FArray, true> opAppend(int16_t{ -31000 }, r2);
		Operation::Delete<RecordWithArray, FString> opDelete("Mars");
		Operation::UpdateFull<RecordWithArray, F64, false> opUpdate(r1, 1'111'111'111'000ULL);
		Operation::Find<RecordWithArray, F16, FString> opFind(int16_t{ -21999 }, "Venus");

		{
			io::StaticBufferAdapter<150'000> walDataBuffer;
			DbWAL<RecordWithArray, decltype(walDataBuffer)> wal{ walDataBuffer };
			REQUIRE(wal.openLogFile({}));

			std::vector<WAL::OperationIdType> registeredOpIds;
			registeredOpIds.push_back(wal.registerOperation(opInsert).value());
			registeredOpIds.push_back(wal.registerOperation(opAppend).value());
			registeredOpIds.push_back(wal.registerOperation(opDelete).value());
			registeredOpIds.push_back(wal.registerOperation(opUpdate).value());
			registeredOpIds.push_back(wal.registerOperation(opFind).value());

			REQUIRE(wal.closeLogFile());
			const auto size = walDataBuffer.size();
			REQUIRE(size > 0);

			REQUIRE(wal.openLogFile({}));

			std::array<size_t, 6> unfinishedOpsCount;
			unfinishedOpsCount.fill(0);

			const bool verificationSuccessful = wal.verifyLog([&](auto&& op) {
				using OpType = std::remove_cvref_t<decltype(op)>;
				static constexpr bool isCompletionmarker = std::is_same_v<OpType, WAL::OperationCompletedMarker>;
				static constexpr size_t indexForOpType = []() {
					if constexpr (isCompletionmarker)
						return 6 - 1;
					else
						return (size_t)OpType::op;
				}();

				++unfinishedOpsCount[indexForOpType];

				if constexpr (isCompletionmarker)
				{
				}
				else if constexpr (std::is_same_v<decltype(opAppend), OpType>)
				{
					REQUIRE(opAppend.keyValue == op.keyValue);
					REQUIRE(opAppend.updatedArray() == op.updatedArray());
					REQUIRE(op.updatedArray() == r2.fieldValue<FArray>());
					// Still crashes intellisense! VS 17.4.3
					//REQUIRE(opAppend.insertIfNotPresent() == op.insertIfNotPresent());
				}
				else if constexpr (std::is_same_v<decltype(opInsert), OpType>)
				{
					REQUIRE(op._record == r1);
				}
				else if constexpr (std::is_same_v<decltype(opDelete), OpType>)
				{
					REQUIRE(opDelete.keyValue == op.keyValue);
				}
				else if constexpr (std::is_same_v<decltype(opUpdate), OpType>)
				{
					REQUIRE(opUpdate.keyValue == op.keyValue);
					REQUIRE(opUpdate.record == r1);
				}
				else if constexpr (std::is_same_v<decltype(opFind), OpType>)
				{
					REQUIRE(opFind._fields == op._fields);
				}
				else
					FAIL();
			});

			REQUIRE(verificationSuccessful);
			REQUIRE(std::accumulate(begin_to_end(unfinishedOpsCount), 0_z) == 5);
			REQUIRE(std::all_of(begin_to_end(unfinishedOpsCount), [](auto count) { return count <= 1; }));
		}
	}
	catch (const std::exception& e) {
		FAIL(e.what());
	}
}

/* Entry structure :

   -----------------------------------------------------------------------------------------------------------------
  |            Field                                |           Size              |        Offset from start        |
  |-------------------------------------------------|-----------------------------|---------------------------------|
  | Complete entry SIZE (2 bytes max for 4K blocks) | sizeof(EntrySizeType) bytes |                         0 bytes |
  | Operation ID                                    | 4 bytes                     |     sizeof(EntrySizeType) bytes |
  | Serialized operation DATA                       | var. length                 | sizeof(EntrySizeType) + 4 bytes |
   -----------------------------------------------------------------------------------------------------------------


   Block format:

  - Number of entries (2 bytes for 4K block size)
  - Entry 1
  - ...
  - Entry N
  - Checksum over the whole block (4 bytes)
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
