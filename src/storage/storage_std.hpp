#pragma once

#include "storage_io_interface.hpp"

#include <algorithm>
#include <stdio.h>

namespace io {

class FopenAdapter
{
public:
	[[nodiscard]] bool open(std::string_view fileName, const OpenMode mode, bool truncate = false) noexcept
	{
		assert_and_return_r(!_handle, false);

		_filePath = fileName;
		toNativePath(_filePath);

		switch (mode) {
		case OpenMode::Read:
			_handle = ::fopen(_filePath.c_str(), "rb");
			break;
		case OpenMode::Write:
			_handle = ::fopen(_filePath.c_str(), "wb");
			break;
		case OpenMode::ReadWrite:
			_handle = ::fopen(_filePath.c_str(), truncate ? "w+b" : "a+b");
			break;
		default:
			assert_and_return_unconditional_r("Unknown open mode " + std::to_string(static_cast<int>(mode)), false);
		}

		_mode = mode;
		return _handle != nullptr;
	}

	[[nodiscard]] bool close() noexcept
	{
		assert_and_return_r(_handle, false);
		if (::fclose(_handle) != 0)
			return false;

		_filePath.clear();
		_handle = 0;
		return true;
	}

	[[nodiscard]] bool read(void* targetBuffer, const size_t dataSize) noexcept
	{
		return ::fread(targetBuffer, 1, dataSize, _handle) == dataSize;
	}

	[[nodiscard]] bool write(const void* const targetBuffer, const size_t dataSize) noexcept
	{
		return ::fwrite(targetBuffer, 1, dataSize, _handle) == dataSize;
	}

	// Sets the absolute position from the beginning of the file
	[[nodiscard]] bool seek(const uint64_t position) noexcept
	{
		return ::fseek(_handle, (long)position, SEEK_SET) == 0;
	}

	[[nodiscard]] bool seekToEnd() noexcept
	{
		return ::fseek(_handle, 0, SEEK_END) == 0;
	}

	[[nodiscard]] uint64_t pos() const noexcept
	{
		const auto p = ::ftell(_handle);
		// TODO: error checking in release build!
		assert_debug_only(p >= 0);
		return static_cast<uint64_t>(p);
	}

	[[nodiscard]] uint64_t size() noexcept
	{
		const auto currentPos = pos();
		assert_and_return_r(seekToEnd(), 0);
		const auto s = pos();
		assert_r(seek(currentPos));
		return s;
	}

	[[nodiscard]] bool atEnd() noexcept
	{
		if (::feof(_handle) != 0)
			return true;
		else if (_mode != OpenMode::Write)
		{
			// Need to try to read at least one symbol
			char dummy;
			const auto isAtEnd = read(&dummy, 1);
			if (isAtEnd)
				return true;
			else
				assert_r(::fseek(_handle, -1, SEEK_CUR) == 0);
		}

		return false;
	}

	bool flush() noexcept
	{
		return ::fflush(_handle) == 0;
	}

	[[nodiscard]] bool clear() noexcept
	{
		assert_and_return_r(close(), false);
		return open(_filePath, _mode, true /* truncate */);
	}

private:
	static void toNativePath(std::string& path) noexcept
	{
#ifdef _WIN32
		for (char& ch: path)
		{
			if (ch == '/')
				ch = '\\';
		}
#else
		assert_r(path.find('\\') == std::string::npos);
#endif
	}

private:
	std::string _filePath;
	FILE* _handle = nullptr;
	OpenMode _mode;
};

} // namespace io
