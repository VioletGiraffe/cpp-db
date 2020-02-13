#include "dbfilegaps_tester.hpp"

#include <algorithm>
#include <iterator>

std::vector<DbFileGaps::Gap> DbFileGaps_Tester::enumerateGaps() const noexcept
{
	std::vector<DbFileGaps::Gap> gaps;
	gaps.reserve(_gaps.size());
	std::copy(cbegin_to_end(_gaps._gapLocations), std::back_inserter(gaps));
	return gaps;
}
