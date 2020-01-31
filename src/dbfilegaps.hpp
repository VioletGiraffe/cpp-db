#pragma once

#include "container/std_container_helpers.hpp"
#include "container/multi_index.hpp"

#include <assert.h>
#include <limits>
#include <stdint.h>
#include <string>
#include <vector>

class DbFileGaps
{
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

	inline uint64_t takeSuitableGap(const uint64_t gapLength) noexcept {
		assert(gapLength > 0);

		const auto [begin, end] = _gapLocations.findSecondaryInRange(gapLength, std::numeric_limits<decltype(Gap::length)>::max());
		/* TODO: experiment with smart heuristics, e. g.:
			- taking the shortest suitable gap;
			- taking the longest suitable gap;
			- taking the gap with the smallest offset (may or not accidentally be what *begin already refers to).
			- taking the gap adjacent with another gap for consolidating free space
		*/


		if (begin != end)
			return (*begin)->location;
		else if (_insertionsSinceLastConsolidation < 1000)
			return noGap;

		// If no gap is found, consolidate gaps and try again
		consolidateGaps();
		const auto [newBegin, newEnd] = _gapLocations.findSecondaryInRange(gapLength, std::numeric_limits<decltype(Gap::length)>::max());
		return newBegin != newEnd ? (*newBegin)->location : noGap;
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

	bool saveToFile(std::string filePath) const
	{

	}

private:
	MultiIndexSet<Gap, &Gap::location, &Gap::length> _gapLocations;
	uint64_t _insertionsSinceLastConsolidation = 0;
};
