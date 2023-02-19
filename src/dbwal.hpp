#pragma once

/*

Operation basics:
* Disk I/O is done in blocks of 4 KiB.
* An internal 4 KiB buffer is kept for incoming operations. The buffer is flushed to disk when full or on timeout.
  'registerOperation()' call blocks until the flush occurs.

* Among the thread currently waiting for flush, one is designated responsible for timeout handling. Other wait passively.
  This one is the thread that first added data to the empty buffer after the previous flush cleared it.

* Only one thread, dubbed the owner thread, is supposed to create, open, verify and close the log. Any number of threads can actually log events.

* Unique operation IDs are generated by incrementing a counter such that the counter reflects the relative order in which threads acquired the buffer mutex.
  This way older requests always have lower IDs, so threads can easily track which operations were flushed and which were not.

* Verification of the log is done in two passes.
  The first pass only looks for operation completion markers and stores them.
  The second pass reads operation IDs of every record, but skips all the other data for IDs that have been successful.
  This eliminates the need to store operations until their status is known, which would be highly problematic due to unknown template arguments.
*/

#include "dbops.hpp"
#include "dbschema.hpp"
#include "storage/storage_io_interface.hpp"
#include "storage/storage_static_buffer.hpp"
#include "utils/dbutilities.hpp"
#include "WAL/wal_serializer.hpp"
#include "WAL/wal_data_types.hpp"
#include "utils/mutex_checked.hpp"

#include "container/std_container_helpers.hpp"
#include "hash/wheathash.hpp"
#include "system/timing.h"

#include <atomic>
#include <optional>
#include <thread>
#include <vector>

static std::atomic_uint64_t maxFill = 0;
static std::atomic_uint64_t totalBlockCount = 0;
static std::atomic_uint64_t totalSizeWritten = 0;

//#define ENABLE_TRACING

#ifdef ENABLE_TRACING
#ifdef _WIN32
#include <Windows.h>
#define get_tid ::GetCurrentThreadId
#else
#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>
#define get_tid(...) ::syscall(__NR_gettid)
#endif
#define TRACE(...) printf(__VA_ARGS__)
#else
#define TRACE(...)
#endif

template <RecordType Record, class StorageAdapter>
class DbWAL
{
public:
	constexpr DbWAL(StorageAdapter& walIoDevice) noexcept :
		_logFile{ walIoDevice }
	{
		checkBlockSize();
	}

	~DbWAL() noexcept;

	[[nodiscard]] bool openLogFile(const std::string& filePath) noexcept;
	[[nodiscard]] bool closeLogFile() noexcept;

	template <typename Receiver>
	[[nodiscard]] bool verifyLog(Receiver&& unfinishedOperationsReceiver) noexcept;
	[[nodiscard]] bool clearLog() noexcept;

	// Registers the new operation and returns its unique ID. Empty optional = failure to register.
	template <class OpType>
	[[nodiscard]] std::optional<WAL::OpID> registerOperation(OpType&& op) noexcept;
	[[nodiscard]] bool updateOpStatus(WAL::OpID opId, WAL::OpStatus status) noexcept;

private:
	constexpr void startNewBlock() noexcept;
	[[nodiscard]] constexpr bool finalizeAndflushCurrentBlock() noexcept;
	[[nodiscard]] constexpr bool newBlockRequiredForData(size_t dataSize) noexcept;

	[[nodiscard]] constexpr bool blockIsEmpty() const noexcept;

	// Waits for the record with ID == opId to get flushed.
	// If that doesn't happen within certain time, performs flush.
	void waitForFlushAndHandleTimeout(WAL::OpID opId, bool firstWriter, uint64_t operationStartTimeStamp) noexcept;

private:
	using EntrySizeType = uint16_t;
	using Serializer = WAL::Serializer<Record>;

	using BlockChecksumType = uint32_t;
	static_assert(std::is_same_v<BlockChecksumType, decltype(wheathash32(nullptr, 0))>);

	using BlockItemCountType = uint16_t;
	static constexpr size_t BlockSize = 4096;

	// These two definitions are for bug checking only
	static constexpr size_t MinItemSize = sizeof(EntrySizeType) + sizeof(WAL::OpID) + 1 /* assume at least one byte of payload */;
	static constexpr size_t MaxItemCount = BlockSize / MinItemSize;

private:
	checked_mutex _mtxBlock;
	// Has to be atomic for the wait loop to spin on it.
	std::atomic<WAL::OpID> _lastFlushedOpId = 0;
	// Doesn't have to be atomic - only accessed under mutex
	WAL::OpID _lastBlockOpId = 0;
	
