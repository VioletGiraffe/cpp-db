#pragma once

#include "io_base_definitions.hpp"
#include "../dbfield.hpp"

#include "assert/advanced_assert.h"
#include "utility/extra_type_traits.hpp"

#include <optional>
#include <string>
#include <string_view>

template <typename IOAdapter>
struct StorageIO
{
	constexpr StorageIO(IOAdapter& io) noexcept :
		_io{ io }
	{}

	~StorageIO() noexcept
	{
		assert_r(close());
	}

	[[nodiscard]] constexpr bool open(std::string_view filePath, const io::OpenMode mode) noexcept;
	[[nodiscard]] constexpr bool close() noexcept;

	template<typename T, auto id, bool isArray>
	[[nodiscard]] constexpr bool writeField(const Field<T, id, isArray>& field, const std::optional<uint64_t> position = {}) noexcept;
	template<typename T, auto id, bool isArray>
	[[nodiscard]] constexpr bool readField(Field<T, id, isArray>& field, const std::optional<uint64_t> position = {}) noexcept;

	template <typename T, sfinae<!std::is_pointer_v<T>&& is_trivially_serializable_v<T>> = true>
	[[nodiscard]] constexpr bool read(T& value, const std::optional<uint64_t> position = {}) noexcept;
	template <typename T, sfinae<!std::is_pointer_v<T>&& is_trivially_serializable_v<T>> = true>
	[[nodiscard]] constexpr bool write(const T& value, const std::optional<uint64_t> position = {}) noexcept;

	[[nodiscard]] constexpr bool read(std::string& value, const std::optional<uint64_t> position = {}) noexcept;
	[[nodiscard]] constexpr bool write(const std::string& value, const std::optional<uint64_t> position = {}) noexcept;

	template <typename T>
	[[nodiscard]] constexpr bool read(std::vector<T>& v, const std::optional<uint64_t> position = {}) noexcept;
	template <typename T>
	[[nodiscard]] constexpr bool write(const std::vector<T>& v, const std::optional<uint64_t> position = {}) noexcept;

	[[nodiscard]] constexpr bool read(void* const dataPtr, uint64_t size) noexcept;
	[[nodiscard]] constexpr bool write(const void* const dataPtr, uint64_t size) noexcept;

	constexpr bool flush() noexcept;
	[[nodiscard]] constexpr bool seek(uint64_t location) noexcept;
	[[nodiscard]] constexpr bool seekToEnd() noexcept;
	[[nodiscard]] constexpr uint64_t pos() const noexcept;

	[[nodiscard]] constexpr uint64_t size() noexcept;
	[[nodiscard]] constexpr bool atEnd() noexcept;

	[[nodiscard]] constexpr bool clear() noexcept;

private:
	template <typename T>
	constexpr bool checkedWrite(T&& value)
	{
		static_assert(is_trivially_serializable_v<remove_cv_and_reference_t<T>>);
		return _io.write(std::addressof(value), sizeof(T));
	}

	template <typename T>
	constexpr bool checkedRead(T& value)
	{
		static_assert(is_trivially_serializable_v<remove_cv_and_reference_t<T>>);
		return _io.read(std::addressof(value), sizeof(T));
	}

private:
	IOAdapter& _io;
};

template<typename IOAdapter>
constexpr bool StorageIO<IOAdapter>::open(std::string_view filePath, const io::OpenMode mode) noexcept
{
	return _io.open(std::move(filePath), mode);
}

template<typename IOAdapter>
constexpr bool StorageIO<IOAdapter>::close() noexcept
{
	return _io.close();
}

template<typename IOAdapter>
template<typename T, auto id, bool isArray>
constexpr bool StorageIO<IOAdapter>::writeField(const Field<T, id, isArray>& field, const std::optional<uint64_t> position) noexcept
{
	if constexpr (field.sizeKnownAtCompileTime())
		static_assert(sizeof(std::remove_reference_t<T>) == field.staticSize());

	return write(field.value, position);
}

template<typename IOAdapter>
template<typename T, auto id, bool isArray>
constexpr bool StorageIO<IOAdapter>::readField(Field<T, id, isArray>& field, const std::optional<uint64_t> position) noexcept
{
	if constexpr (field.sizeKnownAtCompileTime())
		static_assert(sizeof(std::remove_reference_t<T>) == field.staticSize());

	return read(field.value, position);
}

template<typename IOAdapter>
template<typename T, sfinae<!std::is_pointer_v<T>&& is_trivially_serializable_v<T>>>
constexpr bool StorageIO<IOAdapter>::read(T& value, const std::optional<uint64_t> position) noexcept
{
	if (position)
		assert_and_return_r(_io.seek(*position), false);

	return checkedRead(value);
}

template<typename IOAdapter>
template<typename T, sfinae<!std::is_pointer_v<T>&& is_trivially_serializable_v<T>>>
constexpr bool StorageIO<IOAdapter>::write(const T& value, const std::optional<uint64_t> position) noexcept
{
	if (position)
		assert_and_return_r(_io.seek(*position), false);

	return checkedWrite(value);
}

template<typename IOAdapter>
constexpr bool StorageIO<IOAdapter>::read(std::string& str, const std::optional<uint64_t> position) noexcept
{
	if (position)
		assert_and_return_r(_io.seek(*position), false);

	uint32_t dataSize = 0;
	assert_and_return_r(checkedRead(dataSize), false);

	str.resize(static_cast<size_t>(dataSize), '\0'); // This should automatically support empty strings of length 0 as well.
	return _io.read(str.data(), dataSize);
}

template<typename IOAdapter>
constexpr bool StorageIO<IOAdapter>::write(const std::string& str, const std::optional<uint64_t> position) noexcept
{
	if (position)
		assert_and_return_r(_io.seek(*position), false);

	const uint32_t dataSize = static_cast<uint32_t>(str.size());
	assert_and_return_r(checkedWrite(dataSize), false);

	return _io.write(str.data(), dataSize);
}

template<typename IOAdapter>
template <typename T>
constexpr bool StorageIO<IOAdapter>::read(std::vector<T>& v, const std::optional<uint64_t> position) noexcept
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
constexpr bool StorageIO<IOAdapter>::write(const std::vector<T>& v, const std::optional<uint64_t> position) noexcept
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
inline constexpr bool StorageIO<IOAdapter>::read(void* const dataPtr, uint64_t size) noexcept
{
	return _io.read(dataPtr, size);
}

template<typename IOAdapter>
inline constexpr bool StorageIO<IOAdapter>::write(const void* const dataPtr, uint64_t size) noexcept
{
	return _io.write(dataPtr, size);
}

template<typename IOAdapter>
inline constexpr bool StorageIO<IOAdapter>::seek(uint64_t location) noexcept
{
	return _io.seek(location);
}

template<typename IOAdapter>
inline constexpr bool StorageIO<IOAdapter>::seekToEnd() noexcept
{
	return _io.seekToEnd();
}

template<typename IOAdapter>
inline constexpr bool StorageIO<IOAdapter>::flush() noexcept
{
	return _io.flush();
}

template<typename IOAdapter>
inline constexpr uint64_t StorageIO<IOAdapter>::pos() const noexcept
{
	return _io.pos();
}

template<typename IOAdapter>
inline constexpr uint64_t StorageIO<IOAdapter>::size() noexcept
{
	return _io.size();
}

template<typename IOAdapter>
inline constexpr bool StorageIO<IOAdapter>::atEnd() noexcept
{
	return _io.atEnd();
}

template<typename IOAdapter>
inline constexpr bool StorageIO<IOAdapter>::clear() noexcept
{
	return _io.clear();
}
