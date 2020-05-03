#include "3rdparty/catch2/catch.hpp"
#include "fileallocationmanager.hpp"

TEST_CASE("Construction", "[dbfilegaps]") {
	try {
		FileAllocationManager gaps;

		REQUIRE(gaps.takeSuitableGap(1) == FileAllocationManager::NoGap);
	} catch(...) {
		FAIL();
	}
}

TEST_CASE("Operation - simple", "[dbfilegaps]") {
	try {
		FileAllocationManager gaps;

		gaps.registerGap(10, 16);

		REQUIRE(gaps.takeSuitableGap(17) == FileAllocationManager::NoGap);
		REQUIRE(gaps.takeSuitableGap(16) == 10);
		REQUIRE(gaps.takeSuitableGap(16) == FileAllocationManager::NoGap);
		REQUIRE(gaps.takeSuitableGap(1) == FileAllocationManager::NoGap);
	} catch(...) {
		FAIL();
	}
}

TEST_CASE("Gap consolidation", "[dbfilegaps]") {
	try {
		FileAllocationManager gaps;

		gaps.registerGap(1, 1);
		gaps.registerGap(2, 1);
		gaps.registerGap(10, 16);
		gaps.registerGap(26, 10);

		REQUIRE(gaps.takeSuitableGap(26) == FileAllocationManager::NoGap);
		gaps.consolidateGaps();

		REQUIRE(gaps.takeSuitableGap(26) == 10);
		REQUIRE(gaps.takeSuitableGap(2) == 1);
		REQUIRE(gaps.takeSuitableGap(1) == FileAllocationManager::NoGap);
	} catch(...) {
		FAIL();
	}
}
