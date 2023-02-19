#include "3rdparty/catch2/catch.hpp"
#include "dbwal.hpp"
#include "storage/storage_static_buffer.hpp"

#include "threading/thread_helpers.h"

#include "random/randomnumbergenerator.h"
#include "utility/integer_literals.hpp"

#include <array>
#include <deque>

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
	const auto bufferChecksum = wheathash64(buffer.data(), buffer.size());
	IGNORE_ASSERTION(REQUIRE(wal.updateOpStatus(*id + 1, WAL::OpStatus::Successful) == false)); // No such ID
	unfinishedOpsCount = 0;
	buffer.seek(0);
	REQUIRE(wal.verifyLog([&](auto&& /*op*/) {
		++unfinishedOpsCount;
	}));
	REQUIRE(unfinishedOpsCount == 0);

	IGNORE_ASSERTION(REQUIRE(wal.updateOpStatus(*id, WAL::OpStatus::Successful) == false)); // No longer pending

	const auto newChecksum = wheathash64(buffer.data(), buffer.size());
	REQUIRE(newChecksum == bufferChecksum);

	// Closing the log
	REQUIRE(wal.closeLogFile());
	REQUIRE(buffer.size() == 8192);
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

		io::StaticBufferAdapter<80'000> walDataBuffer;
		DbWAL<RecordWithArray, decltype(walDataBuffer)> wal{ walDataBuffer };
		REQUIRE(wal.openLogFile({}));

		std::vector<WAL::OpID> registeredOpIds;
		registeredOpIds.push_back(wal.registerOperation(opInsert).value());
		registeredOpIds.push_back(wal.registerOperation(opAppend).value());
		registeredOpIds.push_back(wal.registerOperation(opDelete).value());
		registeredOpIds.push_back(wal.registerOperation(opUpdate).value());
		registeredOpIds.push_back(wal.registerOperation(opFind).value());

		std::array<size_t, 5> unfinishedOpsCount;
		unfinishedOpsCount.fill(0);

		SECTION("No updateOpStatus()")
		{
			REQUIRE(wal.closeLogFile());
			const auto size = walDataBuffer.size();
			REQUIRE(size > 0);

			REQUIRE(wal.openLogFile({}));

			const bool verificationSuccessful = wal.verifyLog(overload{
				[&](decltype(opAppend) && op) {
					++unfinishedOpsCount[0];
					REQUIRE(opAppend.keyValue == op.keyValue);
					REQUIRE(opAppend.updatedArray() == op.updatedArray());
					REQUIRE(op.updatedArray() == r2.fieldValue<FArray>());
					// Still crashes intellisense! VS 17.4.3
					//REQUIRE(opAppend.insertIfNotPresent() == op.insertIfNotPresent());
				},
				[&](decltype(opInsert) && op) {
					++unfinishedOpsCount[1];
					REQUIRE(op._record == r1);
				},
				[&](decltype(opDelete) && op) {
					++unfinishedOpsCount[2];
					REQUIRE(opDelete.keyValue == op.keyValue);
				},
				[&](decltype(opUpdate) && op) {
					++unfinishedOpsCount[3];
					REQUIRE(opUpdate.keyValue == op.keyValue);
					REQUIRE(opUpdate.record == r1);
				},
				[&](decltype(opFind) && op) {
					++unfinishedOpsCount[4];
					REQUIRE(opFind._fields == op._fields);
				},
				[&](auto&&) {
					FAIL("This overload shouldn't be called!");
				}
				});

			REQUIRE(verificationSuccessful);
			REQUIRE(std::accumulate(begin_to_end(unfinishedOpsCount), 0_z) == 5);
			REQUIRE(std::all_of(begin_to_end(unfinishedOpsCount), [](auto count) { return count == 1; }));
		}

		SECTION("With updateOpStatus()")
		{
			REQUIRE(wal.updateOpStatus(registeredOpIds[1], WAL::OpStatus::Failed));
			REQUIRE(wal.updateOpStatus(registeredOpIds[4], WAL::OpStatus::Successful));

			REQUIRE(wal.closeLogFile());
			const auto size = walDataBuffer.size();
			REQUIRE(size > 0);

			REQUIRE(wal.openLogFile({}));

			const bool verificationSuccessful = wal.verifyLog(overload{
				[&](decltype(opInsert) && op) {
					++unfinishedOpsCount[1];
					REQUIRE(op._record == r1);
				},
				[&](decltype(opDelete) && op) {
					++unfinishedOpsCount[2];
					REQUIRE(opDelete.keyValue == op.keyValue);
				},
				[&](decltype(opUpdate) && op) {
					++unfinishedOpsCount[3];
					REQUIRE(opUpdate.keyValue == op.keyValue);
					REQUIRE(opUpdate.record == r1);
				},
				[&](auto&&) {
					FAIL("This overload shouldn't be called!");
				}
				});

			REQUIRE(verificationSuccessful);
			REQUIRE(std::accumulate(begin_to_end(unfinishedOpsCount), 0_z) == 5 - 2);
			REQUIRE(std::all_of(begin_to_end(unfinishedOpsCount), [](auto count) { return count <= 1; }));
		}
	}
	catch (const std::exception& e) {
		FAIL(e.what());
	}
}

