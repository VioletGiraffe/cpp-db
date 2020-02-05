#pragma once

#include "3rdparty/catch2/catch.hpp"
#include "dbfilegaps_tester.hpp"

TEST_CASE("Small test, filling DbFileGaps", "[dbfilegaps]") {
	try {
		DbFileGaps_Tester tester(100000, 1000);
		tester.generateGaps(10000);

		REQUIRE_NOTHROW(tester.verifyGaps());
	} catch(...) {
		FAIL();
	}
}

TEST_CASE("Larger test, filling DbFileGaps and then removing gaps", "[dbfilegaps]") {
	try {
		DbFileGaps_Tester tester(100000000, 100000);
		tester.generateGaps(100000);

		REQUIRE_NOTHROW(tester.verifyGaps());

		RandomNumberGenerator<uint64_t> rng(0, 1, 100000000);
		for (size_t i = 1; i < tester._referenceGaps.size() / 2; ++i)
			REQUIRE(tester.takeGap(i));

		REQUIRE(tester.verifyGaps());
	}
	catch (...) {
		FAIL();
	}
}