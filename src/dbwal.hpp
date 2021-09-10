#pragma once

#include "dbschema.hpp"
#include "dbops.hpp"
#include "storage/storage_io_interface.hpp"
#include "ops/operation_serializer.hpp"
#include "storage/io_with_hashing.hpp"

#include <map>
#include <thread>

template <class Record, class StorageAdapter>
class DbWAL
{
	static_assert(Record::isRecord());

public:
	using OpID = uint32_t;

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

	struct OperationStatus {
		uint64_t locationInTheLogFile;
		OpStatus status;
	};

	using size_type = uint32_t;

private:
	
	std::map<OpID, OperationStatus> _pendingOperations;
	StorageIO<io::HashingAdapter<StorageAdapter>> _logFile;
	const std::thread::id _ownerThreadId = std::this_thread::get_id();

	static constexpr auto malformed_marker = static_cast<size_type>(0xDEADF00D);
};

template<class Record, class StorageAdapter>
[[nodiscard]] bool DbWAL<Record, StorageAdapter>::openLogFile(const std::string& filePath) noexcept
{
	assert_debug_only(std::this_thread::get_id() == _ownerThreadId);
	return _logFile.open(filePath, io::OpenMode::ReadWrite);
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
	assert_debug_only(std::this_thread::get_id() == _ownerThreadId);
	// Reference: https://github.com/VioletGiraffe/cpp-db/wiki/WAL-concepts#normal-operation 

	// TODO: error handling? What to do with errors here?
	assert_and_return_r(_logFile.write(malformed_marker), false);
	assert_and_return_r(_logFile.write(opId), false);


	using Serializer = Operation::Serializer<Record>;
	const auto binaryData = Serializer::serialize(std::forward<OpType>(op), _logFile);
	return false;
}
