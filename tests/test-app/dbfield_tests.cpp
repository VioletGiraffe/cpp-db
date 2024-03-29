#include "3rdparty/catch2/catch.hpp"
#include "dbfield.hpp"

#include <string>

TEST_CASE("DbField: basic tests", "[dbfield]") {

	try {
		using F3 = Field<long double, 4>;
		using Fs = Field<std::string, 42>;
		STATIC_REQUIRE(F3::sizeKnownAtCompileTime());
		STATIC_REQUIRE(Fs::sizeKnownAtCompileTime() == false);

		constexpr F3 f3{ 3.14L }, f3_mod{ 0.0L };
		STATIC_REQUIRE(F3::isArray() == false);
		STATIC_REQUIRE(f3.value == 3.14L);
		constexpr auto f3_copy = f3;
		STATIC_REQUIRE(f3_copy.value == 3.14L);
		STATIC_REQUIRE(f3_copy == f3);
		STATIC_REQUIRE(f3_copy != f3_mod);

		Fs fs{ std::string{"abc"} };
		STATIC_REQUIRE(Fs::isArray() == false);
		const auto copy = fs;

		STATIC_REQUIRE(copy.id == fs.id);
		REQUIRE(copy == fs);
		REQUIRE(copy.value == fs.value);
		REQUIRE(copy.value == "abc");

		fs.value = "Hello!";
		REQUIRE(copy.value != fs.value);
		REQUIRE(copy != fs);
		REQUIRE(fs.value == "Hello!");
	}
	catch (...) {
		FAIL();
	}
}

TEST_CASE("DbField: arrays", "[dbfield]") {

	try {
		using F3 = Field<long double, 4, true>;
		using Fs = Field<std::string, 42, true>;

		STATIC_REQUIRE(F3::sizeKnownAtCompileTime() == false);
		STATIC_REQUIRE(Fs::sizeKnownAtCompileTime() == false);

		F3 f3 { {3.14L} };
		const F3 f3_mod { {0.0L} };

		STATIC_REQUIRE(F3::isArray() == true);
		REQUIRE((f3.value == std::vector{ 3.14L }));
		const auto f3_copy = f3;
		REQUIRE((f3_copy == std::vector{ 3.14L }));
		REQUIRE(f3_copy == f3);
		REQUIRE(f3_copy != f3_mod);

		f3.value.emplace_back(5.0L);
		REQUIRE((f3.value == std::vector{ {3.14L, 5.0L} }));
		REQUIRE(f3_copy != f3);

		f3.value.pop_back();
		f3.value.pop_back();
		f3.value.emplace_back(0.0L);
		REQUIRE(f3_copy != f3);
		REQUIRE(f3 == f3_mod);

		const Fs fs{ {std::string{"abc"}, std::string{"123"}} };
		STATIC_REQUIRE(Fs::isArray() == true);
		const auto copy = fs;

		STATIC_REQUIRE(copy.id == fs.id);
		REQUIRE(copy == fs);
		REQUIRE(copy.value == fs.value);
		REQUIRE((copy.value == std::vector{std::string{ "abc" }, std::string{ "123" }}));
	}
	catch (...) {
		FAIL();
	}
}
