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

	[[nodiscard]] inline constexpr StorageLocation(uint64_t loc) noexcept : location{ loc } {}

	[[nodiscard]] inline constexpr bool operator==(const StorageLocation& other) const noexcept {
		return location == other.location;
	}

	[[nodiscard]] inline constexpr bool operator!=(const StorageLocation& other) const noexcept {
		return !operator==(other);
	}

	[[nodiscard]] inline constexpr bool operator<(const StorageLocation& other) const noexcept {
		return location < other.location;
	}
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
		return deserializeRecord(record, _storageFile);
	}

	[[nodiscard]] bool writeRecord(const Record& record, const StorageLocation recordStartLocation)
	{
		std::lock_guard locker(_storageMutex);

		assert_and_return_r(_storageFile.seek(recordStartLocation.location), false);
		return serializeRecord(record, _storageFile);
	}

private:
	static_assert(Record::isRecord());

private:
	StorageIO<StorageAdapter> _storageFile;
	std::mutex _storageMutex;
};
