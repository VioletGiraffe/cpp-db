#pragma once

#include "dbops.hpp"
#include "dbschema.hpp"
#include "storage/storage_io_interface.hpp"
#include "storage/storage_static_buffer.hpp"
#include "utils/dbutilities.hpp"
#include "WAL/wal_serializer.hpp"

#include "container/std_container_helpers.hpp"
#include "hash/fnv_1a.h"
#include "system/rdtsc.h"

#include <atomic>
#include <mutex>
#include <optional>
#include <thread>
#include <vector>

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

	using OpID = uint32_t;

	[[nodiscard]] bool openLogFile(const std::string& filePath) noexcept;
	[[nodiscard]] bool closeLogFile() noexcept;

	template <typename Receiver>
	[[nodiscard]] bool verifyLog(Receiver&& unfinishedOperationsReceiver) noexcept;
	[[nodiscard]] bool clearLog() noexcept;

	// Registers the new operation and returns its unique ID. Empty optional = failure to register.
	template <class OpType>
	[[nodiscard]] std::optional<OpID> registerOperation(OpType&& op) noexcept;
	[[nodiscard]] bool updateOpStatus(OpID opId, WAL::OpStatus status) noexcept;

private:
	constexpr void startNewBlock() noexcept;
	[[nodiscard]] constexpr bool finalizeAndflushCurrentBlock() noexcept;
	[[nodiscard]] constexpr bool newBlockRequiredForData(size_t dataSize) noexcept;

	[[nodiscard]] constexpr bool blockIsEmpty() const noexcept;

	// Waits for the record with ID == opId to get flushed.
	// If that doesn't happen within certain time, performs flush.
	void waitForFlushAndHandleTimeout(OpID opId, bool firstWriter, uint64_t operationStartTimeStamp) noexcept;

private:
	using EntrySizeType = uint16_t;
	using Serializer = WAL::Serializer<Record>;

	using BlockChecksumType = uint32_t;
	static_assert(std::is_same_v<BlockChecksumType, decltype(FNV_1a_32(nullptr, 0))>);

	using BlockItemCountType = uint16_t;
	static constexpr size_t BlockSize = 4096;

	// These two definitions are for bug checking only
	static constexpr size_t MinItemSize = sizeof(EntrySizeType) + sizeof(OpID) + 1 /* assume at least one byte of payload */;
	static constexpr size_t MaxItemCount = BlockSize / MinItemSize;

private:
	std::mutex _mtxBlock;
	// Has to be atomic for the wait loop to spin on it.
	std::atomic<OpID> _lastFlushedOpId = 0;
	// Doesn't have to be atomic - only accessed under mutex
	OpID _lastStoredOpId = 0;
	
	io::StaticBufferAdapter<BlockSize> _block;
	size_t _blockItemCount = 0;

	// Solely for tracking how many ops are pending. When none, the log can be trimmed
	std::vector<OpID> _pendingOperations;
	size_t _operationsProcessed = 0;

	StorageIO<StorageAdapter> _logFile;
	std::atomic<OpID> _lastOpId = 0;

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

	startNewBlock();
	// ReadWrite required in order to be able to seek back and patch operation status.
	return _logFile.open(filePath, io::OpenMode::ReadWrite);
}

