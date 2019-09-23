#pragma once

#include "utility/template_magic.hpp"

#include <type_traits>

////////////////////////////////////////////////////////////////////
//              VALUE SIZE                                        //
// Specialize this function for a custom dynamically-sized type
// to use this type in a Field

template<typename T>
size_t valueSize(T&& /*value*/);

// A sample implementation for std::string
/*
	template<>
	inline size_t valueSize(const std::string& value) {
		return sizeof(uint32_t) + value.size();
	}
*/
////////////////////////////////////////////////////////////////////

template <typename T, auto fieldId>
struct Field {
	using ValueType = T;
	static constexpr auto id = fieldId;

	Field() = default;

	Field(const Field&) = default;
	Field(Field&&) = default;

	Field(T val) noexcept : value{ std::move(val) }
	{}

	static constexpr bool hasStaticSize() noexcept
	{
		return std::is_trivial_v<ValueType> && std::is_standard_layout_v<ValueType>;
	}

	static constexpr size_t staticSize() noexcept
	{
		static_assert(hasStaticSize(), "This field type does not have compile-time-static size.");
		return sizeof(ValueType);
	}

	template <bool staticSizeAvailable = hasStaticSize()>
	std::enable_if_t<staticSizeAvailable, size_t> fieldSize() const noexcept
	{
		return staticSize();
	}

	template <bool staticSizeAvailable = hasStaticSize()>
	std::enable_if_t<!staticSizeAvailable, size_t> fieldSize() const noexcept
	{
		return ::valueSize(value);
	}

	bool operator<(const Field& other) const
	{
		return value < other.value;
	}

public:
	T value {};
};

template <auto id, typename FirstField = void, typename... OtherFields>
struct FieldTypeById {
	using Type = std::conditional_t<FirstField::id == id, typename FirstField::ValueType, typename FieldTypeById<id, OtherFields...>::Type>;
};

// This specialization, combined with defaulting 'FirstField' to 'void' when only 'id' argument is supplied, allows for instantiating 'Indices<>' - an empty index.
template <auto id>
struct FieldTypeById<id, void> {
	using Type = void;
};

template <auto id, typename... Fields>
using FieldTypeById_t = typename FieldTypeById<id, Fields...>::Type;