	io::StaticBufferAdapter<BlockSize> _block;
	std::atomic<size_t> _blockItemCount = 0;

	// Solely for tracking how many ops are pending. When none, the log can be trimmed
	std::vector<WAL::OpID> _pendingOperations;
	size_t _operationsProcessed = 0;

	StorageIO<StorageAdapter> _logFile;
	WAL::OpID _lastOpId = 0; // Only accessed under mutex - doesn't have to be atomic

	const std::thread::id _ownerThreadId = std::this_thread::get_id();
};

template<RecordType Record, class StorageAdapter>
DbWAL<Record, StorageAdapter>::~DbWAL() noexcept
{
	std::lock_guard lock(_mtxBlock);
	assert_r(blockIsEmpty());
}

template<RecordType Record, class StorageAdapter>
[[nodiscard]] bool DbWAL<Record, StorageAdapter>::openLogFile(const std::string& filePath) noexcept
{
	assert_debug_only(std::this_thread::get_id() == _ownerThreadId);
	std::lock_guard lock(_mtxBlock);

	startNewBlock();
	return _logFile.open(filePath, io::OpenMode::Write);
}

template<RecordType Record, class StorageAdapter>
[[nodiscard]] bool DbWAL<Record, StorageAdapter>::closeLogFile() noexcept
{
	assert_debug_only(std::this_thread::get_id() == _ownerThreadId);
	std::lock_guard lock(_mtxBlock);

	if (!blockIsEmpty())
	{
		assert_and_return_r(finalizeAndflushCurrentBlock(), false);
		startNewBlock();
	}

	return _logFile.close();
}

