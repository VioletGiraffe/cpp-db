#pragma once

#include "dbrecord.hpp"
#include "db_type_concepts.hpp"
#include "storage/storage_io_interface.hpp"
#include "serialization/dbrecord-serializer.hpp"

#include "assert/advanced_assert.h"
#include "parameter_pack/parameter_pack_helpers.hpp"
#include "utility/extra_type_traits.hpp"
#include "utility/odd_sized_integer.hpp"
#include "utility/named_type_wrapper.hpp"

#include <limits>
#include <mutex>
#include <string>

//using PageNumber = UniqueNamedType(odd_sized_integer<5>);
using PageNumber = odd_sized_integer<5>;

template <typename StorageAdapter, RecordConcept Record>
class DBStorage
{
	static constexpr size_t PageSize = 4096;

public:
	[[nodiscard]] bool openStorageFile(const std::string& filePath)
	{
		return _storageFile.open(filePath, io::OpenMode::ReadWrite);
	}

	[[nodiscard]] bool readRecord(Record& record, const PageNumber recordStartLocation)
	{
		std::lock_guard locker(_storageMutex);

		assert_and_return_r(_storageFile.seek(pageNumberToOffset(recordStartLocation)), false);
		return DbRecordSerializer<Record>::deserialize(record, _storageFile);
	}

	[[nodiscard]] bool writeRecord(const Record& record)
	{
		std::lock_guard locker(_storageMutex);

		assert_and_return_r(_storageFile.seekToEnd(), false);
		return DbRecordSerializer<Record>::serialize(record, _storageFile);
	}

private:
	[[nodiscard]] static constexpr uint64_t pageNumberToOffset(PageNumber pgN) noexcept
	{
		return pgN * PageSize;
	}

private:
	StorageAdapter _ioAdapter;
	StorageIO<StorageAdapter> _storageFile{ _ioAdapter };
	std::mutex _storageMutex;
};
