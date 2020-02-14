#pragma once

#include "3rdparty/catch2/catch.hpp"
#include "dbfilegaps_tester.hpp"

TEST_CASE("Filling empty DbFileGaps benchmark", "[dbfilegaps]") {
	try {
		DbFileGaps gaps;

		BENCHMARK("Filling empty DbFileGaps, 1000 items") {
			constexpr uint64_t maxLength = 1000;
			for (uint64_t offset = 0, length = maxLength; length > 0; offset += length--)
			{
				gaps.registerGap(offset, length);
			}

			return gaps.size();
		};

		BENCHMARK("Filling empty DbFileGaps, 10000 items") {
			constexpr uint64_t maxLength = 10000;
			for (uint64_t offset = 0, length = maxLength; length > 0; offset += length--)
			{
				gaps.registerGap(offset, length);
			}

			return gaps.size();
		};

		BENCHMARK("Filling empty DbFileGaps, 20000 items") {
			constexpr uint64_t maxLength = 20000;
			for (uint64_t offset = 0, length = maxLength; length > 0; offset += length--)
			{
				gaps.registerGap(offset, length);
			}

			return gaps.size();
		};
	}
	catch (...) {
		FAIL();
	}
}