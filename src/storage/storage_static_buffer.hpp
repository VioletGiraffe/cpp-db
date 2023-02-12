#pragma once

#include "storage_io_interface.hpp"
#include "utility/static_data_buffer.hpp"

#include <mutex>
#include <vector>

namespace io {

template <size_t MaxSize>
class StaticBufferAdapter
{
public:
	static constexpr auto MaxCapacity = MaxSize;
	constexpr bool open(std::string_view /*fileName*/, const OpenMode /*mode*/) noexcept
	{
		assert_and_return_r(!_isOpen, false);
		_isOpen = true;
		return true;
	}

	constexpr bool close() noexcept
	{
		if (_isOpen)
		{
			_buffer.seek(0);
			_isOpen = false;
		}
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

	[[nodiscard]] constexpr size_t remainingCapacity() const noexcept
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
	bool _isOpen = false;
};

class VectorAdapter
{
public:
	inline VectorAdapter(const size_t reserve = 0) noexcept
	{
		_data.reserve(reserve);
	}

	constexpr bool open(std::string_view /*fileName*/, const OpenMode /*mode*/) noexcept
	{
		assert_and_return_r(!_isOpen, false);
		_isOpen = true;
		return true;
	}

	constexpr bool close() noexcept
	{
		if (_isOpen)
		{
			_pos = 0;
			_isOpen = false;
		}
		return true;
	}

	inline bool read(void* targetBuffer, const size_t dataSize) noexcept
	{
		std::lock_guard lock(_mtx);

		assert_and_return_r(_pos + dataSize <= size(), false);
		::memcpy(targetBuffer, _data.data() + _pos, dataSize);
		_pos += dataSize;
		return true;
	}

	inline bool write(const void* sourceBuffer, const size_t dataSize) noexcept
	{
		std::lock_guard lock(_mtx);

		if (const auto newSize = _pos + dataSize; newSize > size())
		{
			if (newSize > _data.capacity())
				_data.reserve(newSize + newSize / 4 + 1);
			_data.resize(newSize);
		}

		::memcpy(_data.data() + _pos, sourceBuffer, dataSize);
		_pos += dataSize;
		return true;
	}

	// Sets the absolute position from the beginning of the file
	inline bool seek(const size_t position) & noexcept
	{
		std::lock_guard lock(_mtx);

		assert_and_return_r(position <= size(), false);
		_pos = position;
		return true;
	}

	inline bool seekToEnd() & noexcept
	{
		std::lock_guard lock(_mtx);

		return seek(size());
	}

	[[nodiscard]] inline uint64_t pos() const noexcept
	{
		std::lock_guard lock(_mtx);

		return _pos;
	}

	[[nodiscard]] inline uint64_t size() const noexcept
	{
		std::lock_guard lock(_mtx);

		return _data.size();
	}

	[[nodiscard]] inline bool atEnd() const noexcept
	{
		std::lock_guard lock(_mtx);

		return pos() == size();
	}

	inline constexpr bool flush() noexcept
	{
		return true;
	}

private:
	mutable std::recursive_mutex _mtx;
	std::vector<std::byte> _data;
	size_t _pos = 0;
	bool _isOpen = false;
};

}
