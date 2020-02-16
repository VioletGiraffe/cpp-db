#pragma once

#include "cpp-db.hpp"

#include <string>
#include <type_traits>

inline void cppDb_compileTimeChecks()
{
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

	DbRecord<F1, F2, Fs> record;
	const auto f2 = record.fieldValue<F2>();

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
}
