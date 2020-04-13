#pragma once

#include "3rdparty/catch2/catch.hpp"
#include "index/dbindex.hpp"

#include <string>
#include <list>

TEST_CASE("Simple interface test", "[dbindex]") {
	try {
		using F1 = Field<std::string, 0>;
		DbIndex<F1> index;

		REQUIRE(index.begin() == index.end());
		REQUIRE(std::distance(index.begin(), index.end()) == 0);

		CHECK(index.addLocationForValue("123", { 150 }));
		CHECK(index.addLocationForValue("123", { 10 }));
		CHECK(index.addLocationForValue("023", { 11 }));
		CHECK(index.addLocationForValue("023", { 11 }) == false);

		// Can't use vector because the items are immovable
		std::list<std::pair<const typename decltype(index)::FieldValueType, StorageLocation>> reference;
		reference.emplace_back("023", StorageLocation{ 11 });
		reference.emplace_back("123", StorageLocation{ 150 }); // The relative order of items with duplicate key is preserved by std::multimap
		reference.emplace_back("123", StorageLocation{ 10 });

		CHECK(std::equal(cbegin_to_end(reference), cbegin_to_end(index)));

		auto locations = index.findValueLocations("123");
		CHECK(std::equal(cbegin_to_end(reference), cbegin_to_end(index)));
		locations = index.findValueLocations("124");
		CHECK(locations.empty());

		CHECK(index.removeValueLocation("1", { 0 }) == false);
		locations = index.findValueLocations("123");
		CHECK(std::equal(cbegin_to_end(reference), cbegin_to_end(index)));
		{
			const std::vector<StorageLocation> referenceResult{ {150}, {10} };
			CHECK(std::equal(cbegin_to_end(locations), cbegin_to_end(referenceResult)));
		}

		CHECK(index.removeAllValueLocations("5") == 0);
		locations = index.findValueLocations("123");
		CHECK(std::equal(cbegin_to_end(reference), cbegin_to_end(index)));
		{
			const std::vector<StorageLocation> referenceResult{ {150}, {10} };
			CHECK(std::equal(cbegin_to_end(locations), cbegin_to_end(referenceResult)));
		}

		CHECK(index.removeAllValueLocations("123") == 2);
		reference.erase(++reference.begin(), reference.end());
		CHECK(std::equal(cbegin_to_end(reference), cbegin_to_end(index)));
		locations = index.findValueLocations("123");
		CHECK(locations.empty());
		CHECK(index.begin() != index.end());

		locations = index.findValueLocations("023");
		CHECK(std::equal(cbegin_to_end(reference), cbegin_to_end(index)));
		{
			const std::vector<StorageLocation> referenceResult{ {11} };
			CHECK(std::equal(cbegin_to_end(locations), cbegin_to_end(referenceResult)));
		}
		CHECK(index.removeValueLocation("023", { 11 }) == true);
		CHECK(index.findValueLocations("023").empty());
		CHECK(index.begin() == index.end());

	} catch(...) {
		FAIL();
	}
}
