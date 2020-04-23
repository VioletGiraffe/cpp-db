#pragma once

#include "3rdparty/catch2/catch.hpp"
#include "dbrecord.hpp"

#include <limits>
#include <string>


TEST_CASE("DbRecord - basic functionality", "[dbrecord]") {

	try {
		using F3 = Field<long double, 4>;
		using F_ull = Field<uint64_t, 4>;
		using Fs = Field<std::string, 42>;

		DbRecord<F_ull, F3, F_ull, Fs> record;

		constexpr size_t stringCharacterCountFieldSize = 4 /* the size of the character count filed for strings */;

		CHECK(record.totalSize() == record.staticFieldsSize() + stringCharacterCountFieldSize);

		record.fieldValue<F3>() = 3.14;
		record.fieldValue<F_ull>() = 42;
		CHECK(record.totalSize() == record.staticFieldsSize() + stringCharacterCountFieldSize);
		CHECK(record.fieldAt<0>().value == 3.14);
		static_assert(record.staticFieldsSize() == sizeof(uint64_t) + sizeof(long double));

		record.fieldValue<Fs>() = "abcd";
		CHECK(record.totalSize() == record.staticFieldsSize() + 4 + stringCharacterCountFieldSize);
		CHECK(record.fieldAt<0>().value == 3.14);
		CHECK(record.fieldAt<1>().value == 42);
		CHECK(record.fieldAt<2>().value == std::string{ "abcd" });

		record.fieldAt<1>().value = std::numeric_limits<uint64_t>::max() - 5;
		CHECK(record.fieldAt<0>().value == 3.14);
		CHECK(record.fieldAt<1>().value == std::numeric_limits<uint64_t>::max() - 5);
		CHECK(record.fieldAt<2>().value == std::string{ "abcd" });
		CHECK(record.totalSize() == record.staticFieldsSize() + 4 + stringCharacterCountFieldSize);

		const auto copy = record;
		CHECK(copy.fieldAt<0>().value == 3.14);
		CHECK(copy.fieldAt<1>().value == std::numeric_limits<uint64_t>::max() - 5);
		CHECK(copy.fieldAt<2>().value == std::string{ "abcd" });
		CHECK(copy.totalSize() == record.staticFieldsSize() + 4 + stringCharacterCountFieldSize);
	}
	catch (...) {
		FAIL();
	}
}
