#pragma once

#include "dbops.hpp"
#include "storage/storage_io_interface.hpp"

template <class Record, class StorageAdapter>
class DbWAL
{
	static_assert(Record::isRecord());

public:
	[[nodiscard]] bool openLogFile(const std::string& filePath);

	template <class Operation, sfinae<Operation::op == Operation::Insert::op> = true>
	[[nodiscard]] bool registerOp(const Operation& op);

	template <class Operation, sfinae<Operation::op == Operation::Find::op> = true>
	[[nodiscard]] bool registerOp(const Operation& op);

	template <class Operation, sfinae<Operation::op == Operation::UpdateFull::op> = true>
	[[nodiscard]] bool registerOp(const Operation& op);

	template <class Operation, sfinae<Operation::op == Operation::AppendToArray::op> = true>
	[[nodiscard]] bool registerOp(const Operation& op);

	template <class Operation, sfinae<Operation::op == Operation::Delete::op> = true>
	[[nodiscard]] bool registerOp(const Operation& op);

private:
	StorageIO<StorageAdapter> _storage;
};

template<class Record, class StorageAdapter>
inline bool DbWAL<Record, StorageAdapter>::openLogFile(const std::string& filePath)
{
	return _storage.open(filePath, io::OpenMode::ReadWrite);
}

template<class Record, class StorageAdapter>
template<class Operation, sfinae<Operation::op == Operation::Insert::op>>
inline bool DbWAL<Record, StorageAdapter>::registerOp(const Operation& op)
{
	return false;
}

template<class Record, class StorageAdapter>
template<class Operation, sfinae<Operation::op == Operation::Find::op>>
inline bool DbWAL<Record, StorageAdapter>::registerOp(const Operation& op)
{
	return false;
}
template<class Record, class StorageAdapter>
template<class Operation, sfinae<Operation::op == Operation::UpdateFull::op>>
inline bool DbWAL<Record, StorageAdapter>::registerOp(const Operation& op)
{
	return false;
}
template<class Record, class StorageAdapter>
template<class Operation, sfinae<Operation::op == Operation::AppendToArray::op>>
inline bool DbWAL<Record, StorageAdapter>::registerOp(const Operation& op)
{
	return false;
}
template<class Record, class StorageAdapter>
template<class Operation, sfinae<Operation::op == Operation::Delete::op>>
inline bool DbWAL<Record, StorageAdapter>::registerOp(const Operation& op)
{
	return false;
}