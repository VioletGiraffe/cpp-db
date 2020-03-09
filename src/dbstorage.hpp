#pragma once

#include "dbfield.hpp"
#include "storage/storage_qt.hpp"
#include "assert/advanced_assert.h"
#include "parameter_pack/parameter_pack_helpers.hpp"

#include <QFile>

#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <tuple>

struct StorageLocation {
	uint64_t location;
};

template <typename... Fields>
class DBStorage
{
	static constexpr size_t staticFieldsTotalSize()
	{
		size_t totalSize = 0;
		static_for<0, sizeof...(Fields)>([&totalSize](auto i) {
			using FieldType = pack::type_by_index<decltype(i)::value, Fields...>;
			if constexpr (FieldType::sizeKnownAtCompileTime() == true)
				totalSize += FieldType::staticSize();
		});

		return totalSize;
	}

	static constexpr bool checkFieldOrder()
	{
		if constexpr (sizeof...(Fields) == 1)
			return true;

		bool fixedSizeFieldsBeforeDynamicFields = true;
		static_for<1, sizeof...(Fields)>([&fixedSizeFieldsBeforeDynamicFields](auto i) {
			using Type0 = pack::type_by_index<decltype(i)::value - 1, Fields...>;
			using Type1 = pack::type_by_index<decltype(i)::value, Fields...>;

			fixedSizeFieldsBeforeDynamicFields = fixedSizeFieldsBeforeDynamicFields && !(Type0::sizeKnownAtCompileTime() == false && Type1::sizeKnownAtCompileTime() == true);
		});

		return fixedSizeFieldsBeforeDynamicFields;
	}

public:
	using Record = std::tuple<Fields...>;

	bool openStorageFile(const std::string& filePath)
	{
		_storageFile.setFileName(QString::fromStdString(filePath));
		return _storageFile.open(QFile::ReadWrite);
	}

	Record readRecord(uint64_t recordStartLocation)
	{
		std::lock_guard locker(_storageMutex);

		// TODO: move this code to DBStorage without splitting it into multiple file reads
		assert_r(_storageFile.seek(recordStartLocation));

		// No dynamic fields - can read in a single block.
		static constexpr auto staticFieldsSize = staticFieldsTotalSize();
		char buffer[staticFieldsSize];

		static_assert(checkFieldOrder(), "All the fields with static size must be listed before all the fields with runtime size!");
		// The above limitation enables reading all the static fields in a single block.
		assert_r(_storageFile.read(buffer, staticFieldsSize) == staticFieldsSize);

		Record record;
		size_t bufferOffset = 0;
		static_for<0, sizeof...(Fields)>([&](auto i) {
			constexpr auto index = decltype(i)::value;
			using FieldType = pack::type_by_index<index, Fields...>;
			if constexpr (FieldType::sizeKnownAtCompileTime())
			{
				memcpy(&std::get<index>(record), buffer + bufferOffset, FieldType::staticSize());
				bufferOffset += FieldType::staticSize();
			}
			else
			{
				StorageIO::read(std::get<index>(record), _storageFile);
			}
		});

		return record;
	}

	bool writeRecord(uint64_t recordStartLocation)
	{
		std::lock_guard locker(_storageMutex);
	}

private:
	QFile _storageFile;
	std::mutex _storageMutex;
};