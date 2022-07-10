#pragma once
#include "dbfield.hpp"

#include <string_view>

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
	static constexpr const char tombstoneString[]{ -1, -1, -1, -1 };
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

using NoTombstone = Tombstone<void, 0>; // For the purposes of testing
