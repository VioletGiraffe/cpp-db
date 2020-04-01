#pragma once

#include <QIODevice>

#include <type_traits>

template <typename T>
inline bool checkedWrite(QIODevice& device, T value)
{
	static_assert(std::is_trivially_copy_assignable_v<T>);
	static_assert(std::is_trivially_constructible_v<T>);

	constexpr auto size = sizeof(T);
	return device.write(reinterpret_cast<const char*>(std::addressof(value)), size) == size;
}

template <typename T>
inline bool checkedRead(QIODevice& device, T& value)
{
	static_assert(std::is_trivially_copy_assignable_v<T>);
	static_assert(std::is_trivially_constructible_v<T>);

	constexpr auto size = sizeof(T);
	return device.read(reinterpret_cast<char*>(std::addressof(value)), size) == size;
}
