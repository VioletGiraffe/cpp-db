#pragma once
#include "utility/extra_type_traits.hpp"

#include <QIODevice>

template <typename T>
inline bool checkedWrite(QIODevice& device, T&& value)
{
	static_assert(std::is_trivially_copy_assignable_v<remove_cv_and_reference_t<T>>);
	static_assert(std::is_trivially_constructible_v<remove_cv_and_reference_t<T>>);

	constexpr auto size = sizeof(T);
	return device.write(reinterpret_cast<const char*>(std::addressof(value)), size) == size;
}

template <typename T>
inline bool checkedRead(QIODevice& device, T& value)
{
	static_assert(std::is_trivially_copy_assignable_v<remove_cv_and_reference_t<T>>);
	static_assert(std::is_trivially_constructible_v<remove_cv_and_reference_t<T>>);

	constexpr auto size = sizeof(T);
	return device.read(reinterpret_cast<char*>(std::addressof(value)), size) == size;
}
