#pragma once

#include "3rdparty/catch2/catch.hpp"
#include "dbfilegaps.hpp"

TEST_CASE("Construction", "[dbfilegaps]") {
	try {
		DbFileGaps gaps;

		REQUIRE(gaps.takeSuitableGap(1) == DbFileGaps::noGap);
	} catch(...) {
		FAIL();
	}
}

TEST_CASE("Operation - simple", "[dbfilegaps]") {
	try {
		DbFileGaps gaps;

		gaps.registerGap(10, 16);

		REQUIRE(gaps.takeSuitableGap(17) == DbFileGaps::noGap);
		REQUIRE(gaps.takeSuitableGap(16) == 10);
		REQUIRE(gaps.takeSuitableGap(16) == DbFileGaps::noGap);
		REQUIRE(gaps.takeSuitableGap(1) == DbFileGaps::noGap);
	} catch(...) {
		FAIL();
	}
}

TEST_CASE("Gap consolidation", "[dbfilegaps]") {
	try {
		DbFileGaps gaps;

		gaps.registerGap(1, 1);
		gaps.registerGap(2, 1);
		gaps.registerGap(10, 16);
		gaps.registerGap(26, 10);

		REQUIRE(gaps.takeSuitableGap(26) == DbFileGaps::noGap);
		gaps.consolidateGaps();

		REQUIRE(gaps.takeSuitableGap(26) == 10);
		REQUIRE(gaps.takeSuitableGap(2) == 1);
		REQUIRE(gaps.takeSuitableGap(1) == DbFileGaps::noGap);
	} catch(...) {
		FAIL();
	}
}
