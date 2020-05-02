#include "3rdparty/catch2/catch.hpp"
#include "dbfilegaps_tester.hpp"
#include "storage/storage_qt.hpp"
#include "random/randomnumbergenerator.h"
#include "system/processfilepath.hpp"

#include <QFileInfo>

TEST_CASE("Storing and reloading a large set", "[dbfilegaps]") {
	try {
		DbFileGaps_Tester tester;
		RandomNumberGenerator<uint64_t> rng(0, 1, 1000000000);
#ifdef _DEBUG
		constexpr size_t n = 3000;
#else
		constexpr size_t n = 100000;
#endif
		for (uint64_t i = 0, offset = 0, length = rng.rand(); i < n; ++i)
		{
			tester._gaps.registerGap(offset, length);
			offset += length;
			length = rng.rand();
		}

		REQUIRE(tester._gaps.size() == n);

		const auto reference = tester.enumerateGaps();
		const auto fileMapPath = QFileInfo(QString::fromStdWString(processFilePath())).absolutePath().toStdString() + "/test_file_allocation_map.map";
		REQUIRE(tester._gaps.saveToFile<io::QFileAdapter>(fileMapPath));

		tester._gaps.clear();
		REQUIRE(tester._gaps.size() == 0);
		REQUIRE(tester.enumerateGaps() == decltype(reference){});

		REQUIRE(tester._gaps.loadFromFile<io::QFileAdapter>(fileMapPath));
		REQUIRE(tester.enumerateGaps() == reference);

		REQUIRE(QFileInfo(QString::fromStdString(fileMapPath)).exists());

		tester._gaps.consolidateGaps();
		REQUIRE(tester._gaps.size() == 1);
		const auto newReference = tester.enumerateGaps();
		REQUIRE(newReference.size() == 1);

		REQUIRE(tester._gaps.saveToFile<io::QFileAdapter>(fileMapPath));
		tester._gaps.clear();
		REQUIRE(tester._gaps.size() == 0);
		REQUIRE(tester.enumerateGaps() == decltype(reference){});

		REQUIRE(tester._gaps.loadFromFile<io::QFileAdapter>(fileMapPath));
		REQUIRE(tester.enumerateGaps() != reference);
		REQUIRE(tester.enumerateGaps() == newReference);

	} catch(...) {
		FAIL();
	}
}

TEST_CASE("Storing and reloading an empty set", "[dbfilegaps]") {
	try {
		DbFileGaps_Tester tester;
		REQUIRE(tester._gaps.size() == 0);

		const auto reference = tester.enumerateGaps();
		REQUIRE((reference.empty() && reference == decltype(reference){}));
		const auto fileMapPath = QFileInfo(QString::fromStdWString(processFilePath())).absolutePath() + "/test_file_allocation_map.map";
		REQUIRE(tester._gaps.saveToFile<io::QFileAdapter>(fileMapPath.toStdString()));
		REQUIRE(QFileInfo(fileMapPath).exists());
		CHECK(QFileInfo(fileMapPath).size() == 2 * sizeof(uint64_t));

		REQUIRE(tester._gaps.loadFromFile<io::QFileAdapter>(fileMapPath.toStdString()));
		REQUIRE(tester._gaps.size() == 0);
		REQUIRE(tester.enumerateGaps() == decltype(reference){});
	}
	catch (...) {
		FAIL();
	}
}
