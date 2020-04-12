#pragma once

#include "storage_io_interface.hpp"

#include <QIODevice>

template <>
struct StorageIO<QIODevice>
{
	template <typename T, auto id>
	static bool writeField(const Field<T, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;

	template <auto id>
	static bool writeField(const Field<std::string, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;

	template <typename T, auto id>
	static bool readField(Field<T, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;

	template <auto id>
	static bool readField(Field<std::string, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;

	template <typename T>
	static bool read(T& value, QIODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;

	template <typename T>
	static bool write(T&& value, QIODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;
};

using StorageQt = StorageIO<QIODevice>;

template <typename T, auto id>
bool StorageIO<QIODevice>::writeField(const Field<T, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset) noexcept
{
	if (offset)
		assert_and_return_r(storageDevice.seek(static_cast<qint64>(*offset)), false);

	static_assert (std::is_trivial_v<std::remove_reference_t<T>>);
	static_assert(field.sizeKnownAtCompileTime(), "The field must have static size for this instantiation!");
	constexpr auto size = field.staticSize();
	assert_and_return_r(checkedWrite(storageDevice, field.value), false);

	return true;
}

template <auto id>
bool StorageIO<QIODevice>::writeField(const Field<std::string, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset) noexcept
{
	if (offset)
		assert_and_return_r(storageDevice.seek(static_cast<qint64>(*offset)), false);

	const uint32_t dataSize = static_cast<uint32_t>(field.value.size());
	assert_and_return_r(checkedWrite(storageDevice, dataSize), false);
	assert_and_return_r(storageDevice.write(field.value.data(), dataSize) == dataSize, false);

	return true;
}

template <typename T, auto id>
bool StorageIO<QIODevice>::readField(Field<T, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset) noexcept
{
	if (offset)
		assert_and_return_r(storageDevice.seek(static_cast<qint64>(*offset)), false);

	static_assert(field.sizeKnownAtCompileTime(), "The field must have static size for this instantiation!");
	constexpr auto size = field.staticSize();
	assert_and_return_r(checkedRead(storageDevice, field.value), false);

	return true;
}

template <auto id>
bool StorageIO<QIODevice>::readField(Field<std::string, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset) noexcept
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

template<typename T>
bool StorageIO<QIODevice>::read(T& value, QIODevice &storageDevice, const std::optional<uint64_t> offset) noexcept
{
	if (offset)
		assert_and_return_r(storageDevice.seek(static_cast<qint64>(*offset)), false);

	assert_and_return_r(checkedRead(storageDevice, value), false);
	return true;
}

template<typename T>
bool StorageIO<QIODevice>::write(T&& value, QIODevice &storageDevice, const std::optional<uint64_t> offset) noexcept
{
	if (offset)
		assert_and_return_r(storageDevice.seek(static_cast<qint64>(*offset)), false);

	assert_and_return_r(checkedWrite(storageDevice, std::forward<T>(value)), false);
	return true;
}
