#pragma once

#include "3rdparty/catch2/catch.hpp"
#include "dbindex.hpp"

TEST_CASE("Simple interface test (black box)", "[dbfilegaps]") {
	try {
		using F1 = Field<int, 0>;
		DbIndex<F1> index;

		REQUIRE(index.contents().empty());

	} catch(...) {
		FAIL();
	}
}
