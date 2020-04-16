#pragma once

#include "storage_io_interface.hpp"
#include "storage_helpers.hpp"

#include <QIODevice>

#include <type_traits>

template <>
struct StorageIO<QIODevice>
{
	template <typename T, auto id>
	static bool writeField(const Field<T, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;

	template <typename T, auto id>
	static bool readField(Field<T, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;

	template <typename T>
	static bool read(T& value, QIODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;

	template <typename T>
	static bool write(T&& value, QIODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;

	static bool read(std::string& value, QIODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;

	static bool write(const std::string& value, QIODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;
};

using StorageQt = StorageIO<QIODevice>;

template <typename T, auto id>
bool StorageIO<QIODevice>::writeField(const Field<T, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset) noexcept
{
	static_assert(sizeof(std::remove_reference_t<T>) == field.staticSize());
	return write(field.value, storageDevice, offset);
}

template <typename T, auto id>
bool StorageIO<QIODevice>::readField(Field<T, id>& field, QIODevice& storageDevice, const std::optional<uint64_t> offset) noexcept
{
	static_assert(sizeof(std::remove_reference_t<T>) == field.staticSize());
	return read(field.value, storageDevice, offset);
}

template<typename T>
bool StorageIO<QIODevice>::read(T& value, QIODevice &storageDevice, const std::optional<uint64_t> offset) noexcept
{
	if (offset)
		assert_and_return_r(storageDevice.seek(static_cast<qint64>(*offset)), false);

	return ::checkedRead(storageDevice, value);
}

template<typename T>
bool StorageIO<QIODevice>::write(T&& value, QIODevice &storageDevice, const std::optional<uint64_t> offset) noexcept
{
	if (offset)
		assert_and_return_r(storageDevice.seek(static_cast<qint64>(*offset)), false);

	return ::checkedWrite(storageDevice, std::forward<T>(value));
}

inline bool StorageIO<QIODevice>::read(std::string& str, QIODevice& storageDevice, const std::optional<uint64_t> offset) noexcept
{
	if (offset)
		assert_and_return_r(storageDevice.seek(static_cast<qint64>(*offset)), false);

	uint32_t dataSize = 0;
	assert_and_return_r(checkedRead(storageDevice, dataSize), false);

	str.resize(static_cast<size_t>(dataSize), '\0'); // This should automatically support empty strings of length 0 as well.
	return storageDevice.read(str.data(), dataSize) == dataSize;
}

inline bool StorageIO<QIODevice>::write(const std::string& str, QIODevice& storageDevice, const std::optional<uint64_t> offset) noexcept
{
	if (offset)
		assert_and_return_r(storageDevice.seek(static_cast<qint64>(*offset)), false);

	const uint32_t dataSize = static_cast<uint32_t>(str.size());
	assert_and_return_r(checkedWrite(storageDevice, dataSize), false);

	return storageDevice.write(str.data(), dataSize) == dataSize;
}
