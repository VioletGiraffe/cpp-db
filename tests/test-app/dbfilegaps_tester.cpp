#include "dbfilegaps_tester.hpp"

#include <algorithm>
#include <iterator>

std::vector<std::pair<uint64_t, uint64_t>> DbFileGaps_Tester::enumerateGaps() const noexcept
{
	std::vector<std::pair<uint64_t, uint64_t>> gaps;
	gaps.reserve(_gaps.size());
	for (const DbFileGaps::Gap& gap : _gaps._gapLocations)
		gaps.emplace_back(gap.location, gap.length);

	return gaps;
}
