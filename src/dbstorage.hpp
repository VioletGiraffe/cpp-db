#pragma once
#include "dbfield.hpp"
#include "assert/advanced_assert.h"
#include "std_helpers/qt_string_helpers.hpp"

#include <QFile>

#include <limits>
#include <optional>
#include <string>

namespace DBStorage {

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
bool DBStorage::write(const Field<T, id>& field, QFile& storageFile, const std::optional<uint64_t> offset)
{
	if (offset)
		assert_and_return_r(storageFile.seek(static_cast<qint64>(*offset)), false);

	assert(field.fieldSize() == sizeof(T));
	assert_and_return_r(storageFile.write(reinterpret_cast<const char*>(&field.value), sizeof(T)) == sizeof(T), false);

	return true;
}

template <auto id>
bool DBStorage::write(const Field<std::string, id>& field, QFile& storageFile, const std::optional<uint64_t> offset)
{
	if (offset)
		assert_and_return_r(storageFile.seek(static_cast<qint64>(*offset)), false);
	const uint32_t dataSize = static_cast<uint32_t>(field.value.size());
	assert_and_return_r(storageFile.write(reinterpret_cast<const char*>(&dataSize), sizeof(dataSize)) == sizeof(dataSize), false);
	assert_and_return_r(storageFile.write(field.value.data(), dataSize) == dataSize, false);

	return true;
}

template <typename T, auto id>
bool DBStorage::read(Field<T, id>& field, QFile& storageFile, const std::optional<uint64_t> offset)
{
	if (offset)
		assert_and_return_r(storageFile.seek(static_cast<qint64>(*offset)), false);
	assert(field.fieldSize() == sizeof(T));
	assert_and_return_r(storageFile.read(reinterpret_cast<char*>(&field.value), sizeof(T)) == sizeof(T), false);

	return true;
}

template <auto id>
bool DBStorage::read(Field<std::string, id>& field, QFile& storageFile, const std::optional<uint64_t> offset)
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
