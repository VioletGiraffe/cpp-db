#include "3rdparty/catch2/catch.hpp"
#include "dbrecord.hpp"

#include <limits>
#include <string>

TEST_CASE("DbRecord - construction from a pack of values", "[dbrecord]") {

	try {
		using Fd= Field<double, 4>;
		using Fi = Field<uint64_t, 5>;
		using Fs = Field<std::string, 42>;

		{
			const DbRecord<Fd, Fi, Fs> record{ 3.14, 123, "abc" };
			CHECK(record.fieldValue<Fd>() == 3.14);
			CHECK(record.fieldValue<Fi>() == 123);
			CHECK(record.fieldValue<Fs>() == "abc");

			CHECK(record.fieldAtIndex<0>().value == 3.14);
			CHECK(record.fieldAtIndex<1>().value == 123);
			CHECK(record.fieldAtIndex<2>().value == "abc");
		}

		{
			constexpr DbRecord<Fd, Fi> record2{ 3.14, 123 };
			STATIC_REQUIRE(record2.fieldValue<Fd>() == 3.14);
			STATIC_REQUIRE(record2.fieldValue<Fi>() == 123);

			STATIC_REQUIRE(record2.fieldAtIndex<0>().value == 3.14);
			STATIC_REQUIRE(record2.fieldAtIndex<1>().value == 123);
		}

		{
			constexpr DbRecord<Fi> singleField{ 42 };
			STATIC_REQUIRE(singleField.fieldValue<Fi>() == 42);
			STATIC_REQUIRE(singleField.allFieldsHaveStaticSize());
			STATIC_REQUIRE(singleField.staticFieldsSize() == sizeof(uint64_t));
			//STATIC_REQUIRE(singleField.hasTombstone());
			//STATIC_REQUIRE(singleField.isTombstoneValue(42ull));
		}

		{
			
			const DbRecord<Fs> singleDynamicField{ "def" };
			CHECK(singleDynamicField.fieldValue<Fs>() == "def");
			CHECK(singleDynamicField.fieldAtIndex<0>().value == "def");
		}

		{
			DbRecord<Field<std::string, 564, true>> dynamicFieldArray;
			dynamicFieldArray.fieldAtIndex<0>().value.emplace_back("def");
			CHECK(dynamicFieldArray.fieldAtIndex<0>().value == std::vector{ std::string{"def"} });
		}
	}
	catch (...) {
		FAIL();
	}
}

TEST_CASE("DbRecord - static fields handling", "[dbrecord]") {

	try {
		using Fd = Field<double, 4>;
		using Full = Field<uint64_t, 5>;
		using Fs = Field<std::string, 42>;

		{
			using Record = DbRecord<Fd>;
			STATIC_REQUIRE(Record::staticFieldsCount() == 1);
			STATIC_REQUIRE(Record::staticFieldsSize() == sizeof(double));
		}

		{
			using Record = DbRecord<Fs>;
			STATIC_REQUIRE(Record::staticFieldsCount() == 0);
			STATIC_REQUIRE(Record::staticFieldsSize() == 0);
		}

		{
			using Record = DbRecord<Fs, Field<std::string, 564>>;
			STATIC_REQUIRE(Record::staticFieldsCount() == 0);
			STATIC_REQUIRE(Record::staticFieldsSize() == 0);
		}

		{
			using Record = DbRecord<Full, Fs>;
			STATIC_REQUIRE(Record::staticFieldsCount() == 1);
			STATIC_REQUIRE(Record::staticFieldsSize() == sizeof(uint64_t));
		}

		{
			using Record = DbRecord<Full, Fd>;
			STATIC_REQUIRE(Record::staticFieldsCount() == 2);
			STATIC_REQUIRE(Record::staticFieldsSize() == sizeof(uint64_t) + sizeof(double));
		}

		{
			using Record = DbRecord<Full, Fd, Fs>;
			STATIC_REQUIRE(Record::staticFieldsCount() == 2);
			STATIC_REQUIRE(Record::staticFieldsSize() == sizeof(uint64_t) + sizeof(double));
		}

		CHECK(true);
	}
	catch (...) {
		FAIL();
	}
}


TEST_CASE("DbRecord - basic functionality", "[dbrecord]") {
	try {
		using F3 = Field<double, 4>;
		using F_ull = Field<uint64_t, 5>;
		using Fs = Field<std::string, 42>;

		DbRecord<F3, F_ull, Fs> record;

		constexpr size_t stringCharacterCountFieldSize = 4 /* the size of the character count filed for strings */;

		CHECK(record.totalSize() == record.staticFieldsSize() + stringCharacterCountFieldSize);

		record.fieldValue<F3>() = 3.14;
		record.fieldValue<F_ull>() = 42;
		CHECK(record.totalSize() == record.staticFieldsSize() + stringCharacterCountFieldSize);
		CHECK(record.fieldAtIndex<0>().value == 3.14);
		STATIC_REQUIRE(record.staticFieldsSize() == sizeof(uint64_t) + sizeof(double));

		record.fieldValue<Fs>() = "abcd";
		CHECK(record.totalSize() == record.staticFieldsSize() + 4 + stringCharacterCountFieldSize);
		CHECK(record.fieldAtIndex<0>().value == 3.14);
		CHECK(record.fieldAtIndex<1>().value == 42);
		CHECK(record.fieldAtIndex<2>().value == std::string{ "abcd" });

		record.fieldAtIndex<1>().value = std::numeric_limits<uint64_t>::max() - 5;
		CHECK(record.fieldAtIndex<0>().value == 3.14);
		CHECK(record.fieldAtIndex<1>().value == std::numeric_limits<uint64_t>::max() - 5);
		CHECK(record.fieldAtIndex<2>().value == std::string{ "abcd" });
		CHECK(record.totalSize() == record.staticFieldsSize() + 4 + stringCharacterCountFieldSize);

		const auto copy = record;
		CHECK(copy.fieldAtIndex<0>().value == 3.14);
		CHECK(copy.fieldAtIndex<1>().value == std::numeric_limits<uint64_t>::max() - 5);
		CHECK(copy.fieldAtIndex<2>().value == std::string{ "abcd" });
		CHECK(copy.totalSize() == record.staticFieldsSize() + 4 + stringCharacterCountFieldSize);
	}
	catch (...) {
		FAIL();
	}
}

TEST_CASE("DbRecord - array fields", "[dbrecord]") {
	try {
		using F3 = Field<double, 4, true>;
		using Fs = Field<std::string, 42, true>;

		DbRecord<F3, Fs> record;
		STATIC_REQUIRE(record.allFieldsHaveStaticSize() == false);
		STATIC_REQUIRE(record.staticFieldsCount() == 0);
		STATIC_REQUIRE(record.staticFieldsSize() == 0);

		REQUIRE(record.totalSize() == 2 * 4);
		record.fieldValue<F3>().emplace_back(3.14);
		REQUIRE(record.totalSize() == 2 * 4 + sizeof(double));
		record.fieldValue<Fs>().emplace_back("a");
		record.fieldValue<Fs>().emplace_back("bc");
		REQUIRE(record.totalSize() == 2 * 4 + sizeof(double) + 2 * 4 + 1 + 2);
	}
	catch (...) {
		FAIL();
	}
}
