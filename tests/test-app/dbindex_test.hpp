#pragma once

#include "3rdparty/catch2/catch.hpp"
#include "index_test_helpers.hpp"
#include "storage/storage_qt.hpp"

#include "index/dbindex.hpp"
#include "container/algorithms.hpp"
#include "container/std_container_helpers.hpp"

#include <random>
#include <string>

TEST_CASE("DBindices interface test", "[dbindex]") {

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
		std::vector<std::pair<typename decltype(index)::FieldValueType, StorageLocation>> reference;
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
		reference.erase(reference.begin() + 1, reference.end());
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

TEST_CASE("Adding and removing random locations", "[dbindex]") {
	try {
		using F1 = Field<int64_t, 0>;
		DbIndex<F1> index;

#ifdef _DEBUG
		constexpr uint64_t N = 3000;
#else
		constexpr uint64_t N = 200000;
#endif
		auto reference = fillIndexRandomly(index, N);

		std::mt19937 randomizer;
		randomizer.seed(0);

		CHECK(std::equal(cbegin_to_end(index), cbegin_to_end(reference)));

		auto originalReferenceData = reference;
		// Removing half of the items randomly
		std::shuffle(begin_to_end(reference), randomizer);
		REQUIRE(N % 10 == 0);
		for (uint64_t i = 0; i < N / 10; ++i)
		{
			index.removeValueLocation(reference[i].first, reference[i].second);
			ContainerAlgorithms::erase_all_occurrences(originalReferenceData, reference[i]);
		}

		REQUIRE(originalReferenceData.size() == (reference.size() - N / 10));
		REQUIRE(originalReferenceData.size() == index.size());

		CHECK(std::equal(cbegin_to_end(index), cbegin_to_end(originalReferenceData)));
	}
	catch (...) {
		FAIL();
	}
}

#include "index/index_persistence.hpp"

TEST_CASE("Storing / loading", "[dbindices]") {
	try {
#ifdef _DEBUG
		constexpr uint64_t N = 3000;
#else
		constexpr uint64_t N = 400000;
#endif

		SECTION("int") {
			DbIndex<Field<uint64_t, 0>> index;
			const auto reference = fillIndexRandomly(index, N);
			const auto storePath = Index::store<io::QFileAdapter>(index, ".");
			REQUIRE(storePath);

			index.clear();
			REQUIRE(index.empty());

			const auto loadPath = Index::load<io::QFileAdapter>(index, ".");
			REQUIRE(loadPath);
			REQUIRE(std::equal(cbegin_to_end(index), cbegin_to_end(reference)));

			REQUIRE(*loadPath == *storePath);
			CHECK(QFile::remove(QString::fromStdString(*loadPath)));
		}

		SECTION("std::string") {
			DbIndex<Field<std::string, 0>> index;
			const auto reference = fillIndexRandomly(index, N);
			const auto storePath = Index::store<io::QFileAdapter>(index, ".");
			REQUIRE(storePath);

			index.clear();
			REQUIRE(index.empty());

			const auto loadPath = Index::load<io::QFileAdapter>(index, ".");
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
