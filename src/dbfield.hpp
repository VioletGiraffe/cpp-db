#pragma once

#include "utility/template_magic.hpp"

#include <string>
#include <type_traits>

template<typename T, class = std::enable_if_t<!std::is_rvalue_reference_v<T>>>
size_t valueSize(T&& /*value*/) {
	using BasicT = remove_cv_and_reference_t<T>;
	static_assert(!std::is_pointer_v<BasicT>, "value may not have pointer type.");
	static_assert(std::is_trivial_v<BasicT> && std::is_standard_layout_v<BasicT>, "Only POD types may be trivially serialized.");

	return sizeof(BasicT);
}

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

	size_t fieldSize() const
	{
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
