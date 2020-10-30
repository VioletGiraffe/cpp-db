#pragma once

#include <hash/fnv_1a.h>

namespace io {

template <class IOAdapter>
class HashingAdapter final : public IOAdapter {
public:
	[[nodiscard]] bool read(void* targetBuffer, const size_t dataSize) noexcept
	{
		if (!IOAdapter::read(targetBuffer, dataSize))
			return false;

		_hasher.updateHash(targetBuffer, dataSize);
		return true;
	}

	[[nodiscard]] bool write(const void* const targetBuffer, const size_t dataSize) noexcept
	{
		_hasher.updateHash(targetBuffer, dataSize);
		return IOAdapter::write(targetBuffer, dataSize);
	}

	[[nodiscard]] auto hash() const noexcept
	{
		return _hasher.hash();
	}

	void resetHash() noexcept
	{
		_hasher.reset();
	}

private:
	FNV_1a_32_hasher _hasher;
};

}
