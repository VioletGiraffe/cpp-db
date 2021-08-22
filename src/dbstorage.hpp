#pragma once

#include "dbrecord.hpp"
#include "storage/storage_io_interface.hpp"
#include "assert/advanced_assert.h"
#include "serialization/dbrecord-serializer.hpp"
#include "parameter_pack/parameter_pack_helpers.hpp"
#include "utility/extra_type_traits.hpp"

#include <limits>
#include <mutex>
#include <string>

struct StorageLocation {
	uint64_t location;

	static constexpr auto noLocation = std::numeric_limits<decltype(location)>::max();

	inline constexpr StorageLocation(uint64_t loc) noexcept : location{ loc } {}

	[[nodiscard]] constexpr auto operator<=>(const StorageLocation&) const = default;
};

template <typename StorageAdapter, typename Record>
class DBStorage
{
public:
	[[nodiscard]] bool openStorageFile(const std::string& filePath)
	{
		return _storageFile.open(filePath, io::OpenMode::ReadWrite);
	}

	[[nodiscard]] bool readRecord(Record& record, const StorageLocation recordStartLocation)
	{
		std::lock_guard locker(_storageMutex);

		assert_and_return_r(_storageFile.seek(recordStartLocation.location), false);
		return DbRecordSerializer<Record>::deserialize(record, _storageFile);
	}

	[[nodiscard]] bool writeRecord(const Record& record)
	{
		std::lock_guard locker(_storageMutex);

		assert_and_return_r(_storageFile.seekToEnd(), false);
		return DbRecordSerializer<Record>::serialize(record, _storageFile);
	}

private:
	static_assert(Record::isRecord());

private:
	StorageIO<StorageAdapter> _storageFile;
	std::mutex _storageMutex;
};
