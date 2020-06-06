#pragma once

#include "dbops.hpp"
#include "storage/storage_io_interface.hpp"

template <class Record, class StorageAdapter>
class DbWAL
{
	static_assert(Record::isRecord());

public:
	[[nodiscard]] bool openLogFile(const std::string& filePath);

	template <class Operation, typename = std::enable_if_t<Operation::op == Operation::Insert::op>>
	[[nodiscard]] bool registerOp(const Operation& op);

	template <class Operation, typename = std::enable_if_t<Operation::op == Operation::Find::op>>
	[[nodiscard]] bool registerOp(const Operation& op);

	template <class Operation, typename = std::enable_if_t<Operation::op == Operation::UpdateFull::op>>
	[[nodiscard]] bool registerOp(const Operation& op);

	template <class Operation, typename = std::enable_if_t<Operation::op == Operation::AppendToArray::op>>
	[[nodiscard]] bool registerOp(const Operation& op);

	template <class Operation, typename = std::enable_if_t<Operation::op == Operation::Delete::op>>
	[[nodiscard]] bool registerOp(const Operation& op);

private:
	StorageIO<StorageAdapter> _storage;
};

template<class Record, class StorageAdapter>
inline bool DbWAL<Record, StorageAdapter>::openLogFile(const std::string& filePath)
{
	return _storage.open(filePath, io::OpenMode::ReadWrite);
}
