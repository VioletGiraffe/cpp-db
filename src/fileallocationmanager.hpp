#pragma once

#include "container/std_container_helpers.hpp"
#include "container/multi_index.hpp"
#include "storage/storage_io_interface.hpp"
#include "assert/advanced_assert.h"
#include "hash/sha3_hasher.hpp"

#include <assert.h>
#include <limits>
#include <stdint.h>
#include <string>
#include <vector>

class FileAllocationManager
{
	friend struct DbFileGaps_Tester;
	struct Gap {
		uint64_t location;
		uint64_t length;

		constexpr uint64_t endOffset() const {
			return location + length;
		}
	};

public:
	static constexpr auto noGap = std::numeric_limits<uint64_t>::max();

	inline void registerGap(const uint64_t gapOffset, const uint64_t gapLength) noexcept;
	inline uint64_t takeSuitableGap(const uint64_t requestedGapLength) noexcept;

	inline void consolidateGaps() noexcept;

	template <typename StorageAdapter>
	bool saveToFile(std::string filePath) const noexcept;
	template <typename StorageAdapter>
	bool loadFromFile(std::string filePath) noexcept;

	inline size_t size() const noexcept;
	inline void clear() noexcept;

private:
	MultiIndexSet<Gap, &Gap::location, &Gap::length> _gapLocations;
	uint64_t _insertionsSinceLastConsolidation = 0;
};

inline void FileAllocationManager::registerGap(const uint64_t gapOffset, const uint64_t gapLength) noexcept {
	_gapLocations.emplace(Gap{ gapOffset, gapLength });
	++_insertionsSinceLastConsolidation;
}

inline uint64_t FileAllocationManager::takeSuitableGap(const uint64_t requestedGapLength) noexcept {
	assert(requestedGapLength > 0);

	const auto [begin, end] = _gapLocations.findSecondaryInRange(requestedGapLength, std::numeric_limits<decltype(Gap::length)>::max());
	/* TODO: experiment with smart heuristics, e. g.:
		- taking the shortest suitable gap;
		- taking the longest suitable gap;
		- taking the gap with the smallest offset (may or not accidentally be what *begin already refers to).
		- taking the gap adjacent with another gap for consolidating free space
	*/
	if (begin == end)
	{
		if (_insertionsSinceLastConsolidation < 1000)
			return noGap;

		consolidateGaps(); // If no gap is found, consolidate gaps and try again
		return takeSuitableGap(requestedGapLength); // Recursing once to check if consolidation freed up enough space.
	}

	Gap remainingGap = *(*begin);
	const auto offset = remainingGap.location;
	assert_r(_gapLocations.erase(remainingGap.location) == 1);
	if (remainingGap.length != requestedGapLength)
	{
		assert_debug_only(remainingGap.length > requestedGapLength);
		remainingGap.location += requestedGapLength;
		remainingGap.length -= requestedGapLength;
		_gapLocations.emplace(std::move(remainingGap));
	}

	return offset;
}

inline void FileAllocationManager::consolidateGaps() noexcept
{
	const auto locationsCount = _gapLocations.size();
	if (locationsCount < 2)
		return;

	std::vector<Gap> mergedGaps;
	mergedGaps.reserve(locationsCount / 8);

	std::vector<typename decltype(_gapLocations)::iterator> gapsToErase;
	gapsToErase.reserve(locationsCount / 8);

	auto current = _gapLocations.begin(), next = ++_gapLocations.begin();
	const auto end = _gapLocations.end();
	Gap accumulatedGap{ current->location, current->length };
	bool accumulationOccurred = false;
	for (; next != end;)
	{
		if (current->endOffset() >= next->location)
		{
			assert(current->endOffset() == next->location);
			accumulatedGap.length += next->length;
			accumulationOccurred = true;

			gapsToErase.emplace_back(current);
		}
		else
		{
			if (accumulationOccurred) // The current item was consumed on the previous iteration
				gapsToErase.emplace_back(current);

			accumulationOccurred = false;
			// Process what's been accumulated up to now
			mergedGaps.emplace_back(std::move(accumulatedGap));
			// And reset the accumulator
			accumulatedGap = Gap{ next->location, next->length };
		}

		current = next;
		++next;
	}

	if (accumulationOccurred)
	{
		assert(current != end);
		mergedGaps.emplace_back(std::move(accumulatedGap));
		gapsToErase.emplace_back(current);
	}

	// Erase the items subject to merging
	for (auto&& it : gapsToErase)
		_gapLocations.erase(it);

	// Put the merged items back in
	for (auto&& gap : mergedGaps)
		_gapLocations.emplace(std::move(gap));

	_insertionsSinceLastConsolidation = 0;
}

template <typename StorageAdapter>
bool FileAllocationManager::saveToFile(std::string filePath) const noexcept
{
	StorageIO<StorageAdapter> storage;
	if (!storage.open(std::move(filePath), io::OpenMode::Write))
		return false;

	storage.flush(); // TODO: does it do anything? I want the file to be resized to 0 immediately, guaranteed.
	if (!storage.write(static_cast<int64_t>(_gapLocations.size())))
		return false;

	Sha3_Hasher<256> hasher;
	for (const Gap& gap : _gapLocations)
	{
		hasher.update(gap.length);
		hasher.update(gap.location);
		if (!storage.write(gap.length) || !storage.write(gap.location))
			return false;
	}

	if (!storage.write(hasher.get64BitHash()))
		return false;

	return storage.flush(); // Somewhat unnecessary as the file's destructor will call close() which should flush, but I want to see that 'true' value for success.
}

template <typename StorageAdapter>
bool FileAllocationManager::loadFromFile(std::string filePath) noexcept
{
	clear();

	StorageIO<StorageAdapter> storage;
	if (!storage.open(std::move(filePath), io::OpenMode::Read))
		return false;

	uint64_t size = 0;
	if (!storage.read(size))
		return false;

	Sha3_Hasher<256> hasher;
	for (size_t i = 0; i < size; ++i)
	{
		Gap gap;
		if (!storage.read(gap.length) || !storage.read(gap.location))
			return false;

		hasher.update(gap.length);
		hasher.update(gap.location);

		_gapLocations.emplace(std::move(gap));
	}

	uint64_t hash = 0;
	if (!storage.read(hash))
		return false;

	assert_and_return_r(hash == hasher.get64BitHash(), false);
	return true;
}

inline size_t FileAllocationManager::size() const noexcept
{
	return _gapLocations.size();
}

inline void FileAllocationManager::clear() noexcept
{
	_gapLocations.clear();
	_insertionsSinceLastConsolidation = 0;
}
