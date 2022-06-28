#pragma once

#include "storage_io_interface.hpp"

#include <QBuffer>
#include <QFile>

namespace io {

class QFileAdapter
{
public:
	[[nodiscard]] bool open(const std::string& fileName, const OpenMode mode) noexcept
	{
		QIODevice::OpenModeFlag qtOpenMode;
		switch (mode) {
		case OpenMode::Read:
			qtOpenMode = QIODevice::ReadOnly;
			break;
		case OpenMode::Write:
			qtOpenMode = QIODevice::WriteOnly;
			break;
		case OpenMode::ReadWrite:
			qtOpenMode = QIODevice::ReadWrite;
			break;
		default:
			assert_and_return_unconditional_r("Unknown open mode " + std::to_string(static_cast<int>(mode)), false);
		}

		_file.setFileName(QString::fromStdString(fileName));
		return _file.open(qtOpenMode);
	}

	[[nodiscard]] bool read(void* targetBuffer, const size_t dataSize) noexcept
	{
		return _file.read(reinterpret_cast<char*>(targetBuffer), dataSize) == static_cast<qint64>(dataSize);
	}

	[[nodiscard]] bool write(const void* const targetBuffer, const size_t dataSize) noexcept
	{
		return _file.write(reinterpret_cast<const char*>(targetBuffer), dataSize) == static_cast<qint64>(dataSize);
	}

	// Sets the absolute position from the beginning of the file
	[[nodiscard]] bool seek(const uint64_t position) noexcept
	{
		return _file.seek(static_cast<qint64>(position));
	}

	[[nodiscard]] bool seekToEnd() noexcept
	{
		return _file.seek(_file.size());
	}

	[[nodiscard]] uint64_t pos() const noexcept
	{
		const auto position = _file.pos();
		assert_debug_only(position >= 0);
		return static_cast<uint64_t>(position);
	}

	[[nodiscard]] uint64_t size() const noexcept
	{
		return static_cast<uint64_t>(_file.size());
	}

	[[nodiscard]] bool atEnd() const noexcept
	{
		return _file.atEnd();
	}

	bool flush() noexcept
	{
		return _file.flush();
	}

	[[nodiscard]] bool clear() noexcept
	{
		if (!_file.resize(0))
			return false;

		flush();
		return true;
	}

private:
	QFile _file;
};


class QMemoryDeviceAdapter
{
public:
	[[nodiscard]] bool open(const std::string& /*fileName*/, const OpenMode mode) noexcept
	{
		QIODevice::OpenModeFlag qtOpenMode;
		switch (mode) {
		case OpenMode::Read:
			qtOpenMode = QIODevice::ReadOnly;
			break;
		case OpenMode::Write:
			qtOpenMode = QIODevice::WriteOnly;
			break;
		case OpenMode::ReadWrite:
			qtOpenMode = QIODevice::ReadWrite;
			break;
		default:
			assert_and_return_unconditional_r("Unknown open mode " + std::to_string(static_cast<int>(mode)), false);
		}

		return _ioDevice.open(qtOpenMode);
	}

	[[nodiscard]] bool read(void* targetBuffer, const size_t dataSize) noexcept
	{
		return _ioDevice.read(reinterpret_cast<char*>(targetBuffer), dataSize) == static_cast<qint64>(dataSize);
	}

	[[nodiscard]] bool write(const void* const targetBuffer, const size_t dataSize) noexcept
	{
		return _ioDevice.write(reinterpret_cast<const char*>(targetBuffer), dataSize) == static_cast<qint64>(dataSize);
	}

	// Sets the absolute position from the beginning of the file
	[[nodiscard]] bool seek(const uint64_t position) noexcept
	{
		return _ioDevice.seek(static_cast<qint64>(position));
	}

	[[nodiscard]] bool seekToEnd() noexcept
	{
		return _ioDevice.seek(_ioDevice.size());
	}

	[[nodiscard]] uint64_t pos() const noexcept
	{
		const auto position = _ioDevice.pos();
		assert_debug_only(position >= 0);
		return static_cast<uint64_t>(position);
	}

	[[nodiscard]] uint64_t size() const noexcept
	{
		return static_cast<uint64_t>(_ioDevice.size());
	}

	[[nodiscard]] bool atEnd() const noexcept
	{
		return _ioDevice.atEnd();
	}

	bool flush() noexcept
	{
		return true;
	}

	[[nodiscard]] bool clear() noexcept
	{
		_dataBuffer.resize(0);
		return true;
	}

	[[nodiscard]] const QByteArray& data() const& noexcept
	{
		return _dataBuffer;
	}

private:
	QByteArray _dataBuffer;
	QBuffer _ioDevice;
};

}