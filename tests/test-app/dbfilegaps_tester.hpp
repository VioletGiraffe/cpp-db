#pragma once

#include "dbfilegaps.hpp"
#include "random/randomnumbergenerator.h"
#include "container/algorithms.hpp"

#include <algorithm>
#include <vector>

struct DbFileGaps_Tester {
	DbFileGaps _gaps;

private:
	RandomNumberGenerator<uint64_t> _rngLength;
	RandomNumberGenerator<uint64_t> _rngLocation;
};
