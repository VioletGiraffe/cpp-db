#pragma once

#include "container/std_container_helpers.hpp"
#include "container/multi_index.hpp"
#include "storage/storage_helpers.hpp"
#include "assert/advanced_assert.h"
#include "hash/fnv_1a.h"

#include <QFile>

#include <assert.h>
#include <limits>
#include <stdint.h>
#include <string>
#include <vector>

class DbFileGaps
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

	inline void registerGap(const uint64_t gapOffset, const uint64_t gapLength) noexcept {
		_gapLocations.emplace(Gap{ gapOffset, gapLength });
		++_insertionsSinceLastConsolidation;
	}

	inline uint64_t takeSuitableGap(const uint64_t requestedGapLength) noexcept {
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

	void consolidateGaps() noexcept
	{
		std::vector<Gap> mergedGaps;
		mergedGaps.reserve(1000);

		for (auto current = _gapLocations.begin(), next = ++_gapLocations.begin(), end = _gapLocations.end(); next != end;)
		{
			if (current->endOffset() >= next->location)
			{
				assert(current->endOffset() == next->location);
				// Merge current and next into a new gap item.
				// Can't do in-place because std::set's items are const.
				mergedGaps.emplace_back(Gap{ current->location, next->endOffset() - current->location });

				// So have to erase both
				const auto toBeErased_1 = current, toBeErased_2 = next;
				current = ++next;
				if (next != end)
					++next;

				_gapLocations.erase(toBeErased_1);
				_gapLocations.erase(toBeErased_2);
			}
			else
			{
				current = next;
				++next;
			}
		}

		// Put the merged items back in
		for (auto&& gap : mergedGaps)
			_gapLocations.emplace(std::move(gap));

		_insertionsSinceLastConsolidation = 0;
	}

	bool saveToFile(QString filePath) const
	{
		QFile file(filePath);
		if (!file.open(QFile::WriteOnly))
			return false;

		file.flush(); // TODO: does it do anything? I want the file to be resized to 0 immediately, guaranteed.
		if (!checkedWrite(file, (uint64_t)_gapLocations.size()))
			return false;

		FNV_1a_64_hasher hasher;
		for (const Gap& gap : _gapLocations)
		{
			hasher.updateHash(gap.length);
			hasher.updateHash(gap.location);
			if (!checkedWrite(file, gap.length) || !checkedWrite(file, gap.location))
				return false;
		}

		if (!checkedWrite(file, hasher.hash()))
			return false;

		return file.flush();
	}

	bool loadFromFile(QString filePath)
	{
		clear();

		QFile file(filePath);
		if (!file.open(QFile::ReadOnly))
			return false;

		uint64_t size = 0;
		if (!checkedRead(file, size))
			return false;

		FNV_1a_64_hasher hasher;
		for (size_t i = 0; i < size; ++i)
		{
			Gap gap;
			if (!checkedRead(file, gap.length) || !checkedRead(file, gap.location))
				return false;

			hasher.updateHash(gap.length);
			hasher.updateHash(gap.location);

			_gapLocations.emplace(std::move(gap));
		}

		uint64_t hash = 0;
		if (!checkedRead(file, hash))
			return false;

		assert_and_return_r(hash == hasher.hash(), false);
		return true;
	}

	void clear() noexcept
	{
		_gapLocations.clear();
		_insertionsSinceLastConsolidation = 0;
	}

private:
	MultiIndexSet<Gap, &Gap::location, &Gap::length> _gapLocations;
	uint64_t _insertionsSinceLastConsolidation = 0;
};
