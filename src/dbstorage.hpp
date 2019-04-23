#pragma once
#include "dbfield.hpp"
#include "assert/advanced_assert.h"
#include "parameter_pack/parameter_pack_helpers.hpp"

#include <QFile>

#include <limits>
#include <mutex>
#include <optional>
#include <string>
#include <tuple>

template <typename... Fields>
class DBStorage
{
	static constexpr size_t staticFieldsTotalSize()
	{
		size_t totalSize = 0;
		static_for<0, sizeof...(Fields)>([&totalSize](auto i) {
			using FieldType = pack::type_by_index<decltype(i)::value, Fields...>;
			if constexpr (FieldType::hasStaticSize() == true)
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

			fixedSizeFieldsBeforeDynamicFields = fixedSizeFieldsBeforeDynamicFields && !(Type0::hasStaticSize() == false && Type1::hasStaticSize() == true);
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
			if constexpr (FieldType::hasStaticSize())
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

namespace StorageIO {
	template <typename T, auto id>
	bool write(const Field<T, id>& field, QFile& storageFile, const std::optional<uint64_t> offset = std::nullopt);

	template <auto id>
	bool write(const Field<std::string, id>& field, QFile& storageFile, const std::optional<uint64_t> offset = std::nullopt);

	template <typename T, auto id>
	bool read(Field<T, id>& field, QFile& storageFile, const std::optional<uint64_t> offset = std::nullopt);

	template <auto id>
	bool read(Field<std::string, id>& field, QFile& storageFile, const std::optional<uint64_t> offset = std::nullopt);
}

template <typename T, auto id>
bool StorageIO::write(const Field<T, id>& field, QFile& storageFile, const std::optional<uint64_t> offset)
{
	if (offset)
		assert_and_return_r(storageFile.seek(static_cast<qint64>(*offset)), false);

	static_assert(field.hasStaticSize(), "The field must have static size for this instantiation!");
	constexpr auto size = field.staticSize();
	assert_and_return_r(storageFile.write(reinterpret_cast<const char*>(&field.value), size) == size, false);

	return true;
}

template <auto id>
bool StorageIO::write(const Field<std::string, id>& field, QFile& storageFile, const std::optional<uint64_t> offset)
{
	if (offset)
		assert_and_return_r(storageFile.seek(static_cast<qint64>(*offset)), false);
	const uint32_t dataSize = static_cast<uint32_t>(field.value.size());
	assert_and_return_r(storageFile.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize)) == sizeof(dataSize), false);
	assert_and_return_r(storageFile.write(field.value.data(), dataSize) == dataSize, false);

	return true;
}

template <typename T, auto id>
bool StorageIO::read(Field<T, id>& field, QFile& storageFile, const std::optional<uint64_t> offset)
{
	if (offset)
		assert_and_return_r(storageFile.seek(static_cast<qint64>(*offset)), false);

	static_assert(field.hasStaticSize(), "The field must have static size for this instantiation!");
	constexpr auto size = field.staticSize();
	assert_and_return_r(storageFile.read(reinterpret_cast<char*>(&field.value), size) == size, false);

	return true;
}

template <auto id>
bool StorageIO::read(Field<std::string, id>& field, QFile& storageFile, const std::optional<uint64_t> offset)
{
	if (offset)
		assert_and_return_r(storageFile.seek(static_cast<qint64>(*offset)), false);
	uint32_t dataSize = 0;
	assert_and_return_r(storageFile.read(reinterpret_cast<char*>(&dataSize), sizeof(dataSize)) == sizeof(dataSize), false);
	assert(dataSize > 0);

	field.value.resize(static_cast<size_t>(dataSize), '\0');
	assert_and_return_r(storageFile.read(field.value.data(), dataSize) == dataSize, false);

	return true;
}
