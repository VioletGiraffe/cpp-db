#pragma once

#include "storage_helpers.hpp"
#include "../dbfield.hpp"
#include "assert/advanced_assert.h"

#include <optional>
#include <string>

template <typename IOAdapter>
struct StorageIO
{
	template <typename T, auto id>
	static bool writeField(const Field<T, id>& field, const std::optional<uint64_t> position = {}) noexcept;

	template <typename T, auto id>
	static bool readField(Field<T, id>& field, const std::optional<uint64_t> position = {}) noexcept;

	template <typename T>
	static bool read(T& value, const std::optional<uint64_t> position = {}) noexcept;

	template <typename T>
	static bool write(T&& value, const std::optional<uint64_t> position = {}) noexcept;

	static bool read(std::string& value, const std::optional<uint64_t> position = {}) noexcept;

	static bool write(const std::string& value, const std::optional<uint64_t> position = {}) noexcept;

	static bool read(void* destinationBuffer, const size_t bytesToRead, const std::optional<uint64_t> position = {}) noexcept;
	static bool write(const void const* sourceBuffer, const size_t bytesToWrite, const std::optional<uint64_t> position = {}) noexcept;

private:
	template <typename T>
	bool checkedWrite(T&& value)
	{
		static_assert(is_trivially_serializable_v<remove_cv_and_reference_t<T>>);
		return _io.write(std::addressof(value), sizeof(T));
	}

	template <typename T>
	bool checkedRead(T& value)
	{
		static_assert(is_trivially_serializable_v<remove_cv_and_reference_t<T>>);
		return _io.read(std::addressof(value), sizeof(T));
	}

private:
	IOAdapter _io;
};

template<typename IOAdapter>
template<typename T, auto id>
inline bool StorageIO<IOAdapter>::writeField(const Field<T, id>& field, const std::optional<uint64_t> position) noexcept
{
	static_assert(sizeof(std::remove_reference_t<T>) == field.staticSize());
	return write(field.value, position);
}

template<typename IOAdapter>
template<typename T, auto id>
inline bool StorageIO<IOAdapter>::readField(Field<T, id>& field, const std::optional<uint64_t> position) noexcept
{
	static_assert(sizeof(std::remove_reference_t<T>) == field.staticSize());
	return read(field.value, position);
}

template<typename IOAdapter>
template<typename T>
inline bool StorageIO<IOAdapter>::read(T& value, const std::optional<uint64_t> position) noexcept
{
	if (position)
		assert_and_return_r(storageDevice.seek(*position), false);

	return checkedRead(storageDevice, value);
}

template<typename IOAdapter>
template<typename T>
inline bool StorageIO<IOAdapter>::write(T&& value, const std::optional<uint64_t> position) noexcept
{
	if (position)
		assert_and_return_r(storageDevice.seek(*position), false);

	return checkedWrite(storageDevice, std::forward<T>(value));
}

template<typename IOAdapter>
inline bool StorageIO<IOAdapter>::read(std::string& value, const std::optional<uint64_t> position) noexcept
{
	if (position)
		assert_and_return_r(storageDevice.seek(*position), false);

	uint32_t dataSize = 0;
	assert_and_return_r(checkedRead(storageDevice, dataSize), false);

	str.resize(static_cast<size_t>(dataSize), '\0'); // This should automatically support empty strings of length 0 as well.
	return storageDevice.read(str.data(), dataSize);
}

template<typename IOAdapter>
inline bool StorageIO<IOAdapter>::write(const std::string& value, const std::optional<uint64_t> position) noexcept
{
	if (position)
		assert_and_return_r(storageDevice.seek(*position), false);

	const uint32_t dataSize = static_cast<uint32_t>(str.size());
	assert_and_return_r(checkedWrite(storageDevice, dataSize), false);

	return storageDevice.write(str.data(), dataSize);
}
