#pragma once

#include "dbschema.hpp"
#include "dbops.hpp"
#include "storage/storage_io_interface.hpp"
#include "ops/operation_serializer.hpp"
#include "storage/storage_static_buffer.hpp"

#include "hash/fnv_1a.h"

#include <thread>
#include <vector>

template <class Record, class StorageAdapter>
class DbWAL
{
	static_assert(Record::isRecord());

public:
	using OpID = uint64_t;
	enum class OpStatus : uint8_t {
		Pending = 0xAA,
		Successful = 0xFF,
		Failed = 0x11
	};

	[[nodiscard]] bool openLogFile(const std::string& filePath) noexcept;
	[[nodiscard]] bool closeLogFile() noexcept;

	template <typename Receiver>
	[[nodiscard]] bool verifyLog(Receiver&& unfinishedOperationsReceiver) noexcept;
	[[nodiscard]] bool clearLog() noexcept;

	template <class OpType>
	[[nodiscard]] bool registerOperation(OpID opId, OpType&& op) noexcept;
	[[nodiscard]] bool updateOpStatus(OpID opId, OpStatus status) noexcept;

private:
	using EntrySizeType = uint16_t;
	using HashType = decltype(FNV_1a_32(nullptr, 0));
	using Serializer = Operation::Serializer<Record>;

private:
	struct LogEntry {
		uint64_t offsetInLogFile;
		OpID id;
		OpStatus status;
	};

	std::vector<LogEntry> _pendingOperations;
	StorageIO<StorageAdapter> _logFile;
	size_t _operationsProcessed = 0;

	const std::thread::id _ownerThreadId = std::this_thread::get_id();
};

template<class Record, class StorageAdapter>
[[nodiscard]] bool DbWAL<Record, StorageAdapter>::openLogFile(const std::string& filePath) noexcept
{
	assert_debug_only(std::this_thread::get_id() == _ownerThreadId);
	// ReadWrite required in order to be able to seek back and patch operation status.
	return _logFile.open(filePath, io::OpenMode::ReadWrite);
}

template<class Record, class StorageAdapter>
[[nodiscard]] bool DbWAL<Record, StorageAdapter>::closeLogFile() noexcept
{
	assert_debug_only(std::this_thread::get_id() == _ownerThreadId);
	// ReadWrite required in order to be able to seek back and patch operation status.
	return _logFile.close();
}

