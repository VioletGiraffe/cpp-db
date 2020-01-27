#pragma once

#include "3rdparty/catch2/catch.hpp"
#include "dbfilegaps.hpp"

TEST_CASE("Construction", "[dbfilegaps]") {
	try {
		DbFileGaps gaps;

		REQUIRE_NOTHROW(gaps.takeSuitableGap(1) == DbFileGaps::noGap);
	} catch(...) {
		FAIL();
	}
}

TEST_CASE("Operation - simple", "[dbfilegaps]") {
	try {
		DbFileGaps gaps;

		gaps.registerGap(10, 16);

		REQUIRE_NOTHROW(gaps.takeSuitableGap(17) == DbFileGaps::noGap);
		REQUIRE_NOTHROW(gaps.takeSuitableGap(16) == 10);
		REQUIRE_NOTHROW(gaps.takeSuitableGap(16) == DbFileGaps::noGap);
		REQUIRE_NOTHROW(gaps.takeSuitableGap(1) == DbFileGaps::noGap);
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

		REQUIRE_NOTHROW(gaps.takeSuitableGap(26) == DbFileGaps::noGap);
		gaps.consolidateGaps();

		REQUIRE_NOTHROW(gaps.takeSuitableGap(26) == 10);
		REQUIRE_NOTHROW(gaps.takeSuitableGap(2) == 1);
		REQUIRE_NOTHROW(gaps.takeSuitableGap(1) == DbFileGaps::noGap);
	} catch(...) {
		FAIL();
	}
}
