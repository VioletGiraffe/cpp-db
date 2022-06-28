#pragma once

#include "storage_io_interface.hpp"
#include "utility/static_data_buffer.hpp"

namespace io {

template <size_t MaxSize>
class StaticBufferAdapter
{
public:
	constexpr bool open(const std::string& /*fileName*/, const OpenMode /*mode*/) noexcept
	{
		return true;
	}

	constexpr bool close() noexcept
	{
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

	constexpr bool flush() noexcept
	{
		return true;
	}

	constexpr bool clear() noexcept
	{
		_buffer.reserve(0);
		_buffer.seeks(0);
		return true;
	}

	[[nodiscard]] constexpr const auto* data() const & noexcept
	{
		return _buffer.data();
	}

private:
	static_data_buffer<MaxSize> _buffer;
};

}