#pragma once

#include "dbfield.hpp"
#include "utility/template_magic.hpp"
#include "utility/constexpr_algorithms.hpp"
#include "parameter_pack/parameter_pack_helpers.hpp"
#include "tuple/tuple_helpers.hpp"
#include "assert/advanced_assert.h"
#include "utility/memory_cast.hpp"

#include <array>
#include <limits>
#include <string_view>
#include <tuple>


template <typename T, auto bitPattern>
struct Tombstone {
	using FieldType = T;

	static constexpr auto id = FieldType::id;
	static constexpr bool is_valid_v = true;

	static_assert(FieldType::staticSize() == sizeof(bitPattern));
	static_assert(FieldType::isField());

	[[nodiscard]] consteval static auto tombstoneValue() noexcept
	{
		return bitPattern;
	}

	template <typename U>
	[[nodiscard]] constexpr static bool isTombstoneValue(U&& value) noexcept
	{
		static_assert(sizeof(value) == sizeof(bitPattern));
		if constexpr (is_equal_comparable_v<remove_cv_and_reference_t<U>, decltype(bitPattern)>)
			return value == bitPattern;
		else
		{
			const auto localBitPattern = bitPattern;
			return ::memcmp(std::addressof(value), std::addressof(localBitPattern), sizeof(bitPattern)) == 0;
		}
	}
};

template <auto fieldId, bool is_array>
struct Tombstone<Field<std::string, fieldId, is_array>, 0>
{
	using FieldType = Field<std::string, fieldId, is_array>;

	static constexpr auto id = fieldId;
	static constexpr bool is_valid_v = true;

	[[nodiscard]] consteval static std::string_view tombstoneValue() noexcept
	{
		return std::string_view(tombstoneString, std::size(tombstoneString) - 1);
	}

	template <typename U>
	[[nodiscard]] constexpr static bool isTombstoneValue(const std::string& value) noexcept
	{
		return value == tombstoneValue();
	}

private:
	static constexpr const char tombstoneString[] = "#1IcX~|";
};

template<>
struct Tombstone<void, 0> {
	using FieldType = void;
	static constexpr auto id = -1;
	static constexpr bool is_valid_v = false;

	template <typename T>
	static constexpr bool isTombstoneValue(T&&) noexcept {
		return false;
	}
};

using NoTombstone = Tombstone<void, 0>; // For the purposes of testing DbRecord only

// The first of the fields is always the primary key
template <class TombstoneField /* TODO: concept */, typename... FieldsSequence>
class DbRecord
{
public:
// Traits
	static consteval bool isRecord() noexcept { return true; }
	static consteval bool hasTombstone() noexcept { return TombstoneField::is_valid_v; }

	template <size_t index>
	using FieldTypeByIndex_t = pack::type_by_index<index, FieldsSequence...>;

	template <auto id>
	using FieldById_t = ::FieldById_t<id, FieldsSequence...>;

	using FieldTypesPack = type_pack<FieldsSequence...>;

	using KeyField = typename FieldTypesPack::template Type<0>;

public:
	constexpr DbRecord() = default;

	template <typename... Values>
	constexpr explicit DbRecord(Values&&... values) : _fields{std::forward<Values>(values)...}
	{
		static_assert(sizeof...(Values) == sizeof...(FieldsSequence));
	}

	template <typename Field>
	constexpr const auto& fieldValue() const & noexcept
	{
		return std::get<pack::index_for_type_v<Field, FieldsSequence...>>(_fields).value;
	}

	template <typename Field>
	constexpr auto& fieldValue() & noexcept
	{
		return std::get<pack::index_for_type_v<Field, FieldsSequence...>>(_fields).value;
	}

	static consteval bool allFieldsHaveStaticSize() noexcept
	{
		bool nonStaticSizeFieldDetected = false;
		pack::for_type<FieldsSequence...>([&nonStaticSizeFieldDetected](auto type) {
			if (!decltype(type)::type::sizeKnownAtCompileTime())
				nonStaticSizeFieldDetected = true;
		});

		return nonStaticSizeFieldDetected == false;
	}

