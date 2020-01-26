#pragma once

#include "container/std_container_helpers.hpp"
#include "container/multi_index.hpp"
#include "assert/advanced_assert.h"

#include <limits>
#include <stdint.h>

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

	inline void registerGap(const uint64_t gapOffset, const uint64_t gapLength) {
		_gapLocations.emplace(Gap{ gapOffset, gapLength });
	}

	inline uint64_t takeSuitableGap(const uint64_t gapLength) {
		const auto [begin, end] = _gapLocations.findSecondaryInRange(gapLength, std::numeric_limits<decltype(Gap::length)>::max());
		/* TODO: experiment with smart heuristics, e. g.:
			- taking the shortest suitable gap;
			- taking the longest suitable gap;
			- taking the gap with the smallest offset (may or not accidentally be what *begin already refers to).
			- taking the gap adjacent with another gap for consolidating free space
		*/


		if (begin != end)
			return (*begin)->location;

		// If no gap is found, consolidate gaps and try again
		consolidateGaps();
		const auto [newBegin, newEnd] = _gapLocations.findSecondaryInRange(gapLength, std::numeric_limits<decltype(Gap::length)>::max());
		return newBegin != newEnd ? (*newBegin)->location : noGap;
	}

private:
	void consolidateGaps()
	{
		for (auto current = _gapLocations.begin(), next = ++_gapLocations.begin(), end = _gapLocations.end(); next != end; (current = next), ++next)
		{
			if (current->endOffset() >= next->location)
			{
				assert_debug_only(current->endOffset() == next->location);
				// Merge current into next
				// Can't do in-place because std::set's items are const
				Gap newItem{ current->location, next->endOffset() - current->location };
				
			}
		}
	}

private:
	MultiIndexSet<Gap, &Gap::location, &Gap::length> _gapLocations;
};
