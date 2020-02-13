#pragma once

#include "3rdparty/catch2/catch.hpp"
#include "dbfilegaps_tester.hpp"

TEST_CASE("Simple interface test (black box)", "[dbfilegaps]") {
	try {
		DbFileGaps gaps;

		constexpr auto gapLength = 5ull;
		constexpr uint64_t n = 500;
		for (uint64_t offset = 0; offset < gapLength * n; offset += gapLength)
		{
			gaps.registerGap(offset, gapLength);
		}

		uint64_t freeRegionStart = 0;
		CHECK(gaps.size() == n);
		CHECK(gaps.takeSuitableGap(gapLength) == freeRegionStart);
		freeRegionStart += gapLength;
		CHECK(gaps.takeSuitableGap(gapLength) == 5);
		freeRegionStart += gapLength;
		CHECK(gaps.takeSuitableGap(gapLength) == 10);
		freeRegionStart += gapLength;

		CHECK(gaps.takeSuitableGap(gapLength + 1) == DbFileGaps::noGap);
		CHECK(gaps.takeSuitableGap(gapLength * 2) == DbFileGaps::noGap);
		CHECK(gaps.size() == n - 3);
		gaps.consolidateGaps();
		CHECK(gaps.size() == 1);
		CHECK(gaps.takeSuitableGap(2 * gapLength) == freeRegionStart);
		freeRegionStart += 2 * gapLength;
		CHECK(gaps.size() == 1);
		CHECK(gaps.takeSuitableGap(gapLength * 30) == freeRegionStart);
		freeRegionStart += gapLength * 30;
		CHECK(gaps.size() == 1);

		CHECK(gaps.takeSuitableGap(n * gapLength - freeRegionStart + 1) == DbFileGaps::noGap);
		gaps.registerGap(10000, 10000);
		CHECK(gaps.size() == 2);
		CHECK(gaps.takeSuitableGap(n * gapLength - freeRegionStart + 1) == 10000);
		CHECK(gaps.size() == 2);
		CHECK(gaps.takeSuitableGap(n * gapLength - freeRegionStart) == freeRegionStart);
		CHECK(gaps.size() == 1);

	} catch(...) {
		FAIL();
	}
}

TEST_CASE("White box test", "[dbfilegaps]") {
	try {

		DbFileGaps_Tester tester;
		std::vector<std::pair<uint64_t, uint64_t>> referenceGaps;
		for (uint64_t offset = 0, length = 10000; length > 0; --length, offset += length)
		{
			tester._gaps.registerGap(offset, length);
			referenceGaps.emplace_back(offset, length);
		}

		std::sort(begin_to_end(referenceGaps), [](auto&& gapL, auto&& gapR) {return gapL.first < gapR.first;});
		CHECK(std::equal(begin_to_end(referenceGaps), begin_to_end(tester.enumerateGaps())), )
	}
	catch (...) {
		FAIL();
	}
}