#pragma once

#include "3rdparty/catch2/catch.hpp"
#include "cppDb_compile_time_sanity_checks.hpp"

#include <string>
#include <type_traits>

TEST_CASE("Compilation tests", "[cpp-db]") {
	cppDb_compileTimeChecks();

	using Fs = Field<std::string, 42>;
	Fs fs;
	CHECK(fs.fieldSize() == 4 + 0);
	fs.value = "123";
	CHECK(fs.fieldSize() == 4 + 3);
}
