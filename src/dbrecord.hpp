#include "dbfield.hpp"
#include "utility/template_magic.hpp"
#include "utility/constexpr_algorithms.hpp"
#include "parameter_pack/parameter_pack_helpers.hpp"
#include "tuple/tuple_helpers.hpp"
#include "assert/advanced_assert.h"

#include <array>
#include <limits>
#include <optional>
#include <tuple>

template <typename... FieldsSequence>
struct DbRecord
{
private:
	static std::optional<size_t> _tombstoneFieldId;
	std::tuple<FieldsSequence...> _fields;

public:
	template <typename Field>
	auto fieldValue() const noexcept
	{
		static_assert(Field::isField());
		static_assert(pack::type_count<Field, FieldsSequence...>() == 1);

		return std::get<pack::index_for_type_v<Field, FieldsSequence...>>(_fields);
	}

	// TODO: move to compile time
	// A tombstone field uses high bit (the first one) to encode that this record has been deleted.
	template <typename T>
	static void setTombstoneFieldId(T fieldId) noexcept
	{
		assert_debug_only(!_tombstoneFieldId);
		using FieldType = FieldById_t<fieldId, FieldsSequence...>;
		static_assert(!std::is_same_v<FieldType, void> && FieldType::isSuitableForTombstone());

		_tombstoneFieldId = pack::index_for_type_v<FieldType, FieldsSequence...>;
	}

	static constexpr bool canReuseGaps() noexcept
	{
		bool nonStaticSizeFieldDetected = false;
		pack::for_type<FieldsSequence...>([&nonStaticSizeFieldDetected](auto type) {
			if (!decltype(type)::type::sizeKnownAtCompileTime())
				nonStaticSizeFieldDetected = true;
		});

		return nonStaticSizeFieldDetected == false;
	}

//
// All the junk below is for compile-time correctness validation only.
//
private:
	static constexpr bool checkAssertions() noexcept
	{
		static_assert(sizeof...(FieldsSequence) > 0);
		static_assert((FieldsSequence::isField() && ...), "All template parameter types must be Fields!");

		constexpr_for<1, sizeof...(FieldsSequence)>([](auto index) {
			constexpr int i = index;
			using Field1 = typename pack::type_by_index<i - 1, FieldsSequence...>;
			using Field2 = typename pack::type_by_index<i, FieldsSequence...>;
			static_assert(!(Field1::sizeKnownAtCompileTime() == false && Field2::sizeKnownAtCompileTime() == true), "All the fields with compile-time size must be grouped before fields with dynamic size.");
			static_assert(pack::type_count<Field1, FieldsSequence...>() == 1 && pack::type_count<Field2, FieldsSequence...>() == 1, "Each unique field shall only be specified once within a record!");
		});

		return true;
	}

	static_assert(checkAssertions());
	static_assert(std::numeric_limits<unsigned char>::digits == 8, "No funny business!");
};
