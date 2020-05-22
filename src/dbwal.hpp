#pragma once

#include "storage/storage_io_interface.hpp"

#include <vector>

template <class Record, class StorageAdapter>
class DbWAL
{
	static_assert(Record::isRecord());

public:
	[[nodiscard]] bool openLogFile(const std::string& filePath);

	template <class Operation>
	[[nodiscard]] bool registerOp(const Operation& op);

private:
	StorageIO<StorageAdapter> _storage;
};

template<class Record, class StorageAdapter>
inline bool DbWAL<Record, StorageAdapter>::openLogFile(const std::string& filePath)
{
	return _storage.open(filePath, io::OpenMode::ReadWrite);
}
