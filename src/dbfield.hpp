#pragma once

#include "utility/template_magic.hpp"

#include <string>
#include <type_traits>

template<typename T>
size_t valueSize(T&& /*value*/);

template<>
inline size_t valueSize(const std::string& value) {
	return sizeof(uint32_t) + value.size();
}

template <typename T, auto fieldId>
struct Field {
	using ValueType = T;
	static constexpr auto id = fieldId;

	inline Field() = default;

	Field(const Field<T, fieldId>&) = default;
	Field(Field<T, fieldId>&&) = default;

	Field(T val) : value{ std::move(val) }
	{}

	static constexpr bool hasStaticSize()
	{
		return std::is_trivial_v<ValueType> && std::is_standard_layout_v<ValueType>;
	}

	static constexpr size_t staticSize()
	{
		static_assert(hasStaticSize(), "This field type does not have compile-time-static size.");
		return sizeof(ValueType);
	}

	size_t fieldSize() const
	{
		if constexpr (hasStaticSize())
			return staticSize();
		else
			return ::valueSize(value);
	}

	bool operator<(const Field<T, fieldId>& other) const
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
