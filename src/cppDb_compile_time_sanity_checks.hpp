#pragma once

#include "cpp-db.hpp"
#include "storage/storage_qt.hpp"

#include <assert.h>
#include <string>
#include <type_traits>

inline void dbRecord_checks()
{
	using F3 = Field<long double, 4>;
	using F_ull = Field<uint64_t, 4>;
	using Fs = Field<std::string, 42>;

	DbRecord<F_ull, F3, F_ull, Fs> record;
	static_assert(record.staticFieldsCount() == 2);
	static_assert(record.staticFieldsSize() == sizeof(uint64_t) + sizeof(long double));
	static_assert(record.isRecord());
	static_assert(record.allFieldsHaveStaticSize() == false);
	static_assert(record.fieldCount() == 3);

	assert(record.totalSize() == record.staticFieldsSize());
}

inline void dbField_checks()
{
	using F3 = Field<long double, 4>;
	using F_ull = Field<uint64_t, 4>;
	using Fs = Field<std::string, 42>;

	constexpr F3 f3 = 3.14;
	static_assert(f3.value == 3.14);
	const auto f3_copy = f3;

	Fs fs{ std::string{"abc"} };
	const auto copy = fs;
}

inline void cppDb_compileTimeChecks()
{
	volatile int guard = 0;
	if (guard == 0)
		return;

	dbField_checks();

	using F1 = Field<int, 0>;
	using F2 = Field<float, 1>;
	using F3 = Field<long double, 4>;
	using F_ull = Field<uint64_t, 4>;
	using Fs = Field<std::string, 42>;

	static_assert(Fs::sizeKnownAtCompileTime() == false, "FAIL!");
	static_assert(F1::sizeKnownAtCompileTime() && F3::sizeKnownAtCompileTime(), "FAIL!");
	static_assert(F3::staticSize() == sizeof(long double), "FAIL!");
	static_assert(std::is_same_v<F3::ValueType, long double>, "FAIL!");

	static_assert(F1{}.staticSize() == sizeof(F1::ValueType));
	static_assert(std::is_same_v<F1::ValueType, int>);

	static_assert(F2{}.staticSize() == sizeof(F2::ValueType));
	static_assert(std::is_same_v<F2::ValueType, float>);

	static_assert(F3{}.staticSize() == sizeof(F3::ValueType));
	static_assert(std::is_same_v<F3::ValueType, long double>);

	static_assert(F_ull{}.staticSize() == sizeof(F_ull::ValueType));
	static_assert(std::is_same_v<F_ull::ValueType, uint64_t>);

	static_assert(Fs::sizeKnownAtCompileTime() == false);
	static_assert(std::is_same_v<Fs::ValueType, std::string>);

	static_assert(Fs::sizeKnownAtCompileTime() == false);

	static_assert(F2::staticSize() == sizeof(float));
	static_assert(F1::staticSize() == sizeof(int));
	static_assert(F_ull::staticSize() == sizeof(uint64_t));
	static_assert(F3::staticSize() == sizeof(long double));

	DbRecord<Tombstone<F2>, F1, F2, Fs> record;
	const auto f2 = record.fieldValue<F2>();
	static_assert(std::is_same_v<std::remove_cv_t<decltype(f2)>, float>);

	static_assert(record.allFieldsHaveStaticSize() == false);
	static_assert(decltype(record)::allFieldsHaveStaticSize() == false);
	static_assert(DbRecord<Tombstone<F1>, F1, F3>::allFieldsHaveStaticSize() == true);

	static_assert(pack::index_for_type<F1, F2, Fs, F1>() == 2);
	static_assert(pack::index_for_type<F3, F2, Fs, F1>().has_value() == false);
	static_assert(pack::has_type_v<F3, F2, Fs, F1> == false);
	constexpr size_t i = pack::index_for_type_v<Fs, F2, Fs, F1>;
	static_assert(i == 1);

	DbIndex<F1> f1Index;

	static_assert(std::is_same_v<F3, FieldById_t<F3::id, F1, F2, F3, Fs>>);
	static_assert(std::is_same_v<Fs, FieldById_t<Fs::id, F1, F2, F3, Fs>>);
	static_assert(std::is_same_v<void, FieldById_t<Fs::id, F1, F2, F3>>);
	static_assert(std::is_same_v<Fs, FieldById_t<Fs::id, Fs>>);
	static_assert(std::is_same_v<void, FieldById_t<Fs::id, F3>>);

	static_assert(std::is_same_v<std::string, FieldValueTypeById_t<Fs::id, Fs>>);
	static_assert(std::is_same_v<F3::ValueType, FieldValueTypeById_t<F3::id, F1, F2, F3, Fs>>);

	FileAllocationManager gaps;

	Indices<> noIndices;
	Indices<F1> singleFieldIndex;
	Indices<F2, F1, Fs> threeIndices;

	[[maybe_unused]] bool added = threeIndices.addLocationForValue<Fs::id>("123", 0);
	[[maybe_unused]] std::vector<StorageLocation> locs = threeIndices.findValueLocations<Fs::id>("123");
	locs = threeIndices.findValueLocations<F2::id>(3.14f);
	locs = threeIndices.findValueLocations<F1::id>(31);
	[[maybe_unused]] size_t nRemoved = threeIndices.removeAllValueLocations<Fs::id>({});
	[[maybe_unused]] bool success = threeIndices.load(".");
	success = threeIndices.store(".");

	static_assert(decltype(threeIndices)::hasIndex<F3>() == false);
	static_assert(decltype(noIndices)::hasIndex<F3>() == false);

	static_assert(Indices<>::hasIndex<F1::id>() == false);
	static_assert(Indices<F1>::hasIndex<F2::id>() == false);
	static_assert(Indices<F1>::hasIndex<F1::id>() == true);
	static_assert(Indices<Fs, F1>::hasIndex<F2::id>() == false);
	static_assert(Indices<Fs, F1>::hasIndex<F1::id>() == true);
	static_assert(Indices<Fs, F1>::hasIndex<Fs::id>() == true);

	auto& f1Index1 = singleFieldIndex.indexForField<F1::id>();
	auto& fsIndex = threeIndices.indexForField<Fs::id>();

	auto storePath = Index::store<io::QFileAdapter>(fsIndex, "Z:Z:");
	storePath = Index::load<io::QFileAdapter>(fsIndex, "Z:Z:");

	fsIndex.addLocationForValue("123", { 10 });
	fsIndex.findValueLocations("123");
	fsIndex.removeValueLocation("1", { 0 });
	f1Index1.removeAllValueLocations(5);

	dbRecord_checks();
}
