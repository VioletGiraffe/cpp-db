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
