#include "3rdparty/catch2/catch.hpp"
#include "index_test_helpers.cpp"

#include "index/dbindices.hpp"
#include "container/algorithms.hpp"

#include <QFile>
#include <QDir>

#include <random>
#include <string>


TEST_CASE("Indices - simple interface test", "[dbindices]") {

	try {
		using Fs = Field<std::string, 0>;
		using F1 = Field<int, 1>;
		using F2 = Field<float, 2>;

		Indices<F1, Fs, F2> indices;

		CHECK(indices.addLocationForKey<Fs::id>("123", 0));
		CHECK(indices.addLocationForKey<Fs::id>("123", 1) == false);
		CHECK(indices.addLocationForKey<Fs::id>("0", 2));

		CHECK(indices.addLocationForKey<F2::id>(0.0f, 5));
		CHECK(indices.addLocationForKey<F2::id>(-1e35f, 20));

		CHECK(indices.addLocationForKey<Fs::id>("123", 0) == false);

		CHECK(verifyIndexContents(indices.indexForField<F1::id>(), std::vector < std::pair<int, PageNumber> > {}));
		CHECK(verifyIndexContents(indices.indexForField<Fs::id>(), std::vector < std::pair<std::string, PageNumber> > { {"0", 2}, {"123", 0} }));

		CHECK(verifyIndexContents(indices.indexForField<F2::id>(), std::vector < std::pair<float, PageNumber> > { { -1e35, 20 }, { 0, 5 }}));

		CHECK(indices.removeKey<F2::id>(-1e34f) == false);

		CHECK(indices.findKey<F2::id>(-1e35f) == PageNumber{20});
		CHECK(indices.removeKey<F2::id>(-1e35f) == true);
		CHECK(indices.findKey<F2::id>(-1e35f).has_value() == false);

		CHECK(indices.removeKey<F2::id>(-1e35f) == false);
		CHECK(indices.findKey<F2::id>(0.0f) == PageNumber{5});
		CHECK(indices.removeKey<F2::id>(0.0f));
		CHECK(indices.removeKey<F2::id>(0.0f) == false);
		CHECK(indices.findKey<F2::id>(0.0f).has_value() == false);
		CHECK(verifyIndexContents(indices.indexForField<F2::id>(), std::vector < std::pair<float, PageNumber> > {}));

		CHECK(indices.addLocationForKey<F2::id>(0.0f, 18));
		CHECK(indices.addLocationForKey<F2::id>(0.0f, 17) == false);
		CHECK(verifyIndexContents(indices.indexForField<F2::id>(), std::vector < std::pair<float, PageNumber> > { {0.0, 18} }));
		CHECK(indices.findKey<F2::id>(0.0f) == PageNumber{18});


		CHECK(indices.findKey<Fs::id>("123") == PageNumber{0});
		CHECK(indices.removeKey<Fs::id>("123"));
		CHECK(indices.findKey<Fs::id>("123").has_value() == false);
		CHECK(verifyIndexContents(indices.indexForField<Fs::id>(), std::vector < std::pair<std::string, PageNumber> > { {"0", 2} }));
	}
	catch (...) {
		FAIL();
	}
}

TEST_CASE("Indices - Storing / loading", "[dbindices]") {
	try {
#ifdef _DEBUG
		constexpr uint64_t N = 1000;
#else
		constexpr uint64_t N = 400000;
#endif

		using Fs = Field<std::string, 0>;
		using F1 = Field<int, 1>;
		using F2 = Field<float, 2>;

		Indices<F1, Fs, F2> indices;

		const auto referenceF1 = fillIndexRandomly(indices.indexForField<F1::id>(), N);
		const auto referenceF2 = fillIndexRandomly(indices.indexForField<F2::id>(), N);
		const auto referenceFs = fillIndexRandomly(indices.indexForField<Fs::id>(), N);

		REQUIRE(indices.store("."));

		decltype(indices) newIndices;
		REQUIRE(newIndices.load("."));

		for (auto&& entry : QDir(".").entryInfoList({ "*.index" }, QDir::Files))
		{
			CHECK(QFile::remove(entry.absoluteFilePath()));
		}

		REQUIRE(verifyIndexContents(newIndices.indexForField<F1::id>(), referenceF1));
		REQUIRE(verifyIndexContents(newIndices.indexForField<F2::id>(), referenceF2));
		REQUIRE(verifyIndexContents(newIndices.indexForField<Fs::id>(), referenceFs));
	}
	catch (...) {
		FAIL();
	}
}
