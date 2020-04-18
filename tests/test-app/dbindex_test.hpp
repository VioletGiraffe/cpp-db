#pragma once

#include "3rdparty/catch2/catch.hpp"
#include "index_test_helpers.hpp"

#include "index/dbindex.hpp"
#include "container/algorithms.hpp"

#include <random>
#include <string>

TEST_CASE("Simple interface test", "[dbindices]") {

	try {
		using Fs = Field<std::string, 0>;
		using F1 = Field<int, 1>;
		using F2 = Field<float, 2>;
		

	} catch(...) {
		FAIL();
	}
}

TEST_CASE("Storing / loading", "[dbindices]") {
	try {
#ifdef _DEBUG
		constexpr uint64_t N = 10000;
#else
		constexpr uint64_t N = 400000;
#endif

		SECTION("int") {
			DbIndex<Field<uint64_t, 0>> index;
			const auto reference = fillIndexRandomly(index, N);
			const auto storePath = Index::store(index, ".");
			REQUIRE(storePath);

			index.clear();
			REQUIRE(index.empty());

			const auto loadPath = Index::load(index, ".");
			REQUIRE(loadPath);
			REQUIRE(std::equal(cbegin_to_end(index), cbegin_to_end(reference)));

			REQUIRE(*loadPath == *storePath);
			CHECK(QFile::remove(QString::fromStdString(*loadPath)));
		}

		SECTION("std::string") {
			DbIndex<Field<std::string, 0>> index;
			const auto reference = fillIndexRandomly(index, N);
			const auto storePath = Index::store(index, ".");
			REQUIRE(storePath);

			index.clear();
			REQUIRE(index.empty());

			const auto loadPath = Index::load(index, ".");
			REQUIRE(loadPath);
			REQUIRE(std::equal(cbegin_to_end(index), cbegin_to_end(reference)));

			REQUIRE(*loadPath == *storePath);
			CHECK(QFile::remove(QString::fromStdString(*loadPath)));
		}
	}
	catch (...) {
		FAIL();
	}
}