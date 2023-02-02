#include "3rdparty/catch2/catch.hpp"
#include "cppDb_compile_time_sanity_checks.hpp"

#include <string>
#include <type_traits>

TEST_CASE("Compilation tests", "[compilation]") {
	cppDb_compileTimeChecks();

	using Fs = Field<std::string, 42>;
	STATIC_REQUIRE(Fs::isArray() == false);
	STATIC_REQUIRE(Fs::sizeKnownAtCompileTime() == false);

	Fs fs;
	CHECK(fs.fieldSize() == 4 + 0);
	fs.value = "123";
	CHECK(fs.fieldSize() == 4 + 3);

	using Fa = Field<int16_t, 0, true>;
	Fa arr;
	STATIC_REQUIRE(Fa::isSuitableForTombstone() == false);
	STATIC_REQUIRE(Fa::sizeKnownAtCompileTime() == false);
	STATIC_REQUIRE(Fa::isArray() == true);

	CHECK(arr.fieldSize() == 4);
	arr.value.emplace_back(123_i16);
	CHECK(arr.fieldSize() == 4 + 2);

	Field<std::string, 42, true> strArray;
	STATIC_REQUIRE(strArray.isSuitableForTombstone() == false);
	STATIC_REQUIRE(strArray.sizeKnownAtCompileTime() == false);
	STATIC_REQUIRE(strArray.isArray() == true);

	CHECK(strArray.fieldSize() == 4);
	strArray.value.emplace_back("123");
	CHECK(strArray.fieldSize() == 4 + 4 + 3);
	strArray.value.emplace_back("a");
	CHECK(strArray.fieldSize() == 4 + (4 + 3) + (4 + 1));
}
