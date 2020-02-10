#pragma once

#include "3rdparty/catch2/catch.hpp"
#include "dbfilegaps_tester.hpp"

TEST_CASE("Small test, filling DbFileGaps", "[dbfilegaps]") {
	try {
		DbFileGaps gaps;

		constexpr auto gapLength = 5ull;
		constexpr uint64_t n = 500;
		for (uint64_t offset = 0; offset < gapLength * n; offset += gapLength)
		{
			gaps.registerGap(offset, gapLength);
		}

		CHECK(gaps.size() == n);
		CHECK(gaps.takeSuitableGap(5) == 0);
		CHECK(gaps.takeSuitableGap(5) == 5);
		CHECK(gaps.takeSuitableGap(5) == 10);

		CHECK(gaps.takeSuitableGap(4 * gapLength) == DbFileGaps::noGap);
		CHECK(gaps.size() == n - 3);
		gaps.consolidateGaps();
		CHECK(gaps.size() == 1);
		CHECK(gaps.takeSuitableGap(4 * gapLength) == 15);
		CHECK(gaps.size() == 1);
		CHECK(gaps.takeSuitableGap(150) == 25);
		CHECK(gaps.size() == 1);

		CHECK(gaps.takeSuitableGap(n - 174) == DbFileGaps::noGap);
		gaps.registerGap(10000, 10000);
		CHECK(gaps.size() == 2);
		CHECK(gaps.takeSuitableGap(n - 174) == 10000);
		CHECK(gaps.size() == 1);

	} catch(...) {
		FAIL();
	}
}

TEST_CASE("Larger test, filling DbFileGaps and then removing gaps", "[dbfilegaps]") {
	try {
	
	}
	catch (...) {
		FAIL();
	}
}