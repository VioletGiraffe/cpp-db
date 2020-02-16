#pragma once

#include "3rdparty/catch2/catch.hpp"
#include "dbfilegaps_tester.hpp"

#ifndef TRAVIS_BUILD
constexpr uint64_t travis_downscale_factor = 1;
#else
#include <iostream>

constexpr uint64_t travis_downscale_factor = 100;
#endif

TEST_CASE("Filling empty DbFileGaps benchmark", "[dbfilegaps]") {
	try {
		FileAllocationManager gaps;

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

TEST_CASE("Taking 500 out of 20000 equal length gaps", "[dbfilegaps]") {
	try {

#ifdef TRAVIS_BUILD
		std::cout << "Travis CI build." << std::endl;
#endif

		FileAllocationManager gaps;

		constexpr uint64_t n = 20000 / travis_downscale_factor;
		for (uint64_t offset = 0, length = 1, i = 0; i < n; offset += length, ++i)
			gaps.registerGap(offset, length);

		BENCHMARK("Taking 500 out of 20000 equal length gaps") {
			for (uint64_t i = 0; i < 500 / travis_downscale_factor; ++i)
			{
				gaps.takeSuitableGap(1);
			}

			return gaps.size();
		};
	}
	catch (...) {
		FAIL();
	}
}

TEST_CASE("Taking 500 out of 40000 equal length gaps", "[dbfilegaps]") {
	try {
		FileAllocationManager gaps;

		constexpr uint64_t n = 40000 / travis_downscale_factor;

		for (uint64_t offset = 0, length = 1, i = 0; i < n; offset += length, ++i)
			gaps.registerGap(offset, length);

		BENCHMARK("Taking 500 out of 40000 equal length gaps") {
			for (uint64_t i = 0; i < 500 / travis_downscale_factor; ++i)
			{
				gaps.takeSuitableGap(1);
			}

			return gaps.size();
		};
	}
	catch (...) {
		FAIL();
	}
}

TEST_CASE("Taking 1000 out of 40000 equal length gaps", "[dbfilegaps]") {
	try {
		FileAllocationManager gaps;

		constexpr uint64_t n = 40000 / travis_downscale_factor;

		for (uint64_t offset = 0, length = 1, i = 0; i < n; offset += length, ++i)
			gaps.registerGap(offset, length);

		BENCHMARK("Taking 1000 out of 40000 equal length gaps") {
			for (uint64_t i = 0; i < 1000 / travis_downscale_factor; ++i)
			{
				gaps.takeSuitableGap(1);
			}

			return gaps.size();
		};
	}
	catch (...) {
		FAIL();
	}
}

TEST_CASE("Requesting unavailable gap 1000 times out of 40000 equal length gaps", "[dbfilegaps]") {
	try {
		FileAllocationManager gaps;
	
		constexpr uint64_t n = 40000 / travis_downscale_factor;

		for (uint64_t offset = 0, length = 1, i = 0; i < n; offset += length, ++i)
			gaps.registerGap(offset, length);

		BENCHMARK("Requesting unavailable gap 1000 times out of 40000 equal length gaps") {
			for (uint64_t i = 0; i < 1000 / travis_downscale_factor; ++i)
			{
				gaps.takeSuitableGap(n + 5);
			}

			return gaps.size();
		};
	}
	catch (...) {
		FAIL();
	}
}
