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

	[[nodiscard]] bool openLogFile(const std::string& filePath) noexcept;
	[[nodiscard]] bool verifyLog() noexcept;
	[[nodiscard]] bool clearLog() noexcept;

	template <class OpType>
	[[nodiscard]] bool registerOperation(OpID opId, OpType&& op) noexcept;

private:
	enum OpStatus : uint8_t {
		Pending = 0xAA,
		Successful = 0xFF,
		Failed = 0x11
	};

	struct LogEntry {
		uint64_t offsetInLogFile;
		OpID id;
		OpStatus status;
	};

private:

	std::vector<LogEntry> _pendingOperations;
	StorageIO<StorageAdapter> _logFile;
	const std::thread::id _ownerThreadId = std::this_thread::get_id();
};

template<class Record, class StorageAdapter>
[[nodiscard]] bool DbWAL<Record, StorageAdapter>::openLogFile(const std::string& filePath) noexcept
{
	assert_debug_only(std::this_thread::get_id() == _ownerThreadId);
	// ReadWrite required in order to be able to seek back and patch operation status.
	assert_and_return_r(_logFile.open(filePath, io::OpenMode::ReadWrite), false);
	return _logFile.clear();
}

template<class Record, class StorageAdapter>
[[nodiscard]] bool DbWAL<Record, StorageAdapter>::verifyLog() noexcept
{
	assert_debug_only(std::this_thread::get_id() == _ownerThreadId);
	assert_r(_logFile.pos() == 0);
	return false;
}

template<class Record, class StorageAdapter>
[[nodiscard]] bool DbWAL<Record, StorageAdapter>::clearLog() noexcept
{
	assert_debug_only(std::this_thread::get_id() == _ownerThreadId);
	return _logFile.clear();
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

	using EntrySizeType = uint16_t;
	// Reserving space for the total entry size in the beginning
	buffer.ioAdapter().reserve(sizeof(EntrySizeType));
	buffer.ioAdapter().seekToEnd();

	using Serializer = Operation::Serializer<Record>;
	// TODO: how to handle this error?
	assert_and_return_r(Serializer::serialize(std::forward<OpType>(op), buffer), false);

	// !!!!!!!!!!!!!!!!!! Warning: unsafe to use 'op' after forwarding it

	using HashType = decltype(FNV_1a_32(nullptr, 0));

	const EntrySizeType entrySize = static_cast<EntrySizeType>(buffer.size() + sizeof(HashType));
	// Fill in the size
	(void)buffer.write(entrySize, 0 /* position to write at */);
	buffer.ioAdapter().seekToEnd();
	
	const auto hash = FNV_1a_32(buffer.ioAdapter().data(), entrySize - sizeof(HashType));
	(void)buffer.write(hash);

	_pendingOperations.emplace_back(_logFile.pos(), opId, Pending);

	return _logFile.write(buffer.ioAdapter().data(), buffer.size());
}