	static consteval size_t staticFieldsCount() noexcept
	{
		size_t count = 0;
		static_for<0, sizeof...(FieldsSequence)>([&count]<auto I>() {
			using FieldType = pack::type_by_index<I, FieldsSequence...>;
			if constexpr (FieldType::sizeKnownAtCompileTime() == true)
				++count;
		});

		return count;
	}

	static consteval size_t staticFieldsSize() noexcept
	{
		size_t totalSize = 0;
		static_for<0, staticFieldsCount()>([&totalSize]<auto I>() {
			using FieldType = pack::type_by_index<I, FieldsSequence...>;
			static_assert(FieldType::sizeKnownAtCompileTime() == true);

			totalSize += FieldType::staticSize();
		});

		return totalSize;
	}

	size_t totalSize() const noexcept
	{
		size_t totalSize = staticFieldsSize();
		static_for<staticFieldsCount(), sizeof...(FieldsSequence)>([&totalSize, this]<auto I>() {
			using FieldType = pack::type_by_index<I, FieldsSequence...>;
			static_assert(FieldType::sizeKnownAtCompileTime() == false);

			totalSize += std::get<I>(_fields).fieldSize();
		});

		return totalSize;
	}

	template <int index>
	constexpr auto& fieldAtIndex() & noexcept
	{
		return std::get<index>(_fields);
	}

	template <int index>
	constexpr const auto& fieldAtIndex() const & noexcept
	{
		return std::get<index>(_fields);
	}

	static consteval size_t fieldCount() noexcept
	{
		return sizeof...(FieldsSequence);
	}

	template <typename U>
	static constexpr bool isTombstoneValue(U&& value) noexcept
	{
		return TombstoneField::isTombstoneValue(std::forward<U>(value));
	}

	constexpr bool operator==(const DbRecord& other) const noexcept
	{
		return _fields == other._fields;
	}

	template <typename F>
	static constexpr bool has_field_v = pack::has_type_v<F, FieldsSequence...>;

//
// All the junk below is for compile-time correctness validation only.
//
private:
	static consteval bool checkAssertions() noexcept
	{
		static_assert(sizeof...(FieldsSequence) > 0);
		static_assert((FieldsSequence::isField() && ...), "All template parameter types must be Fields!");
		static_assert(TombstoneField::is_valid_v == true || TombstoneField::is_valid_v == false, "The first template parameter must beither Tombstone<FieldType>, or NoTombstone.");
		static_assert(!TombstoneField::is_valid_v || pack::has_type_v<typename TombstoneField::FieldType, FieldsSequence...>, "The tombstone field must be one of the fields in this DbRecord");

		static_assert(std::is_same_v<typename pack::type_by_index<0, FieldsSequence...>, std::tuple_element_t<0, std::tuple<FieldsSequence...>>>);

		static_for<1, sizeof...(FieldsSequence)>([]<auto index>() {
			using Field1 = typename pack::type_by_index<index - 1, FieldsSequence...>;
			using Field2 = typename pack::type_by_index<index, FieldsSequence...>;
			static_assert(!(Field1::sizeKnownAtCompileTime() == false && Field2::sizeKnownAtCompileTime() == true), "All the fields with compile-time size must be grouped before fields with dynamic size.");
			static_assert(pack::type_count<Field1, FieldsSequence...>() == 1 && pack::type_count<Field2, FieldsSequence...>() == 1, "Each unique field type shall only be specified once within a record!");
			static_assert(Field1::id != Field2::id, "All the fields must have different IDs!");

			static_assert(std::is_same_v<Field2, std::tuple_element_t<index, std::tuple<FieldsSequence...>>>);
		});

		return true;
	}

	static_assert(checkAssertions());
	static_assert(std::numeric_limits<unsigned char>::digits == 8, "No funny business!");
	static_assert(sizeof...(FieldsSequence) > 0);

private:
	std::tuple<FieldsSequence...> _fields;
};