template<class Record, class StorageAdapter>
[[nodiscard]] bool DbWAL<Record, StorageAdapter>::clearLog() noexcept
{
	assert_debug_only(std::this_thread::get_id() == _ownerThreadId);
	assert_and_return_r(_logFile.clear(), false);
	return _logFile.flush();
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

template<class Record, class StorageAdapter>
template <typename Receiver>
[[nodiscard]] bool DbWAL<Record, StorageAdapter>::verifyLog(Receiver&& unfinishedOperationsReceiver) noexcept
{
	assert_debug_only(std::this_thread::get_id() == _ownerThreadId);
	assert_r(_logFile.pos() == 0);

	// Malformed data - OK, not an error, skip and return true.
	std::vector<std::byte> buffer;
	bool success = true;
	while (!_logFile.atEnd())
	{
		EntrySizeType wholeEntrySize;
		assert_and_return_r(_logFile.read(wholeEntrySize), false);

		const auto dataAndHashSize = wholeEntrySize - sizeof(wholeEntrySize);
		buffer.resize(dataAndHashSize);
		assert_and_return_r(_logFile.read(buffer.data(), dataAndHashSize), false);

		static constexpr auto OpStatusOffset = 0;
		const OpStatus status = memory_cast<OpStatus>(buffer.data() + OpStatusOffset);

		// No need to process operations that finished successfully, but we do want to check the hash.
		const HashType hash = memory_cast<HashType>(buffer.data() + dataAndHashSize - sizeof(HashType));

		// Patch STATUS to PENDING in order to verify the hash
		{
			const auto pending = OpStatus::Pending;
			::memcpy(buffer.data() + OpStatusOffset, &pending, sizeof(pending));
		}

		// Hash verification
		FNV_1a_32_hasher hasher;
		hasher.updateHash(wholeEntrySize);
		const HashType actualHash = hasher.updateHash(buffer.data(), dataAndHashSize - sizeof(HashType));
		if (actualHash != hash)
		{
			assert_unconditional_r("Broken data on the disk!");
			continue; // Check the next item?
		}

		// Operation has completed - no need to deserialize it, move on to the next log entry.
		if (status != OpStatus::Pending)
			continue;

		StorageIO<io::MemoryBlockAdapter> ioDevice(buffer.data() + sizeof(OpStatus), dataAndHashSize - sizeof(HashType) - sizeof(OpStatus));
		const bool deserializedSuccessfully = Serializer::deserialize(ioDevice, [&unfinishedOperationsReceiver](auto&& operation) {
			// This operation is logged, but wasn't completed against the storage.
			unfinishedOperationsReceiver(std::move(operation));
		});

		if (!deserializedSuccessfully)
		{
			assert_unconditional_r("Failed to deserialize an operation from log, but hash checked out!");
			success = false;
		}

		// Continue to the next operation
	}

	return success;
}

template<class Record, class StorageAdapter>
template<class OpType>
[[nodiscard]] bool DbWAL<Record, StorageAdapter>::registerOperation(const OpID opId, OpType&& op) noexcept
{
	// Compose all the data in a buffer and then write in one go.
	// Reference: https://github.com/VioletGiraffe/cpp-db/wiki/WAL-concepts#normal-operation 

	assert_debug_only(std::this_thread::get_id() == _ownerThreadId);

	// 4K ought to be enough for everybody!
	StorageIO<io::StaticBufferAdapter<4096>> buffer;

	// Reserving space for the total entry size in the beginning
	buffer.ioAdapter().reserve(sizeof(EntrySizeType));
	buffer.ioAdapter().seekToEnd();

	assert_and_return_r(buffer.write(OpStatus::Pending), false);

	using Serializer = Operation::Serializer<Record>;
	// TODO: how to handle this error?
	assert_and_return_r(Serializer::serialize(std::forward<OpType>(op), buffer), false);

	// !!!!!!!!!!!!!!!!!! Warning: unsafe to use 'op' after forwarding it

	const auto bufferSize = buffer.size();
	assert_r(bufferSize < std::numeric_limits<EntrySizeType>::max() - sizeof(HashType));
	const EntrySizeType entrySize = static_cast<EntrySizeType>(buffer.size() + sizeof(HashType));
	// Fill in the size
	(void)buffer.write(entrySize, 0 /* position to write at */);
	buffer.ioAdapter().seekToEnd();
	
	const auto hash = FNV_1a_32(buffer.ioAdapter().data(), entrySize - sizeof(HashType));
	(void)buffer.write(hash);

	_pendingOperations.emplace_back(_logFile.pos(), opId, OpStatus::Pending);

	++_operationsProcessed;

	assert_and_return_r(_logFile.seekToEnd(), false);
	assert_and_return_r(_logFile.write(buffer.ioAdapter().data(), buffer.size()), false);
	assert_and_return_r(_logFile.flush(), false);

	return true;
}

template<class Record, class StorageAdapter>
inline bool DbWAL<Record, StorageAdapter>::updateOpStatus(const OpID opId, const OpStatus status) noexcept
{
	assert_debug_only(std::this_thread::get_id() == _ownerThreadId);

	auto logEntry = std::find_if(begin_to_end(_pendingOperations), [opId](const LogEntry& item) {
		return item.id == opId;
	});

	assert_and_return_message_r(logEntry != _pendingOperations.end(), "Operation" + std::to_string(opId) + " hasn't been registered!", false);
	assert_debug_only(logEntry->status == OpStatus::Pending);
	assert_debug_only(status != OpStatus::Pending);

	assert_and_return_r(_logFile.seek(logEntry->offsetInLogFile + sizeof(EntrySizeType)), false);
	assert_and_return_r(_logFile.write(status), false);
	// TODO: flush!

	_pendingOperations.erase(logEntry);

	if (_pendingOperations.empty() && _operationsProcessed > 1024)
	{
		// No more pending operations left - safe to clear the log
		// TODO: do it asynchronously
		assert_and_return_r(clearLog(), false);
		_operationsProcessed = 0;
	}

	return true;
}