#pragma once

namespace detail {

template <size_t Index, auto id, typename... EmptyTypesList>
struct FieldToIndex {
	static constexpr size_t value = static_cast<size_t>(-1);
};


template <size_t Index, auto id, typename Head, typename... Rest>
struct FieldToIndex<Index, id, Head, Rest...> {
	static constexpr size_t value = Head::id == id ? Index : FieldToIndex<Index + 1, id, Rest...>::value;
};

template <auto id, typename... Args>
inline constexpr size_t indexByFieldId = FieldToIndex<0, id, Args...>::value;

} // namespace detail
