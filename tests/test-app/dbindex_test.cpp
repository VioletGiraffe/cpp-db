#include "3rdparty/catch2/catch.hpp"
#include "index_test_helpers.cpp"

#include "index/dbindex.hpp"
#include "container/algorithms.hpp"
#include "container/std_container_helpers.hpp"

#include <random>
#include <string>

TEST_CASE("DbIndex interface test", "[dbindex]") {

	try {
		using F1 = Field<std::string, 0>;

		DbIndex<F1> index;

		REQUIRE(index.begin() == index.end());
		REQUIRE(index.size() == 0);
		REQUIRE(index.empty());
		REQUIRE(std::distance(index.begin(), index.end()) == 0);

		CHECK(  index.addLocationForKey("123", PageNumber{ 150 }));
		CHECK(! index.addLocationForKey("123", PageNumber{ 10 }));
		CHECK(  index.addLocationForKey("023", PageNumber{ 11 }));
		CHECK(! index.addLocationForKey("023", PageNumber{ 11 }));

		std::vector<std::pair<typename decltype(index)::key_type, PageNumber>> reference;
		reference.emplace_back("023", PageNumber{ 11 });
		reference.emplace_back("123", PageNumber{ 150 });

		CHECK(std::equal(cbegin_to_end(reference), cbegin_to_end(index)));

		auto location = index.findKey("123");
		CHECK(std::equal(cbegin_to_end(reference), cbegin_to_end(index)));
		CHECK((location && location == 150));

		location = index.findKey("1");
		CHECK(std::equal(cbegin_to_end(reference), cbegin_to_end(index)));
		CHECK(!location);

		CHECK(index.removeKey("1") == 0);
		CHECK(std::equal(cbegin_to_end(reference), cbegin_to_end(index)));

		CHECK(index.removeKey("123") == 1);
		reference.erase(reference.begin() + 1, reference.end());
		CHECK(std::equal(cbegin_to_end(reference), cbegin_to_end(index)));
		location = index.findKey("123");
		CHECK(std::equal(cbegin_to_end(reference), cbegin_to_end(index)));
		CHECK(!location);

		CHECK(index.removeKey("023") == 1);
		CHECK(!index.findKey("023"));
		CHECK(index.removeKey("023") == 0);
		CHECK(index.begin() == index.end());
		CHECK(index.size() == 0);
		CHECK(index.empty());

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
			REQUIRE(index.removeKey(reference[i].first) == 1);
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

//#include "index/index_persistence.hpp"
//
//TEST_CASE("Storing / loading", "[dbindices]") {
//	try {
//#ifdef _DEBUG
//		constexpr uint64_t N = 1000;
//#else
//		constexpr uint64_t N = 400000;
//#endif
//
//		SECTION("int") {
//			DbIndex<Field<uint64_t, 0>> index;
//			const auto reference = fillIndexRandomly(index, N);
//			const auto storePath = Index::store<io::QFileAdapter>(index, ".");
//			REQUIRE(storePath);
//
//			index.clear();
//			REQUIRE(index.empty());
//
//			const auto loadPath = Index::load<io::QFileAdapter>(index, ".");
//			REQUIRE(loadPath);
//			REQUIRE(std::equal(cbegin_to_end(index), cbegin_to_end(reference)));
//
//			REQUIRE(*loadPath == *storePath);
//			CHECK(QFile::remove(QString::fromStdString(*loadPath)));
//		}
//
//		SECTION("std::string") {
//			DbIndex<Field<std::string, 0>> index;
//			const auto reference = fillIndexRandomly(index, N);
//			const auto storePath = Index::store<io::QFileAdapter>(index, ".");
//			REQUIRE(storePath);
//
//			index.clear();
//			REQUIRE(index.empty());
//
//			const auto loadPath = Index::load<io::QFileAdapter>(index, ".");
//			REQUIRE(loadPath);
//			REQUIRE(std::equal(cbegin_to_end(index), cbegin_to_end(reference)));
//
//			REQUIRE(*loadPath == *storePath);
//			CHECK(QFile::remove(QString::fromStdString(*loadPath)));
//		}
//	}
//	catch (...) {
//		FAIL();
//	}
//}
