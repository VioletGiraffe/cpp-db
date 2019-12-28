#pragma once

#include "dbfield.hpp"
#include "assert/advanced_assert.h"

#include <QFile>

#include <optional>
#include <string>

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
