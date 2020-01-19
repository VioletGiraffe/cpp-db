#pragma once

#include "container/std_container_helpers.hpp"

#include <limits>
#include <set>
#include <stdint.h>

class DbFileGaps
{
	struct Gap {
		uint64_t location;
		uint64_t length;
	};

public:
	static constexpr auto noGap = std::numeric_limits<uint64_t>::max();

	inline void registerGap(const uint64_t gapOffset, const uint64_t gapLength) {
		_gapLocations.emplace(Gap{ gapOffset, gapLength });
		// TODO: consolidate gaps
	}

	inline uint64_t takeSuitableGap(const uint64_t dataLength) {
		const auto suitableGapIt = std::find_if(begin_to_end(_gapLocations), [&dataLength](const Gap& gap){return gap.length >= dataLength;});
	}

private:
	std::set <Gap> _gapLocations;
};
