#pragma once

#include "dbfield.hpp"
#include "storage/storage_helpers.hpp"
#include "assert/advanced_assert.h"

#include <QIODevice>

#include <optional>
#include <string>

namespace StorageIO {
	template <typename T, auto id>
	bool write(const Field<T, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset = {});

	template <auto id>
	bool write(const Field<std::string, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset = {});

	template <typename T, auto id>
	bool read(Field<T, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset = {});

	template <auto id>
	bool read(Field<std::string, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset = {});
}

template <typename T, auto id>
bool StorageIO::write(const Field<T, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset)
{
	if (offset)
		assert_and_return_r(storageDevice.seek(static_cast<qint64>(*offset)), false);

	static_assert(field.sizeKnownAtCompileTime(), "The field must have static size for this instantiation!");
	constexpr auto size = field.staticSize();
	assert_and_return_r(checkedWrite(storageDevice, field.value), false);

	return true;
}

template <auto id>
bool StorageIO::write(const Field<std::string, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset)
{
	if (offset)
		assert_and_return_r(storageDevice.seek(static_cast<qint64>(*offset)), false);
	const uint32_t dataSize = static_cast<uint32_t>(field.value.size());
	assert_and_return_r(checkedWrite(storageDevice, dataSize), false);
	assert_and_return_r(storageDevice.write(field.value.data(), dataSize) == dataSize, false);

	return true;
}

template <typename T, auto id>
bool StorageIO::read(Field<T, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset)
{
	if (offset)
		assert_and_return_r(storageDevice.seek(static_cast<qint64>(*offset)), false);

	static_assert(field.sizeKnownAtCompileTime(), "The field must have static size for this instantiation!");
	constexpr auto size = field.staticSize();
	assert_and_return_r(checkedRead(storageDevice, field.value), false);

	return true;
}

template <auto id>
bool StorageIO::read(Field<std::string, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset)
{
	if (offset)
		assert_and_return_r(storageDevice.seek(static_cast<qint64>(*offset)), false);
	uint32_t dataSize = 0;
	assert_and_return_r(checkedRead(storageDevice, dataSize), false);
	assert(dataSize > 0);

	field.value.resize(static_cast<size_t>(dataSize), '\0');
	assert_and_return_r(storageDevice.read(field.value.data(), dataSize) == dataSize, false);

	return true;
}