template<RecordType Record, class StorageAdapter>
[[nodiscard]] bool DbWAL<Record, StorageAdapter>::clearLog() noexcept
{
	// TODO: why startNewBlock?
	//{
	//	std::lock_guard lock(_mtxBlock);
	//	startNewBlock();
	//}

	std::lock_guard lock(_mtxBlock);

	assert_and_return_r(_logFile.clear(), false);
	return _logFile.flush();
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

template<RecordType Record, class StorageAdapter>
template <typename Receiver>
[[nodiscard]] bool DbWAL<Record, StorageAdapter>::verifyLog(Receiver&& unfinishedOperationsReceiver) noexcept
{
	assert_debug_only(std::this_thread::get_id() == _ownerThreadId);
	std::lock_guard lock(_mtxBlock); // Should not be required, but just in case. Using at as a more of a global lock on the _logfile.

	assert_and_return_r(_logFile.pos() == 0, false);

	// This is the buffer for the whole WAL block
	io::StaticBufferAdapter<BlockSize> blockBuffer;
	blockBuffer.reserve(BlockSize);
	StorageIO blockBufferIo{ blockBuffer };

	assert_r(_logFile.size() % BlockSize == 0);

	std::vector<WAL::OpID> completedOperations;
	const auto isCompleted = [&completedOperations](const WAL::OpID id) {
		return std::find(completedOperations.begin(), completedOperations.end(), id) != completedOperations.end();
	};

	// Two passes are required because of the templated operation types that cannot be [easily] stored at runtime.
	// The first pass reads and stores all completed operations.
	// Then the seconds pass can skip over all transactions that have completed successfully and process the ones that didn't.

	// Remeber: malformed last block is not an error
	for (size_t pass = 1; pass <= 2; ++pass)
	{
		while (!_logFile.atEnd() && (_logFile.size() - _logFile.pos() >= BlockSize))
		{
			const bool isLastBlock = _logFile.size() - _logFile.pos() == BlockSize;
			if (!_logFile.read(blockBuffer.data(), BlockSize))
			{
				if (isLastBlock)
					continue; // Not an error
				else
					fatalAbort("Failed to read a WAL block that's not the last one!");
			}

			blockBuffer.seek(0);

			// Verify checksum before processing the block
			const auto checksum = memory_cast<BlockChecksumType>(blockBuffer.data() + BlockSize - sizeof(BlockChecksumType));

			const auto actualChecksum = wheathash32(blockBuffer.data(), BlockSize - sizeof(BlockChecksumType));
			if (checksum != actualChecksum)
			{
				if (isLastBlock)
					continue; // Not an error
				else
					fatalAbort("Failed to read a WAL block that's not the last one!");
			}

			BlockItemCountType itemCountInBlock = 0;
			assert_and_return_r(blockBufferIo.read(itemCountInBlock), false);
			assert_and_return_r(itemCountInBlock > 0 && itemCountInBlock <= MaxItemCount, false);

			for (size_t i = 0; i < itemCountInBlock; ++i)
			{
				const auto entryStartPos = blockBufferIo.pos();
				EntrySizeType wholeEntrySize = 0;
				assert_and_return_r(blockBufferIo.read(wholeEntrySize) && wholeEntrySize > 0, false);

				WAL::OpID operationId = 0;
				assert_and_return_r(blockBufferIo.read(operationId) && operationId != 0, false);

				// First pass: skip everything. Second pass: skip completed operations.
				const bool skipEntry = pass == 1 || isCompleted(operationId);

				if (pass == 1 && Serializer::isOperationCompletionMarker(blockBufferIo))
					completedOperations.push_back(operationId);

				if (skipEntry)
					assert_and_return_r(blockBufferIo.seek(entryStartPos + wholeEntrySize), false);
				else
				{
					// Contruct the operation and report that it has to be replayed.
					// Reports operations that are logged but weren't completed against the storage.
					const bool deserializedSuccessfully = Serializer::deserialize(blockBufferIo, std::forward<Receiver>(unfinishedOperationsReceiver));
					assert_and_return_message_r(deserializedSuccessfully, "Failed to deserialize an operation from log, but checksum has been verified!", false);
				}
			}
		}

		if (pass == 1)
			assert_and_return_r(_logFile.seek(0), false);
	}

	return true;
}

// Registers the new operation and returns its unique ID. Empty optional = failure to register.

template<RecordType Record, class StorageAdapter>
template<class OpType>
[[nodiscard]] std::optional<WAL::OpID>
DbWAL<Record, StorageAdapter>::registerOperation(OpType&& op) noexcept
{
	// Compose all the data in a buffer and then write in one go.
	// Reference: https://github.com/VioletGiraffe/cpp-db/wiki/WAL

	// This is the buffer for the single entry, it cannot exceed the block size
	io::StaticBufferAdapter<BlockSize> entryBuffer;

	// Reserving space for the total entry size and operation ID in the beginning
	entryBuffer.reserve(sizeof(EntrySizeType) + sizeof(WAL::OpID));
	entryBuffer.seekToEnd();

	StorageIO io{ entryBuffer };
	
	// !!!!!!!!!!!!!!!!!! Warning: unsafe to use 'op' after forwarding it
	assert_and_return_r(Serializer::serialize(std::forward<OpType>(op), io), {});

	// Fill in the size
	const auto entrySize = entryBuffer.size();
	assert_debug_only(entrySize <= std::numeric_limits<EntrySizeType>::max());
	assert_debug_only(entrySize > MinItemSize);

	assert_r(io.write(static_cast<EntrySizeType>(entrySize), 0 /* position to write at */));

	bool firstWriter = false;
	uint64_t timeStamp = 0;
	WAL::OpID newOpId = 0;

	{
		// Time to lock the shared block buffer
		std::lock_guard lock{_mtxBlock};

		newOpId = ++_lastOpId;
		// Fill in the ID
		assert_r(io.write(newOpId));
		// Now buffer holds the complete entry

		assert_debug_only(newOpId > _lastBlockOpId);
		assert_debug_only(_lastBlockOpId >= _lastFlushedOpId);

		TRACE("Thread %ld\tregisterOperation: \tcurrentOpId=%d, first=%d, blockSize=%ld,t=%lu\n", get_tid(), newOpId, (int)firstWriter, _blockItemCount.load(), timeElapsedMs());

		if (newBlockRequiredForData(entrySize)) // Not enough space left
		{
			if (firstWriter) [[unlikely]]
				fatalAbort("Not enough space in the block!"); // TODO: handle the case of data being larger than one block can fit

			assert_and_return_r(finalizeAndflushCurrentBlock(), {});
			startNewBlock();
		}

		firstWriter = blockIsEmpty();
		timeStamp = firstWriter ? timeElapsedMs() : 0;

		_lastBlockOpId = newOpId;

		assert_and_return_r(_block.write(entryBuffer.data(), entrySize), {});
		++_blockItemCount;
		assert_debug_only(_blockItemCount <= MaxItemCount);

		_pendingOperations.push_back(newOpId);
		++_operationsProcessed;
	}

	/*//////////////////////////////////////////////////////////////////////////////////////////////////////
	             Waiting for another thread to flush the block and handling the timeout
	//////////////////////////////////////////////////////////////////////////////////////////////////////*/

	waitForFlushAndHandleTimeout(newOpId, firstWriter, timeStamp);

	// Buffer flushed - return
	return newOpId;
}

template<RecordType Record, class StorageAdapter>
bool DbWAL<Record, StorageAdapter>::updateOpStatus(const WAL::OpID opId, const WAL::OpStatus status) noexcept
{
	io::StaticBufferAdapter<BlockSize> entryBuffer;

	// Reserving space for the entry size field
	entryBuffer.reserve(sizeof(EntrySizeType));
	
	StorageIO bufferIo{ entryBuffer };
	assert_and_return_r(bufferIo.seek(sizeof(EntrySizeType)), false);
	assert_and_return_r(bufferIo.write(opId), false);

	const WAL::OperationCompletedMarker completionMarker{ .status = status };
	assert_and_return_r(Serializer::serialize(completionMarker, bufferIo), false);
	const auto entrySize = entryBuffer.size();
	assert_debug_only(entrySize <= std::numeric_limits<EntrySizeType>::max());
	assert_debug_only(entrySize >= MinItemSize);
	// Fill in the size
	assert_and_return_r(bufferIo.write(static_cast<EntrySizeType>(entrySize), /* pos */ 0), false);

	bool firstWriter = false;
	uint64_t timeStamp = 0;
	WAL::OpID newOpId = 0;

	// Time to lock the shared block
	{
		std::lock_guard lock{_mtxBlock};

		newOpId = ++_lastOpId;
		assert_debug_only(newOpId > _lastBlockOpId);
		assert_debug_only(_lastBlockOpId >= _lastFlushedOpId);

		TRACE("Thread %ld\tregisterOperation: \tcurrentOpId=%d, first=%d, blockSize=%ld, t=%lu\n", get_tid(), newOpId, (int)firstWriter, _blockItemCount.load(), timeElapsedMs());

		if (newBlockRequiredForData(entrySize)) // Not enough space left
		{
			if (firstWriter) [[unlikely]]
				fatalAbort("Not enough space in the block!");

			assert_and_return_r(finalizeAndflushCurrentBlock(), false);
			startNewBlock();
		}

		firstWriter = blockIsEmpty();
		timeStamp = firstWriter ? timeElapsedMs() : 0;

		StorageIO blockIo{ _block };
		assert_and_return_r(blockIo.write(entryBuffer.data(), entrySize), false);
		++_blockItemCount;
		assert_debug_only(_blockItemCount <= MaxItemCount);
		_lastBlockOpId = newOpId;

		const auto pendingOperationIterator = std::find(begin_to_end(_pendingOperations), opId);
		assert_and_return_message_r(pendingOperationIterator != _pendingOperations.end(), "OpId=" + std::to_string(opId) + " hasn't been registered!", false);
		_pendingOperations.erase(pendingOperationIterator);

		// TODO: this requires _mtxBlock to be recursive
		//if (_pendingOperations.empty() && _operationsProcessed > 1'000) // TODO: increase this limit for production
		//{
		//	// It's unlikely that there will be no pending operations when this branch is examined - this will almost never be invoked

		//	// No more pending operations left - safe to clear the log
		//	// TODO: do it asynchronously
		//	assert_and_return_r(clearLog(), false);
		//	_operationsProcessed = 0;
		//}
	}

	//  Waiting for another thread to flush the block and handling the timeout

	waitForFlushAndHandleTimeout(newOpId, firstWriter, timeStamp);
	return true;
}

template<RecordType Record, class StorageAdapter>
inline constexpr void DbWAL<Record, StorageAdapter>::startNewBlock() noexcept
{
	assert_debug_only(_mtxBlock.locked_by_caller());
	_blockItemCount = 0;
	_block.clear();
	_block.reserve(sizeof(BlockItemCountType));
	_block.seekToEnd();

	TRACE("Thread %ld\tstartNewBlock: \tblockSize=%ld,t=%lu\n", get_tid(), _blockItemCount.load(), timeElapsedMs());
}

template<RecordType Record, class StorageAdapter>
constexpr bool DbWAL<Record, StorageAdapter>::finalizeAndflushCurrentBlock() noexcept
{
	assert_debug_only(_mtxBlock.locked_by_caller());
	assert_r(_block.size() > 0);

	_block.seek(0);
	assert_and_return_r(_blockItemCount <= std::numeric_limits<BlockItemCountType>::max() && _blockItemCount > 0, false);
	assert_debug_only(_blockItemCount <= MaxItemCount);

	StorageIO blockWriter{ _block };
	assert_and_return_r(blockWriter.write(static_cast<BlockItemCountType>(_blockItemCount.load())), false);
	const auto actualBlockSize = _block.size();
	// Extend the size to full 4K
	_block.reserve(_block.MaxCapacity);
	assert_debug_only(_block.size() == _block.MaxCapacity);
	::memset(_block.data() + actualBlockSize, 0, _block.MaxCapacity - actualBlockSize);

	const auto hash = wheathash32(_block.data(), _block.MaxCapacity - sizeof(BlockChecksumType));
	static_assert(std::is_same_v<decltype(hash), const BlockChecksumType>);
	_block.seek(_block.MaxCapacity - sizeof(BlockChecksumType));
	assert_and_return_r(blockWriter.write(hash), false);

	assert_and_return_r(_logFile.seekToEnd(), false); // TODO: what for?
	assert_and_return_r(_logFile.write(_block.data(), _block.MaxCapacity), false);
	assert_and_return_r(_logFile.flush(), false);

	TRACE("Thread %ld\tflushing: \t\tlastBlockOpId=%d, t=%lu\n", get_tid(), _lastBlockOpId, timeElapsedMs());

	_lastFlushedOpId = _lastBlockOpId;

	++totalBlockCount;
	totalSizeWritten += actualBlockSize;
	if (maxFill < actualBlockSize)
		maxFill = actualBlockSize;

	return true;
}

template<RecordType Record, class StorageAdapter>
inline constexpr bool DbWAL<Record, StorageAdapter>::newBlockRequiredForData(size_t dataSize) noexcept
{
	assert_debug_only(_mtxBlock.locked_by_caller());
	const auto remainingCapacity = _block.remainingCapacity() - sizeof(BlockChecksumType) - sizeof(BlockItemCountType);
	return dataSize + MinItemSize + 20 > remainingCapacity;
}

template<RecordType Record, class StorageAdapter>
inline constexpr bool DbWAL<Record, StorageAdapter>::blockIsEmpty() const noexcept
{
	assert_debug_only(_mtxBlock.locked_by_caller());
	return _blockItemCount == 0;
}

template<RecordType Record, class StorageAdapter>
void DbWAL<Record, StorageAdapter>::waitForFlushAndHandleTimeout(WAL::OpID currentOpId, const bool firstWriter, const uint64_t operationStartTimeStamp) noexcept
{
	/*//////////////////////////////////////////////////////////////////////////////////////////////////////
	                 Waiting for another thread to flush the block and handling the timeout
	//////////////////////////////////////////////////////////////////////////////////////////////////////*/

#ifdef ENABLE_TRACING
	const auto tid = get_tid();
#endif
	TRACE("Thread %ld\twaiting: \t\tcurrentOpId=%d, first=%d, lastFlushedOpId=%d\n", tid, currentOpId, (int)firstWriter, _lastFlushedOpId.load());

	while (_lastFlushedOpId < currentOpId)
	{
		static constexpr uint64_t timeOut = 50; // ms
		const uint64_t elapsed = timeElapsedMs() - operationStartTimeStamp;
		if (firstWriter && elapsed >= timeOut)
		{
			std::lock_guard lck(_mtxBlock);
			// Re-check the last flushed OP ID under lock in case another thread started flushing at the same time
			if (_lastFlushedOpId < currentOpId)
			{
				TRACE("Thread %ld\ttimeout: \t\tcurrentOpId=%d, first=%d, lastFlushedOpId=%d, lastStoredOpId=%d\n", tid, currentOpId, (int)firstWriter, _lastFlushedOpId.load(), _lastBlockOpId);
				if (!finalizeAndflushCurrentBlock()) [[unlikely]]
					fatalAbort("WAL: flush failed!");

				startNewBlock();
			}

			break;
		}

		// Keep spinning until either another thread finishes the job or timeout occurs
		std::this_thread::yield();
	}

	TRACE("Thread %ld\tdone: \t\t\tcurrentOpId=%d, first=%d, lastFlushedOpId=%d, t=%lu\n", tid, currentOpId, (int)firstWriter, _lastFlushedOpId.load(), timeElapsedMs());
	// Done waiting - return
}
