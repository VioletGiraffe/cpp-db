#pragma once

#include "utility/template_magic.hpp"
#include "dbfield_size_helpers.hpp"

#include <type_traits>
#include <utility>
#include <vector>


template <typename T, auto fieldId, bool is_array = false>
struct Field {
	static constexpr bool isField = true;

	/// Traits
	using ValueType = std::conditional_t<is_array, std::vector<T>, T>;

	static constexpr auto id = fieldId;
	static consteval bool isArray() noexcept { return is_array; }
	///

	constexpr Field() noexcept = default;
	constexpr Field(ValueType val) noexcept : value{ std::move(val) }
	{}

	static consteval bool sizeKnownAtCompileTime() noexcept
	{
		return !isArray() && std::is_trivial_v<ValueType> && std::is_standard_layout_v<ValueType>;
	}

	static consteval size_t staticSize() noexcept
	{
		static_assert(sizeKnownAtCompileTime(), "This field type does not have compile-time-static size.");
		return sizeof(ValueType);
	}

	size_t fieldSize() const noexcept
	{
		if constexpr (sizeKnownAtCompileTime())
			return staticSize();
		else
			return ::valueSize(value);
	}

	static consteval bool isSuitableForTombstone() noexcept
	{
		return std::is_trivial_v<ValueType> && sizeKnownAtCompileTime();
	}

	constexpr bool operator==(const Field& other) const noexcept
	{
		return value == other.value;
	}

	constexpr bool operator!=(const Field& other) const noexcept
	{
		return !operator==(other);
	}

	constexpr bool operator<(const Field& other) const noexcept requires(!is_array)
	{
		return value < other.value;
	}

public:
	ValueType value {};

private:
	static_assert(sizeKnownAtCompileTime() == std::is_trivial_v<ValueType>);
};

//
// Helper templates
//

template <auto id, typename FirstField = void, typename... OtherFields>
struct FieldById {
	using Type = std::conditional_t<FirstField::id == id, FirstField, typename FieldById<id, OtherFields...>::Type>;
};

// This specialization, combined with defaulting 'FirstField' to 'void' when only 'id' argument is supplied, allows for instantiating 'Indices<>' - an empty index.
template <auto id>
struct FieldById<id, void> {
	using Type = void;
};

template <auto id, typename... Fields>
using FieldById_t = typename FieldById<id, Fields...>::Type;

template <auto id, typename... Fields>
using FieldValueTypeById_t = typename FieldById_t<id, Fields...>::ValueType;
