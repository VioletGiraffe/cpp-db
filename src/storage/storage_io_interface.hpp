#pragma once

#include "storage_helpers.hpp"
#include "../dbfield.hpp"

#include "hash/fnv_1a.h"
#include "assert/advanced_assert.h"
#include "utility/extra_type_traits.hpp"

#include <optional>
#include <string>

namespace io {
	enum class OpenMode { Read, Write, ReadWrite };
}

template <typename IOAdapter>
struct StorageIO
{
	[[nodiscard]] bool open(std::string filePath, const io::OpenMode mode) noexcept;

	template<typename T, auto id, bool isArray>
	[[nodiscard]] bool writeField(const Field<T, id, isArray>& field, const std::optional<uint64_t> position = {}) noexcept;
	template<typename T, auto id, bool isArray>
	[[nodiscard]] bool readField(Field<T, id, isArray>& field, const std::optional<uint64_t> position = {}) noexcept;

	template <typename T, typename = std::enable_if_t<!std::is_pointer_v<T>&& is_trivially_serializable_v<T>>>
	[[nodiscard]] bool read(T& value, const std::optional<uint64_t> position = {}) noexcept;
	template <typename T, typename = std::enable_if_t<!std::is_pointer_v<T>&& is_trivially_serializable_v<T>>>
	[[nodiscard]] bool write(const T& value, const std::optional<uint64_t> position = {}) noexcept;

	[[nodiscard]] bool read(std::string& value, const std::optional<uint64_t> position = {}) noexcept;
	[[nodiscard]] bool write(const std::string& value, const std::optional<uint64_t> position = {}) noexcept;

	template <typename T>
	[[nodiscard]] bool read(std::vector<T>& v, const std::optional<uint64_t> position = {}) noexcept;
	template <typename T>
	[[nodiscard]] bool write(const std::vector<T>& v, const std::optional<uint64_t> position = {}) noexcept;

	[[nodiscard]] bool read(void* const dataPtr, uint64_t size) noexcept;
	[[nodiscard]] bool write(const void* const dataPtr, uint64_t size) noexcept;

	bool flush() noexcept;
	[[nodiscard]] bool seek(uint64_t location) noexcept;
	[[nodiscard]] bool seekToEnd() noexcept;
	[[nodiscard]] uint64_t pos() const noexcept;

	[[nodiscard]] uint64_t size() const noexcept;
	[[nodiscard]] bool atEnd() const noexcept;

	[[nodiscard]] bool clear() noexcept;

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
bool StorageIO<IOAdapter>::open(std::string filePath, const io::OpenMode mode) noexcept
{
	return _io.open(std::move(filePath), mode);
}

template<typename IOAdapter>
template<typename T, auto id, bool isArray>
bool StorageIO<IOAdapter>::writeField(const Field<T, id, isArray>& field, const std::optional<uint64_t> position) noexcept
{
	if constexpr (field.sizeKnownAtCompileTime()) static_assert(sizeof(std::remove_reference_t<T>) == field.staticSize());
	return write(field.value, position);
}

template<typename IOAdapter>
template<typename T, auto id, bool isArray>
bool StorageIO<IOAdapter>::readField(Field<T, id, isArray>& field, const std::optional<uint64_t> position) noexcept
{
	if constexpr (field.sizeKnownAtCompileTime()) static_assert(sizeof(std::remove_reference_t<T>) == field.staticSize());
	return read(field.value, position);
}

template<typename IOAdapter>
template<typename T, typename>
bool StorageIO<IOAdapter>::read(T& value, const std::optional<uint64_t> position) noexcept
{
	if (position)
		assert_and_return_r(_io.seek(*position), false);

	return checkedRead(value);
}

template<typename IOAdapter>
template<typename T, typename>
bool StorageIO<IOAdapter>::write(const T& value, const std::optional<uint64_t> position) noexcept
{
	if (position)
		assert_and_return_r(_io.seek(*position), false);

	return checkedWrite(value);
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
template <typename T>
bool StorageIO<IOAdapter>::read(std::vector<T>& v, const std::optional<uint64_t> position) noexcept
{
	if (position)
		assert_and_return_r(_io.seek(*position), false);

	uint32_t nItems = 0;
	assert_and_return_r(checkedRead(nItems), false);

	v.resize(nItems);
	for (size_t i = 0; i < nItems; ++i)
	{
		if (this->read(v[i]) == false)
			return false;
	}

	return true;
}

template<typename IOAdapter>
template <typename T>
bool StorageIO<IOAdapter>::write(const std::vector<T>& v, const std::optional<uint64_t> position) noexcept
{
	if (position)
		assert_and_return_r(_io.seek(*position), false);

	const uint32_t nItems = static_cast<uint32_t>(v.size());
	assert_and_return_r(checkedWrite(nItems), false);

	for (size_t i = 0; i < nItems; ++i)
	{
		if (this->write(v[i]) == false)
			return false;
	}

	return true;
}

template<typename IOAdapter>
bool StorageIO<IOAdapter>::read(void* const dataPtr, uint64_t size) noexcept
{
	return _io.read(dataPtr, size);
}

template<typename IOAdapter>
bool StorageIO<IOAdapter>::write(const void* const dataPtr, uint64_t size) noexcept
{
	return _io.write(dataPtr, size);
}

template<typename IOAdapter>
bool StorageIO<IOAdapter>::seek(uint64_t location) noexcept
{
	return _io.seek(location);
}

template<typename IOAdapter>
bool StorageIO<IOAdapter>::seekToEnd() noexcept
{
	return _io.seekToEnd();
}

template<typename IOAdapter>
bool StorageIO<IOAdapter>::flush() noexcept
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

template<typename IOAdapter>
bool StorageIO<IOAdapter>::atEnd() const noexcept
{
	return _io.atEnd();
}

template<typename IOAdapter>
inline bool StorageIO<IOAdapter>::clear() noexcept
{
	return _io.clear();
}
