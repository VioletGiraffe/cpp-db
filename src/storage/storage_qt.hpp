#pragma once

#include "storage_io_interface.hpp"

#include <QBuffer>
#include <QFile>

#include <string>

namespace io {

class QFileAdapter
{
public:
	bool open(std::string fileName, const OpenMode mode)
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

	bool read(void* targetBuffer, const size_t dataSize)
	{
		return _file.read(reinterpret_cast<char*>(targetBuffer), dataSize) == static_cast<qint64>(dataSize);
	}

	bool write(const void* const targetBuffer, const size_t dataSize)
	{
		return _file.write(reinterpret_cast<const char*>(targetBuffer), dataSize) == static_cast<qint64>(dataSize);
	}

	// Sets the absolute position from the beginning of the file
	bool seek(const uint64_t position)
	{
		return _file.seek(static_cast<qint64>(position));
	}

	uint64_t pos() const noexcept
	{
		const auto position = _file.pos();
		assert_debug_only(position >= 0);
		return static_cast<uint64_t>(position);
	}

	uint64_t size() const noexcept
	{
		return static_cast<uint64_t>(_file.size());
	}

	bool flush()
	{
		return _file.flush();
	}

private:
	QFile _file;
};


class QMemoryDeviceAdapter
{
public:
	bool open(std::string /*fileName*/, const OpenMode mode)
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

	bool read(void* targetBuffer, const size_t dataSize)
	{
		return _ioDevice.read(reinterpret_cast<char*>(targetBuffer), dataSize) == static_cast<qint64>(dataSize);
	}

	bool write(const void* const targetBuffer, const size_t dataSize)
	{
		return _ioDevice.write(reinterpret_cast<const char*>(targetBuffer), dataSize) == static_cast<qint64>(dataSize);
	}

	// Sets the absolute position from the beginning of the file
	bool seek(const uint64_t position)
	{
		return _ioDevice.seek(static_cast<qint64>(position));
	}

	uint64_t pos() const noexcept
	{
		const auto position = _ioDevice.pos();
		assert_debug_only(position >= 0);
		return static_cast<uint64_t>(position);
	}

	uint64_t size() const noexcept
	{
		return static_cast<uint64_t>(_ioDevice.size());
	}

	bool flush()
	{
		return true;
	}

	const QByteArray& data() const&
	{
		return _dataBuffer;
	}

private:
	QByteArray _dataBuffer;
	QBuffer _ioDevice;
};

}