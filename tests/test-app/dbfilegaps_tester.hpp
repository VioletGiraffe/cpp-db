#pragma once

#include "fileallocationmanager.hpp"
#include "random/randomnumbergenerator.h"
#include "container/algorithms.hpp"

#include <algorithm>
#include <vector>

struct DbFileGaps_Tester {
	FileAllocationManager _gaps;

	std::vector<std::pair<uint64_t, uint64_t>> enumerateGaps() const noexcept;

private:
	RandomNumberGenerator<uint64_t> _rngLength;
	RandomNumberGenerator<uint64_t> _rngLocation;
};
