#include "3rdparty/catch2/catch.hpp"
#include "cppDb_compile_time_sanity_checks.hpp"

#include <string>
#include <type_traits>

TEST_CASE("Compilation tests", "[cpp-db]") {
	cppDb_compileTimeChecks();

	using Fs = Field<std::string, 42>;
	static_assert(Fs::isArray() == false);
	static_assert(Fs::sizeKnownAtCompileTime() == false);

	Fs fs;
	CHECK(fs.fieldSize() == 4 + 0);
	fs.value = "123";
	CHECK(fs.fieldSize() == 4 + 3);

	using Fa = Field<short, 0, true>;
	Fa arr;
	static_assert(Fa::isSuitableForTombstone() == false);
	static_assert(Fa::sizeKnownAtCompileTime() == false);
	static_assert(Fa::isArray() == true);

	CHECK(arr.fieldSize() == 4);
	arr.value.emplace_back(123);
	CHECK(arr.fieldSize() == 4 + 2);
	
}
