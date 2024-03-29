#pragma once

#include "fileallocationmanager.hpp"
#include "random/randomnumbergenerator.h"

#include <vector>

struct DbFileGaps_Tester {
	FileAllocationManager _gaps;

	[[nodiscard]] std::vector<std::pair<uint64_t, uint64_t>> enumerateGaps() const noexcept;

private:
	RandomNumberGenerator<uint64_t> _rngLength;
	RandomNumberGenerator<uint64_t> _rngLocation;
};
