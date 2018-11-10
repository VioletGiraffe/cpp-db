#pragma once

#include <limits>
#include <set>
#include <stdint.h>

class DbFileGaps
{
public:
	static constexpr auto noGap = std::numeric_limits<uint64_t>::max();

	inline void registerGap(const uint64_t gapOffset) {
		_gapLocations.insert(gapOffset);
	}

	inline uint64_t nextGap() const {
		return _gapLocations.empty() ? noGap : *_gapLocations.begin();
	}

	inline uint64_t takeNextGap() {
		const auto begin = _gapLocations.begin();
		if (begin == _gapLocations.end())
			return noGap;

		const auto gap = *begin;
		_gapLocations.erase(begin);

		return gap;
	}

private:
	std::set<uint64_t> _gapLocations;
};
