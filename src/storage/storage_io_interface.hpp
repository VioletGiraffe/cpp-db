#pragma once

#include "storage_helpers.hpp"
#include "../dbfield.hpp"
#include "assert/advanced_assert.h"

#include <optional>
#include <string>

namespace io {
	enum class OpenMode { Read, Write, ReadWrite };
}

template <typename IOAdapter>
struct StorageIO
{
	bool open(std::string filePath, const io::OpenMode mode);

	template <typename T, auto id>
	bool writeField(const Field<T, id>& field, const std::optional<uint64_t> position = {}) noexcept;

	template <typename T, auto id>
	bool readField(Field<T, id>& field, const std::optional<uint64_t> position = {}) noexcept;

	template <typename T>
	bool read(T& value, const std::optional<uint64_t> position = {}) noexcept;

	template <typename T>
	bool write(T&& value, const std::optional<uint64_t> position = {}) noexcept;

	bool read(std::string& value, const std::optional<uint64_t> position = {}) noexcept;

	bool write(const std::string& value, const std::optional<uint64_t> position = {}) noexcept;

	bool flush();

	uint64_t pos() const noexcept;
	uint64_t size() const noexcept;

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
bool StorageIO<IOAdapter>::open(std::string filePath, const io::OpenMode mode)
{
	return _io.open(std::move(filePath), mode);
}

template<typename IOAdapter>
template<typename T, auto id>
bool StorageIO<IOAdapter>::writeField(const Field<T, id>& field, const std::optional<uint64_t> position) noexcept
{
	static_assert(sizeof(std::remove_reference_t<T>) == field.staticSize());
	return write(field.value, position);
}

template<typename IOAdapter>
template<typename T, auto id>
bool StorageIO<IOAdapter>::readField(Field<T, id>& field, const std::optional<uint64_t> position) noexcept
{
	static_assert(sizeof(std::remove_reference_t<T>) == field.staticSize());
	return read(field.value, position);
}

template<typename IOAdapter>
template<typename T>
bool StorageIO<IOAdapter>::read(T& value, const std::optional<uint64_t> position) noexcept
{
	if (position)
		assert_and_return_r(_io.seek(*position), false);

	return checkedRead(value);
}

template<typename IOAdapter>
template<typename T>
bool StorageIO<IOAdapter>::write(T&& value, const std::optional<uint64_t> position) noexcept
{
	if (position)
		assert_and_return_r(_io.seek(*position), false);

	return checkedWrite(std::forward<T>(value));
}

template<typename IOAdapter>
bool StorageIO<IOAdapter>::read(std::string& str, const std::optional<uint64_t> position) noexcept
{
	if (position)
		assert_and_return_r(_io.seek(*position), false);

	uint32_t dataSize = 0;
	assert_and_return_r(checkedRead(dataSize), false);

	str.resize(static_cast<size_t>(dataSize), '\0'); // This should automatically support empty strings of length 0 as well.
	return _io.read(str.data(), dataSize);
}

template<typename IOAdapter>
bool StorageIO<IOAdapter>::write(const std::string& str, const std::optional<uint64_t> position) noexcept
{
	if (position)
		assert_and_return_r(_io.seek(*position), false);

	const uint32_t dataSize = static_cast<uint32_t>(str.size());
	assert_and_return_r(checkedWrite(dataSize), false);

	return _io.write(str.data(), dataSize);
}

template<typename IOAdapter>
bool StorageIO<IOAdapter>::flush()
{
	return _io.flush();
}

template<typename IOAdapter>
uint64_t StorageIO<IOAdapter>::pos() const noexcept
{
	return _io.pos();
}

template<typename IOAdapter>
uint64_t StorageIO<IOAdapter>::size() const noexcept
{
	return _io.size();
}
