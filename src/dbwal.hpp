#pragma once

#include "dbschema.hpp"
#include "dbops.hpp"
#include "storage/storage_io_interface.hpp"
#include "ops/operation_serializer.hpp"

template <class Record, class StorageAdapter>
class DbWAL
{
	static_assert(Record::isRecord());

public:
	[[nodiscard]] bool openLogFile(const std::string& filePath);
	[[nodiscard]] bool verifyLog();
	[[nodiscard]] bool clearLog();

	template <class OpType>
	[[nodiscard]] bool registerOperation(OpType&& op);

private:
	StorageIO<StorageAdapter> _logFile;
};

template<class Record, class StorageAdapter>
[[nodiscard]] inline bool DbWAL<Record, StorageAdapter>::openLogFile(const std::string& filePath)
{
	return _logFile.open(filePath, io::OpenMode::ReadWrite);
}

template<class Record, class StorageAdapter>
[[nodiscard]] inline bool DbWAL<Record, StorageAdapter>::verifyLog()
{

	return false;
}

template<class Record, class StorageAdapter>
[[nodiscard]] bool DbWAL<Record, StorageAdapter>::clearLog()
{
	return _logFile.clear();
}

template<class Record, class StorageAdapter>
template<class OpType>
[[nodiscard]] bool DbWAL<Record, StorageAdapter>::registerOperation(OpType&& op)
{
	using Serializer = Operation::Serializer<DbSchema<Record>>;
	const auto binaryData = Serializer::serialize(std::forward<OpType>(op));
	return false;
}