TEST_CASE("DbWAL: registering multiple operations - multiple threads", "[dbwal]")
{
	try {
		using F64 = Field<uint64_t, 1>;
		using F16 = Field<int16_t, 2>;
		using FString = Field<std::string, 3 >;
		using FArray = Field<uint64_t, 4, true>;

		using RecordWithArray = DbRecord<F64, F16, FString, FArray>;
		RecordWithArray r1(1'000'000'000'000ULL, int16_t{ -32700 }, "Hello!", std::vector<uint64_t>(100, 123));
		RecordWithArray r2(1'111'111'111'000ULL, int16_t{ -27000 }, "World!", std::vector<uint64_t>(30, 456));

		Operation::Insert<RecordWithArray> opInsert(r1);
		Operation::AppendToArray<RecordWithArray, F16, FArray, true> opAppend(int16_t{ -31000 }, r2);
		Operation::Delete<RecordWithArray, FString> opDelete("Mars");
		Operation::UpdateFull<RecordWithArray, F64, false> opUpdate(r1, 1'111'111'111'000ULL);
		Operation::Find<RecordWithArray, F16, FString> opFind(int16_t{ -21999 }, "Venus");

		io::VectorAdapter walDataBuffer(100000);
		DbWAL<RecordWithArray, decltype(walDataBuffer)> wal{ walDataBuffer };
		REQUIRE(wal.openLogFile({}));

		const auto registerOperationByIndex = [&](const size_t index) {
			switch (index) {
			case 0:
				return wal.registerOperation(opInsert);
			case 1:
				return wal.registerOperation(opAppend);
			case 2:
				return wal.registerOperation(opDelete);
			case 3:
				return wal.registerOperation(opUpdate);
			case 4:
				return wal.registerOperation(opFind);
			default:
				throw std::runtime_error("!!! RNG failed !!!");
			}
		};

		// Each thread gets a different, but deterministic seed
		std::atomic_uint64_t seed = 13419494910660666967ULL;

		static constexpr size_t NOperationsPerThread = 500;
		static constexpr size_t NThreads = 12;

		const auto threadFunction = [&](const bool issueUpdateOpStatusCalls) {
			std::vector<WAL::OpID> opIds;
			opIds.reserve(NOperationsPerThread);
			const auto localSeed = seed++;
			RandomNumberGenerator<uint64_t> rng{ localSeed, 0, issueUpdateOpStatusCalls ? 5_z : 4_z };
			RandomNumberGenerator<uint64_t> rngForOpId{ localSeed + 1 };

			for (size_t i = 0; i < NOperationsPerThread; ++i)
			{
				const auto index = rng.rand();
				if (index < 5)
				{
					const auto id = registerOperationByIndex(index);
					REQUIRE(id);
					opIds.push_back(*id);
				}
				else if (!opIds.empty())
				{
					const auto it = opIds.begin() + rngForOpId.rand() % opIds.size();
					const auto id = *it;
					const auto random = wheathash64v(rdtsc());
					const auto status = (random & 1) != 0 ? WAL::OpStatus::Successful : WAL::OpStatus::Failed;
					REQUIRE(wal.updateOpStatus(id, status));
					opIds.erase(it);
				}
				else
				{
					const auto id = registerOperationByIndex(1);
					REQUIRE(id);
					opIds.push_back(*id);
				}
			}
		};

		const auto fillWal = [&threadFunction](bool issueUpdateOpStatusCalls) {
			const auto startTime = timeElapsedMs();
			std::deque<std::thread> threads;
			// Start N threads
			for (size_t i = 0; i < NThreads; ++i)
				threads.emplace_back(threadFunction, issueUpdateOpStatusCalls);

			joinAll(threads);

			if (issueUpdateOpStatusCalls)
				printf("Filling the WAL with updateOpStatus() (%lu threads x %lu ops) took %F s\n", (unsigned long)NThreads, (unsigned long)NOperationsPerThread, (float)(timeElapsedMs() - startTime) / 1000.0f);
			else
				printf("Filling the WAL (%lu threads x %lu ops) took %F s\n", (unsigned long)NThreads, (unsigned long)NOperationsPerThread, (float)(timeElapsedMs() - startTime) / 1000.0f);
		};

		SECTION("No updateOpStatus()")
		{
			fillWal(false);

			std::array<size_t, 5> unfinishedOpsCount;
			unfinishedOpsCount.fill(0);

			REQUIRE(wal.closeLogFile());
			const auto size = walDataBuffer.size();
			REQUIRE(size > 0);

			REQUIRE(wal.openLogFile({}));

			const bool verificationSuccessful = wal.verifyLog(overload{
				[&](decltype(opAppend) && op) {
					++unfinishedOpsCount[0];
					REQUIRE(opAppend.keyValue == op.keyValue);
					REQUIRE(opAppend.updatedArray() == op.updatedArray());
					REQUIRE(op.updatedArray() == r2.fieldValue<FArray>());
					// Still crashes intellisense! VS 17.4.3
					//REQUIRE(opAppend.insertIfNotPresent() == op.insertIfNotPresent());
				},
				[&](decltype(opInsert) && op) {
					++unfinishedOpsCount[1];
					REQUIRE(op._record == r1);
				},
				[&](decltype(opDelete) && op) {
					++unfinishedOpsCount[2];
					REQUIRE(opDelete.keyValue == op.keyValue);
				},
				[&](decltype(opUpdate) && op) {
					++unfinishedOpsCount[3];
					REQUIRE(opUpdate.keyValue == op.keyValue);
					REQUIRE(opUpdate.record == r1);
				},
				[&](decltype(opFind) && op) {
					++unfinishedOpsCount[4];
					REQUIRE(opFind._fields == op._fields);
				},
				[&](auto&&) {
					FAIL("This overload shouldn't be called!");
				}
				});

			REQUIRE(verificationSuccessful);
			REQUIRE(std::accumulate(begin_to_end(unfinishedOpsCount), 0_z) == NThreads * NOperationsPerThread);
			REQUIRE(std::all_of(begin_to_end(unfinishedOpsCount), [](auto count) { return count > 0; }));
		}

		SECTION("With updateOpStatus()")
		{
			fillWal(true);

			std::array<size_t, 5> unfinishedOpsCount;
			unfinishedOpsCount.fill(0);

			REQUIRE(wal.closeLogFile());
			const auto size = walDataBuffer.size();
			REQUIRE(size > 0);

			REQUIRE(wal.openLogFile({}));

			const bool verificationSuccessful = wal.verifyLog(overload{
				[&](decltype(opAppend) && op) {
					++unfinishedOpsCount[0];
					REQUIRE(opAppend.keyValue == op.keyValue);
					REQUIRE(opAppend.updatedArray() == op.updatedArray());
					REQUIRE(op.updatedArray() == r2.fieldValue<FArray>());
					// Still crashes intellisense! VS 17.4.3
					//REQUIRE(opAppend.insertIfNotPresent() == op.insertIfNotPresent());
				},
				[&](decltype(opInsert) && op) {
					++unfinishedOpsCount[1];
					REQUIRE(op._record == r1);
				},
				[&](decltype(opDelete) && op) {
					++unfinishedOpsCount[2];
					REQUIRE(opDelete.keyValue == op.keyValue);
				},
				[&](decltype(opUpdate) && op) {
					++unfinishedOpsCount[3];
					REQUIRE(opUpdate.keyValue == op.keyValue);
					REQUIRE(opUpdate.record == r1);
				},
				[&](decltype(opFind) && op) {
					++unfinishedOpsCount[4];
					REQUIRE(opFind._fields == op._fields);
				},
				[&](WAL::OperationCompletedMarker&& /*marker*/) {
					// Neither successful nor failed operations should be reported, as long as they are completed!
					FAIL("This branch shouldn't be called!");
				},
				[&](auto&&) {
					FAIL("This overload shouldn't be called!");
				}
				});

			REQUIRE(verificationSuccessful);
			const auto unfinished = std::accumulate(begin_to_end(unfinishedOpsCount), 0_z);
			// The number of completion markers in the log is approximately 1/6th of the total number of entries: there are 5 operations + the completion markers, uniformly distributed.
			// So the total number of operations logged is 5/6 * NThreads * NOperationsPerThread, of which 1/6th are marked completed, so 4/6th remains.
			REQUIRE((unfinished > NThreads * NOperationsPerThread * 4 / 7 && unfinished < NThreads * NOperationsPerThread * 4 / 5));
			REQUIRE(std::all_of(begin_to_end(unfinishedOpsCount), [](auto count) { return count > 0; }));
		}

	}
	catch (const std::exception& e) {
		FAIL(e.what());
	}

	printf("Max fill: %ld, avg. fill: %ld\n", (long)maxFill.load(), (long)totalSizeWritten.load() / (long)totalBlockCount.load());
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
