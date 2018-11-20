#pragma once

#include <string>
#include <type_traits>

template<typename T, class = std::enable_if_t<!std::is_rvalue_reference_v<T>>>
size_t valueSize(T&& /*value*/) {
	static_assert(!std::is_pointer_v<T>, "value may not have pointer type.");
	static_assert(!std::is_same_v<std::decay_t<T>, std::string>, "value may not be std::string.");

	return sizeof(T);
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