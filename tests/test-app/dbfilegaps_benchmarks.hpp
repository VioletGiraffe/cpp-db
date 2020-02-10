#pragma once

#include "3rdparty/catch2/catch.hpp"
#include "dbfilegaps_tester.hpp"

TEST_CASE("Filling empty DbFileGaps benchmark", "[dbfilegaps]") {
	try {
		DbFileGaps_Tester tester(100000, 1000);

		BENCHMARK("Filling an empty index, 100 items") {
			tester.generateGaps(100);
		};
		REQUIRE(tester.verifyGaps());

		tester.clear();
		BENCHMARK("Filling an empty index, 200 items") {
			tester.generateGaps(200);
		};
		REQUIRE(tester.verifyGaps());

		tester.clear();
		BENCHMARK("Filling an empty index, 400 items") {
			tester.generateGaps(400);
		};
		REQUIRE(tester.verifyGaps());

	} catch(...) {
		FAIL();
	}
}

TEST_CASE("Adding more gaps to non-empty DbFileGaps benchmark", "[dbfilegaps]") {
	try {
		DbFileGaps_Tester tester(100000, 1000);

		tester.generateGaps(1000);

		BENCHMARK("Adding 100 to 1000 existing, 1000 items") {
			tester.generateGaps(100);
		};
		REQUIRE(tester.verifyGaps());

		BENCHMARK("Adding 100 more") {
			tester.generateGaps(100);
		};
		REQUIRE(tester.verifyGaps());

		BENCHMARK("And another 100") {
			tester.generateGaps(100);
		};
		REQUIRE(tester.verifyGaps());

	}
	catch (...) {
		FAIL();
	}
}