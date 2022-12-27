#pragma once

#include "storage_io_interface.hpp"
#include "utility/static_data_buffer.hpp"

namespace io {

template <size_t MaxSize>
class StaticBufferAdapter
{
public:
	static constexpr auto MaxCapacity = MaxSize;
	constexpr bool open(const std::string& /*fileName*/, const OpenMode /*mode*/) noexcept
	{
		return true;
	}

	constexpr bool close() noexcept
	{
		_buffer.seek(0);
		return true;
	}

	constexpr bool reserve(const size_t nBytes) & noexcept
	{
		return _buffer.reserve(nBytes);
	}

	constexpr bool read(void* targetBuffer, const size_t dataSize) noexcept
	{
		return _buffer.read(targetBuffer, dataSize);
	}

	constexpr bool write(const void* const buffer, const size_t dataSize) noexcept
	{
		return _buffer.write(buffer, dataSize);
	}

	// Sets the absolute position from the beginning of the file
	constexpr bool seek(const size_t position) & noexcept
	{
		_buffer.seek(position);
		return true;
	}

	constexpr bool seekToEnd() & noexcept
	{
		return seek(_buffer.size());
	}

	[[nodiscard]] constexpr uint64_t pos() const noexcept
	{
		return _buffer.pos();
	}

	[[nodiscard]] constexpr uint64_t size() const noexcept
	{
		return _buffer.size();
	}

	[[nodiscard]] constexpr bool atEnd() const noexcept
	{
		return _buffer.pos() == _buffer.size();
	}

	[[nodiscard]] constexpr bool remainingCapacity() const noexcept
	{
		return MaxSize - _buffer.pos();
	}

	constexpr bool flush() noexcept
	{
		return true;
	}

	constexpr bool clear() noexcept
	{
		_buffer.reserve(0);
		_buffer.seek(0);
		return true;
	}

	[[nodiscard]] constexpr const auto* data() const & noexcept
	{
		return _buffer.data();
	}

	[[nodiscard]] constexpr auto* data() & noexcept
	{
		return _buffer.data();
	}

private:
	static_data_buffer<MaxSize> _buffer;
};

class MemoryBlockAdapter
{
public:
	inline MemoryBlockAdapter(const void* dataPtr, const size_t size) noexcept :
		_data{ reinterpret_cast<const std::byte*>(dataPtr) },
		_dataSize{size}
	{}

	inline constexpr bool read(void* targetBuffer, const size_t dataSize) noexcept
	{
		assert_and_return_r(dataSize <= _dataSize - _pos, false);
		::memcpy(targetBuffer, _data + _pos, dataSize);
		_pos += dataSize;
		return true;
	}

	// Sets the absolute position from the beginning of the file
	inline constexpr bool seek(const size_t position) & noexcept
	{
		assert_and_return_r(position < _dataSize, false);
		_pos = position;
		return true;
	}

	inline constexpr bool seekToEnd() & noexcept
	{
		return seek(size());
	}

	[[nodiscard]] inline constexpr uint64_t pos() const noexcept
	{
		return _pos;
	}

	[[nodiscard]] inline constexpr uint64_t size() const noexcept
	{
		return _dataSize;
	}

	[[nodiscard]] inline constexpr bool atEnd() const noexcept
	{
		return pos() == size();
	}

	inline constexpr bool flush() noexcept
	{
		return true;
	}

private:
	const std::byte* _data;
	const size_t _dataSize;
	size_t _pos = 0;
};

}
