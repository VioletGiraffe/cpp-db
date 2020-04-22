#pragma once

#include "dbrecord.hpp"
#include "storage/storage_io_interface.hpp"
#include "assert/advanced_assert.h"
#include "parameter_pack/parameter_pack_helpers.hpp"
#include "utility/extra_type_traits.hpp"

#include <mutex>
#include <optional>
#include <string>
#include <string.h> // memcpy

struct StorageLocation {
	uint64_t location;

	inline constexpr StorageLocation(uint64_t loc) noexcept : location{ loc } {}

	inline constexpr bool operator==(const StorageLocation& other) const noexcept {
		return location == other.location;
	}

	inline constexpr bool operator!=(const StorageLocation& other) const noexcept {
		return !operator==(other);
	}

	inline constexpr bool operator<(const StorageLocation& other) const noexcept {
		return location < other.location;
	}
};

template <typename StorageAdapter, typename Record>
class DBStorage
{
public:
	bool openStorageFile(const std::string& filePath)
	{
		return _storageFile.open(filePath, io::OpenMode::ReadWrite);
	}

	bool readRecord(Record& record, const StorageLocation recordStartLocation)
	{
		std::lock_guard locker(_storageMutex);

		assert_r(_storageFile.seek(recordStartLocation.location));

		// Statically sized fields are grouped before others and can be read or written in a single block.
		static constexpr size_t staticFieldsSize = record.staticFieldsSize();
		std::array<uint8_t, staticFieldsSize> buffer;

		assert_and_return_r(_storageFile.read(buffer.data(), staticFieldsSize), false);

		size_t bufferOffset = 0;
		bool success = true;
		static_for<0, Record::fieldCount()>([&](auto i) {
			using FieldType = typename Record::template FieldTypeByIndex_t<i>;

			auto& field = record.template fieldAt<i>();
			static_assert(std::is_same_v<std::remove_cv_t<FieldType>, remove_cv_and_reference_t<decltype(field)>>);

			if constexpr (FieldType::sizeKnownAtCompileTime())
			{
				static_assert(is_trivially_serializable_v<FieldType>);
				::memcpy(std::addressof(field.value), buffer.data() + bufferOffset, FieldType::staticSize());
				bufferOffset += FieldType::staticSize();
			}
			else
			{
				if (!success)
					return;

				if (!_storageFile.read(field))
				{
					assert_unconditional_r("Failed to read data!");
					success = false;
				}
			}
		});

		return success;
	}

	bool writeRecord(const Record& record, const StorageLocation recordStartLocation)
	{
		std::lock_guard locker(_storageMutex);

		assert_r(_storageFile.seek(recordStartLocation.location));

		// Statically sized fields are grouped before others and can be read or written in a single block.
		static constexpr size_t staticFieldsSize = record.staticFieldsSize();
		std::array<uint8_t, staticFieldsSize> buffer;

		size_t bufferOffset = 0;
		static_for<0, record.staticFieldsCount()>([&](auto i) {
			using FieldType = typename Record::template FieldTypeByIndex_t<i>;

			const auto& field = record.template fieldAt<i>();
			static_assert(std::is_same_v<std::remove_cv_t<FieldType>, remove_cv_and_reference_t<decltype(field)>>);
			static_assert(is_trivially_serializable_v<FieldType>);
			static_assert(FieldType::sizeKnownAtCompileTime());

			::memcpy(buffer.data() + bufferOffset, std::addressof(field.value), FieldType::staticSize());
			bufferOffset += FieldType::staticSize();
		});

		assert_r(bufferOffset == buffer.size());
		assert_and_return_r(_storageFile.write(buffer.data(), staticFieldsSize), false);

		bool success = true;
		static_for<record.staticFieldsCount(), record.fieldCount()>([&](auto i) {
			if (!success)
				return;

			using FieldType = typename Record::template FieldTypeByIndex_t<i>;

			const auto& field = record.template fieldAt<i>();
			static_assert(std::is_same_v<std::remove_cv_t<FieldType>, remove_cv_and_reference_t<decltype(field)>>);
			static_assert(!FieldType::sizeKnownAtCompileTime());

			if (!_storageFile.write(field))
			{
				assert_unconditional_r("Failed to write data!");
				success = false;
			}
		});

		return success;
	}

private:
	static_assert(Record::isRecord());

private:
	StorageIO<StorageAdapter> _storageFile;
	std::mutex _storageMutex;
};
