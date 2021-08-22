#pragma once

#include "dbschema.hpp"
#include "dbops.hpp"
#include "storage/storage_io_interface.hpp"
#include "ops/operation_serializer.hpp"
#include "storage/io_with_hashing.hpp"

#include <thread>

template <class Record, class StorageAdapter>
class DbWAL
{
	static_assert(Record::isRecord());

public:
	[[nodiscard]] bool openLogFile(const std::string& filePath) noexcept;
	[[nodiscard]] bool verifyLog() noexcept;
	[[nodiscard]] bool clearLog() noexcept;

	template <class OpType>
	[[nodiscard]] bool registerOperation(OpType&& op) noexcept;

private:
	StorageIO<io::HashingAdapter<StorageAdapter>> _logFile;
	const std::thread::id _ownerThreadId = std::this_thread::get_id();
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
[[nodiscard]] bool DbWAL<Record, StorageAdapter>::registerOperation(OpType&& op) noexcept
{
	assert_debug_only(std::this_thread::get_id() == _ownerThreadId);

	using Serializer = Operation::Serializer<DbSchema<Record>>;
	const auto binaryData = Serializer::serialize(std::forward<OpType>(op), _logFile);
	return false;
}
