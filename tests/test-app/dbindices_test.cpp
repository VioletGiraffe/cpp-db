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

		CHECK(indices.addLocationForValue<Fs::id>("123", 0));
		CHECK(indices.addLocationForValue<Fs::id>("123", 1));
		CHECK(indices.addLocationForValue<Fs::id>("0", 2));

		CHECK(indices.addLocationForValue<F2::id>(0.0f, 5));
		CHECK(indices.addLocationForValue<F2::id>(-1e35f, 20));

		CHECK(indices.addLocationForValue<Fs::id>("123", 0) == false);

		CHECK(verifyIndexContents(indices.indexForField<F1::id>(), std::vector < std::pair<int, PageNumber> > {}));
		CHECK(verifyIndexContents(indices.indexForField<Fs::id>(), std::vector < std::pair<std::string, PageNumber> > { {"0", 2}, {"123", 0}, { "123", 1 } }));

		CHECK(verifyIndexContents(indices.indexForField<F2::id>(), std::vector < std::pair<float, PageNumber> > { { -1e35, 20 }, { 0, 5 }}));

		CHECK(indices.removeValueLocation<F2::id>(-1e35f, 200) == false);
		CHECK(indices.removeValueLocation<F2::id>(-1e34f, 20) == false);

		CHECK(indices.findValueLocations<F2::id>(-1e35f) == std::vector<PageNumber>{20});

		CHECK(indices.removeValueLocation<F2::id>(-1e35f, 20) == true);
		CHECK(indices.findValueLocations<F2::id>(-1e35f) == std::vector<PageNumber>{});
		CHECK(indices.findValueLocations<F2::id>(0.0f) == std::vector<PageNumber>{5});
		CHECK(indices.removeAllValueLocations<F2::id>(0.0f) == 1);
		CHECK(indices.removeAllValueLocations<F2::id>(0.0f) == 0);
		CHECK(indices.findValueLocations<F2::id>(0.0f) == std::vector<PageNumber>{});
		CHECK(verifyIndexContents(indices.indexForField<F2::id>(), std::vector < std::pair<float, PageNumber> > {}));

		CHECK(indices.addLocationForValue<F2::id>(0.0f, 18));
		CHECK(indices.addLocationForValue<F2::id>(0.0f, 17));
		CHECK(verifyIndexContents(indices.indexForField<F2::id>(), std::vector < std::pair<float, PageNumber> > { {0.0, 18}, {0.0, 17}}));
		CHECK(indices.findValueLocations<F2::id>(0.0f) == std::vector<PageNumber>{18, 17});


		CHECK(indices.findValueLocations<Fs::id>("123") == std::vector<PageNumber>{0, 1});
		CHECK(indices.removeAllValueLocations<Fs::id>("123") == 2);
		CHECK(indices.findValueLocations<Fs::id>("123") == std::vector<PageNumber>{});
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
