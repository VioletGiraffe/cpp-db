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
			const DbRecord<Tombstone<Fi, 0ULL>, Fd, Fi, Fs> record{ 3.14, 123, "abc" };
			CHECK(record.fieldValue<Fd>() == 3.14);
			CHECK(record.fieldValue<Fi>() == 123);
			CHECK(record.fieldValue<Fs>() == "abc");

			CHECK(record.fieldAtIndex<0>().value == 3.14);
			CHECK(record.fieldAtIndex<1>().value == 123);
			CHECK(record.fieldAtIndex<2>().value == "abc");
		}

		{
			constexpr DbRecord<Tombstone<Fi, 0ULL>, Fd, Fi> record2{ 3.14, 123 };
			static_assert(record2.fieldValue<Fd>() == 3.14);
			static_assert(record2.fieldValue<Fi>() == 123);

			static_assert(record2.fieldAtIndex<0>().value == 3.14);
			static_assert(record2.fieldAtIndex<1>().value == 123);
		}

		{
			constexpr DbRecord<Tombstone<Fi, 42ull>, Fi> singleField{ 42 };
			static_assert(singleField.fieldValue<Fi>() == 42);
			static_assert(singleField.allFieldsHaveStaticSize());
			static_assert(singleField.staticFieldsSize() == sizeof(uint64_t));
			static_assert(singleField.hasTombstone());
			static_assert(singleField.isTombstoneValue(42ull));
		}

		{
			
			const DbRecord<Tombstone<Fs, 0>, Fs> singleDynamicField{ "def" };
			CHECK(singleDynamicField.fieldValue<Fs>() == "def");
			CHECK(singleDynamicField.fieldAtIndex<0>().value == "def");
		}

		{
			DbRecord<NoTombstone, Field<std::string, 564, true>> dynamicFieldArray;
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
			using Record = DbRecord<NoTombstone, Fd>;
			static_assert(Record::staticFieldsCount() == 1);
			static_assert(Record::staticFieldsSize() == sizeof(double));
		}

		{
			using Record = DbRecord<NoTombstone, Fs>;
			static_assert(Record::staticFieldsCount() == 0);
			static_assert(Record::staticFieldsSize() == 0);
		}

		{
			using Record = DbRecord<NoTombstone, Fs, Field<std::string, 564>>;
			static_assert(Record::staticFieldsCount() == 0);
			static_assert(Record::staticFieldsSize() == 0);
		}

		{
			using Record = DbRecord<NoTombstone, Full, Fs>;
			static_assert(Record::staticFieldsCount() == 1);
			static_assert(Record::staticFieldsSize() == sizeof(uint64_t));
		}

		{
			using Record = DbRecord<NoTombstone, Full, Fd>;
			static_assert(Record::staticFieldsCount() == 2);
			static_assert(Record::staticFieldsSize() == sizeof(uint64_t) + sizeof(double));
		}

		{
			using Record = DbRecord<NoTombstone, Full, Fd, Fs>;
			static_assert(Record::staticFieldsCount() == 2);
			static_assert(Record::staticFieldsSize() == sizeof(uint64_t) + sizeof(double));
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

		DbRecord<Tombstone<F_ull, std::numeric_limits<uint64_t>::max()>, F3, F_ull, Fs> record;

		constexpr size_t stringCharacterCountFieldSize = 4 /* the size of the character count filed for strings */;

		CHECK(record.totalSize() == record.staticFieldsSize() + stringCharacterCountFieldSize);

		record.fieldValue<F3>() = 3.14;
		record.fieldValue<F_ull>() = 42;
		CHECK(record.totalSize() == record.staticFieldsSize() + stringCharacterCountFieldSize);
		CHECK(record.fieldAtIndex<0>().value == 3.14);
		static_assert(record.staticFieldsSize() == sizeof(uint64_t) + sizeof(double));

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

		DbRecord<Tombstone<F3, std::numeric_limits<uint64_t>::max()>, F3> doubleRecord;
		static_assert(doubleRecord.hasTombstone());
		CHECK(doubleRecord.isTombstoneValue(std::numeric_limits<uint64_t>::max()));
		CHECK(!doubleRecord.isTombstoneValue(std::numeric_limits<uint64_t>::max() - 1));
	}
	catch (...) {
		FAIL();
	}
}

TEST_CASE("DbRecord - array fields", "[dbrecord]") {
	try {
		using F3 = Field<double, 4, true>;
		using Fs = Field<std::string, 42, true>;

		DbRecord<NoTombstone, F3, Fs> record;
		static_assert(record.allFieldsHaveStaticSize() == false);
		static_assert(record.staticFieldsCount() == 0);
		static_assert(record.staticFieldsSize() == 0);

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
