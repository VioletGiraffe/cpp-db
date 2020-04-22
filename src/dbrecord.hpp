#pragma once

#include "dbfield.hpp"
#include "utility/template_magic.hpp"
#include "utility/constexpr_algorithms.hpp"
#include "parameter_pack/parameter_pack_helpers.hpp"
#include "tuple/tuple_helpers.hpp"
#include "assert/advanced_assert.h"

#include <array>
#include <limits>
#include <tuple>


template <typename T>
struct Tombstone {
	using Field = T;
	static constexpr auto id = Field::id;

	static_assert(Field::isField());
};

template <class TombstoneField /* TODO: concept */, typename... FieldsSequence>
class DbRecord
{
public:
// Traits
	template <int index>
	using FieldTypeByIndex_t = typename pack::type_by_index<index, FieldsSequence...>;

	static constexpr bool isRecord() noexcept { return true; }

public:
	constexpr DbRecord() noexcept = default;

	template <typename Field>
	auto fieldValue() const noexcept
	{
		return std::get<pack::index_for_type_v<Field, FieldsSequence...>>(_fields).value;
	}

	static constexpr bool allFieldsHaveStaticSize() noexcept
	{
		bool nonStaticSizeFieldDetected = false;
		pack::for_type<FieldsSequence...>([&nonStaticSizeFieldDetected](auto type) {
			if (!decltype(type)::type::sizeKnownAtCompileTime())
				nonStaticSizeFieldDetected = true;
		});

		return nonStaticSizeFieldDetected == false;
	}

	static constexpr size_t staticFieldsCount() noexcept
	{
		size_t count = 0;
		static_for<0, sizeof...(FieldsSequence)>([&totalSize](auto i) {
			using FieldType = pack::type_by_index<decltype(i)::value, FieldsSequence...>;
			if constexpr (FieldType::sizeKnownAtCompileTime() == true)
				++count;
		});

		return count;
	}

	static constexpr size_t staticFieldsSize() noexcept
	{
		size_t totalSize = 0;
		static_for<0, staticFieldsCount()>([&totalSize](auto i) {
			using FieldType = pack::type_by_index<decltype(i)::value, FieldsSequence...>;
			static_assert(FieldType::sizeKnownAtCompileTime() == true);

			totalSize += FieldType::staticSize();
		});

		return totalSize;
	}

	size_t totalSize() const noexcept
	{
		size_t totalSize = staticFieldsSize();
		static_for<staticFieldsCount(), sizeof...(FieldsSequence)>([&totalSize](auto i) {
			using FieldType = pack::type_by_index<decltype(i)::value, FieldsSequence...>;
			static_assert(FieldType::sizeKnownAtCompileTime() == false);

			totalSize += std::get<i>(_fields).fieldSize();
		});

		return totalSize;
	}

	template <int index>
	auto& fieldAt() & noexcept
	{
		return std::get<index>(_fields);
	}

	template <int index>
	const auto& fieldAt() const & noexcept
	{
		return std::get<index>(_fields);
	}

	static constexpr size_t fieldCount() noexcept
	{
		return sizeof...(FieldsSequence);
	}

//
// All the junk below is for compile-time correctness validation only.
//
private:
	static constexpr bool checkAssertions() noexcept
	{
		static_assert(sizeof...(FieldsSequence) > 0);
		static_assert((FieldsSequence::isField() && ...), "All template parameter types must be Fields!");
		static_assert(pack::has_type_v<typename TombstoneField::Field, FieldsSequence...>);

		static_assert(std::is_same_v<typename pack::type_by_index<0, FieldsSequence...>, std::tuple_element_t<0, std::tuple<FieldsSequence...>>>);

		constexpr_for<1, sizeof...(FieldsSequence)>([](auto index) {
			constexpr int i = index;
			using Field1 = FieldTypeByIndex_t<i - 1>;
			using Field2 = FieldTypeByIndex_t<i>;
			static_assert(!(Field1::sizeKnownAtCompileTime() == false && Field2::sizeKnownAtCompileTime() == true), "All the fields with compile-time size must be grouped before fields with dynamic size.");
			static_assert(pack::type_count<Field1, FieldsSequence...>() == 1 && pack::type_count<Field2, FieldsSequence...>() == 1, "Each unique field shall only be specified once within a record!");

			static_assert(std::is_same_v<Field2, std::tuple_element_t<i, std::tuple<FieldsSequence...>>>);
		});

		return true;
	}

	static_assert(checkAssertions());
	static_assert(std::numeric_limits<unsigned char>::digits == 8, "No funny business!");

private:
	std::tuple<FieldsSequence...> _fields;
};