template<RecordType Record, class StorageAdapter>
[[nodiscard]] bool DbWAL<Record, StorageAdapter>::closeLogFile() noexcept
{
	assert_debug_only(std::this_thread::get_id() == _ownerThreadId);

	std::lock_guard lock(_mtxBlock);
	// ReadWrite required in order to be able to seek back and patch operation status.
	if (!blockIsEmpty())
	{
		assert_and_return_r(finalizeAndflushCurrentBlock(), false);
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
	assert_r(_logFile.pos() == 0);

	// This is the buffer for the whole WAL block
	io::StaticBufferAdapter<BlockSize> blockBuffer;
	blockBuffer.reserve(BlockSize);
	StorageIO blockBufferIo{ blockBuffer };

	assert_r(_logFile.size() % BlockSize == 0);

	std::vector<OpID> completedOperations;
	const auto isCompleted = [&completedOperations](const OpID id) {
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

			const auto actualChecksum = FNV_1a_32_hasher{}.updateHash(blockBuffer.data(), BlockSize - sizeof(BlockChecksumType));
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

				OpID operationId = 0;
				assert_and_return_r(blockBufferIo.read(operationId) && operationId != 0, false);

				// First pass: skip everything. Second pass: skip completed operations.
				const bool skipEntry = pass == 1 || isCompleted(operationId);

				if (pass == 1 && Serializer::isOperationCompletionMarker(blockBufferIo))
					completedOperations.push_back(operationId);

				if (skipEntry)
					assert_and_return_r(blockBufferIo.seek(entryStartPos + wholeEntrySize), false);
				else
				{
					// Contruct the operation and report that it has to be replayed
					const bool deserializedSuccessfully = Serializer::deserialize(blockBufferIo, [&unfinishedOperationsReceiver](auto&& operation) {
					// This operation is logged, but wasn't completed against the storage.
						unfinishedOperationsReceiver(std::move(operation));
					});

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
[[nodiscard]] std::optional<typename DbWAL<Record, StorageAdapter>::OpID>
DbWAL<Record, StorageAdapter>::registerOperation(OpType&& op) noexcept
{
	// Compose all the data in a buffer and then write in one go.
	// Reference: https://github.com/VioletGiraffe/cpp-db/wiki/WAL

	// This is the buffer for the single entry, it cannot exceed the block size
	io::StaticBufferAdapter<BlockSize> entryBuffer;

	// Reserving space for the total entry size and operation ID in the beginning
	entryBuffer.reserve(sizeof(EntrySizeType) + sizeof(OpID));
	entryBuffer.seekToEnd();

	StorageIO io{ entryBuffer };
	
	// !!!!!!!!!!!!!!!!!! Warning: unsafe to use 'op' after forwarding it
	assert_and_return_r(Serializer::serialize(std::forward<OpType>(op), io), {});

	// Fill in the size
	const auto entrySize = entryBuffer.size();
	assert_debug_only(entrySize <= std::numeric_limits<EntrySizeType>::max());
	assert_debug_only(entrySize > MinItemSize);

	assert_r(io.write(static_cast<EntrySizeType>(entrySize), 0 /* position to write at */));

	const OpID newOpId = ++_lastOpId;
	// Fill in the ID
	assert_r(io.write(newOpId));
	// Now buffer holds the complete entry

	bool firstWriter = false;
	uint64_t timeStamp = 0;

	{
		// Time to lock the shared block buffer
		std::lock_guard lock{_mtxBlock};

		firstWriter = blockIsEmpty();
		timeStamp = firstWriter ? rdtsc() : 0;

		if (newBlockRequiredForData(entrySize)) // Not enough space left
		{
			if (firstWriter) [[unlikely]]
				fatalAbort("Not enough space in the block!"); // TODO: handle the case of data being larger than one block can fit

			assert_and_return_r(finalizeAndflushCurrentBlock(), {});
			startNewBlock();
		}

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
inline bool DbWAL<Record, StorageAdapter>::updateOpStatus(const OpID opId, const WAL::OpStatus status) noexcept
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
	OpID newOpId = 0;

	// Time to lock the shared block
	{
		std::lock_guard lock{_mtxBlock};

		firstWriter = blockIsEmpty();
		timeStamp = firstWriter ? rdtsc() : 0;
		newOpId = ++_lastOpId;

		if (newBlockRequiredForData(entrySize)) // Not enough space left
		{
			if (firstWriter) [[unlikely]]
				fatalAbort("Not enough space in the block!");

			assert_and_return_r(finalizeAndflushCurrentBlock(), false);
			startNewBlock();
		}

		StorageIO blockIo{ _block };
		assert_and_return_r(blockIo.write(entryBuffer.data(), entrySize), false);
		++_blockItemCount;

		auto pendingOperationIterator = std::find(begin_to_end(_pendingOperations), opId);
		assert_and_return_message_r(pendingOperationIterator != _pendingOperations.end(), "Operation" + std::to_string(opId) + " hasn't been registered!", false);
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
	_blockItemCount = 0;
	_block.clear();
	_block.reserve(sizeof(BlockItemCountType));
	_block.seekToEnd();
}

template<RecordType Record, class StorageAdapter>
inline constexpr bool DbWAL<Record, StorageAdapter>::finalizeAndflushCurrentBlock() noexcept
{
	assert_r(_block.size() > 0);

	_block.seek(0);
	assert_and_return_r(_blockItemCount <= std::numeric_limits<BlockItemCountType>::max() && _blockItemCount > 0, false);
	assert_debug_only(_blockItemCount <= MaxItemCount);

	StorageIO blockWriter{ _block };
	assert_and_return_r(blockWriter.write(static_cast<BlockItemCountType>(_blockItemCount)), false);
	const auto actualSize = _block.size();
	// Extend the size to full 4K
	_block.reserve(_block.MaxCapacity);
	assert_debug_only(_block.size() == _block.MaxCapacity);
	::memset(_block.data() + actualSize, 0, _block.MaxCapacity - actualSize);

	const auto hash = FNV_1a_32(_block.data(), _block.MaxCapacity - sizeof(BlockChecksumType));
	static_assert(std::is_same_v<decltype(hash), const BlockChecksumType>);
	_block.seek(_block.MaxCapacity - sizeof(BlockChecksumType));
	assert_and_return_r(blockWriter.write(hash), false);

	assert_and_return_r(_logFile.seekToEnd(), false); // TODO: what for?
	assert_and_return_r(_logFile.write(_block.data(), _block.MaxCapacity), false);
	assert_and_return_r(_logFile.flush(), false);

	_lastFlushedOpId = _lastStoredOpId;

	return true;
}

template<RecordType Record, class StorageAdapter>
inline constexpr bool DbWAL<Record, StorageAdapter>::newBlockRequiredForData(size_t dataSize) noexcept
{
	return dataSize > _block.remainingCapacity() - sizeof(BlockChecksumType) - sizeof(BlockItemCountType);
}

template<RecordType Record, class StorageAdapter>
inline constexpr bool DbWAL<Record, StorageAdapter>::blockIsEmpty() const noexcept
{
	return _block.size() <= 2;
}

template<RecordType Record, class StorageAdapter>
inline void DbWAL<Record, StorageAdapter>::waitForFlushAndHandleTimeout(OpID currentOpId, const bool firstWriter, const uint64_t operationStartTimeStamp) noexcept
{
	/*//////////////////////////////////////////////////////////////////////////////////////////////////////
	                 Waiting for another thread to flush the block and handling the timeout
	//////////////////////////////////////////////////////////////////////////////////////////////////////*/

	while (_lastFlushedOpId < currentOpId)
	{
		// Assuming 2 GHz clock speed. Will increment twice faster (shorter timeout) at 4 GHz.
		static constexpr uint64_t twoBillion = 1_u64 << (31 - 3); // Counting in 8ths of a second
		// Shorter tmeout for debugging. TODO: restore later
		//static constexpr uint64_t timeOut = 4;
		static constexpr uint64_t timeOut = 1;
		const uint64_t elapsedSeconds = (rdtsc() - operationStartTimeStamp) / twoBillion;
		if (firstWriter && elapsedSeconds > timeOut)
		{
			std::lock_guard lck(_mtxBlock);
			// Re-check the last flushed OP ID under lock in case another thread started flushing at the same time
			if (_lastFlushedOpId < currentOpId)
			{
				if (!finalizeAndflushCurrentBlock()) [[unlikely]]
					fatalAbort("WAL: flush failed!");

				startNewBlock();
			}

			break;
		}

		// Keep spinning until either another thread finishes the job or timeout occurs
		std::this_thread::yield();
	}

	// Done waiting - return
}
