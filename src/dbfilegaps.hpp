#pragma once

#include <limits>
#include <set>
#include <stdint.h>

class DbFileGaps
{
	struct Gap {
		uint64_t location;
		uint64_t length;

		inline constexpr bool operator< (const Gap& other) const {
			return location < other.location;
		}
	};

public:
	static constexpr auto noGap = std::numeric_limits<uint64_t>::max();

	inline void registerGap(const uint64_t gapOffset, const uint64_t gapLength) {
		_gapLocations.emplace(gapOffset, gapLength);
		// TODO: consolidate gaps
	}

	inline uint64_t takeSuitableGap(const uint64_t dataLength) {
		
	}

private:
	std::set <Gap> _gapLocations;
};
