#pragma once

#include "dbfilegaps.hpp"
#include "random/randomnumbergenerator.h"

#include <algorithm>
#include <vector>

struct DbFileGaps_Tester {
	DbFileGaps _gaps;
	std::vector<DbFileGaps::Gap> _referenceGaps;

	inline DbFileGaps_Tester(uint64_t workingSpace = 4000000000, uint64_t maxGapLength = 100000) noexcept :
		_rngLength{0, 1, maxGapLength},
		_rngLocation{0, 0, workingSpace}
	{}

	inline bool generateGaps(const uint64_t count) {
		_referenceGaps.clear();
		_gaps.clear();
		_referenceGaps.reserve(count);
		for (uint64_t i = 0; i < count;)
		{
			DbFileGaps::Gap newGap{_rngLocation.rand(), _rngLength.rand()};
			if (std::find_if(cbegin_to_end(_referenceGaps), [&newGap](const DbFileGaps::Gap& gap){return gap.location == newGap.location;}) != _referenceGaps.cend())
				continue;

			_gaps.registerGap(newGap.location, newGap.length);
			_referenceGaps.emplace_back(std::move(newGap));
			++i;
		}

		return true;
	}

	inline bool verifyGaps() noexcept
	{
		if (_gaps._gapLocations.size() != _referenceGaps.size())
			return false;

		std::vector<DbFileGaps::Gap> actualGaps;
		actualGaps.reserve(_referenceGaps.size());
		for (const DbFileGaps::Gap& gap: _gaps._gapLocations)
		{
			actualGaps.emplace_back(gap);
		}

		constexpr auto gapLessComp = [](const DbFileGaps::Gap& l, const DbFileGaps::Gap& r) {
			return l.location < r.location;
		};

		std::sort(begin_to_end(_referenceGaps), gapLessComp);
		std::sort(begin_to_end(actualGaps), gapLessComp);
		return std::equal(cbegin_to_end(_referenceGaps), cbegin_to_end(actualGaps), [](const DbFileGaps::Gap& l, const DbFileGaps::Gap& r) {return r.length == l.length && r.location == l.location; });
	}

private:
	RandomNumberGenerator<uint64_t> _rngLength;
	RandomNumberGenerator<uint64_t> _rngLocation;
};
