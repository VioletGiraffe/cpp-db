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

template <class... FieldsSequence>
struct DbRecord
{
private:
	std::optional<size_t> _tombstoneFieldId;
	std::tuple<FieldsSequence...> _fields;

public:
	template <typename Field>
	auto fieldValue() const noexcept
	{
		static_assert(Field::is_field());
		static_assert(pack::type_count<Field, FieldsSequence...>() == 1);

		return std::get<pack::index_for_type_v<Field, FieldsSequence...>>(_fields);
	}

	template <typename T>
	void setTombstoneFieldId(T fieldId)
	{
		assert_debug_only(!_tombstoneFieldId);
		using FieldType = FieldById_t<fieldId, FieldsSequence...>;
		static_assert(!std::is_same_v<FieldType, void>);
		_tombstoneFieldId = pack::index_for_type_v<FieldType, FieldsSequence...>;
	}


//
// All the junk below is for compile-time correctness validation only.
//
private:
	static constexpr bool checkAssertions() noexcept
	{
		static_assert(sizeof...(FieldsSequence) > 0);
		static_assert((FieldsSequence::is_field() && ...), "All template parameter types must be Fields!");

		constexpr_for<1, sizeof...(FieldsSequence)>([](auto index) {
			constexpr int i = index;
			using Field1 = typename pack::type_by_index<i - 1, FieldsSequence...>;
			using Field2 = typename pack::type_by_index<i, FieldsSequence...>;
			static_assert(Field1::is_field() && Field2::is_field(), "One of the types in FieldsSequence is not a field!");
			static_assert(!(Field1::hasStaticSize() == false && Field2::hasStaticSize() == true), "All the fields with compile-time size must be grouped before fields with dynamic size.");
			static_assert(pack::type_count<Field1, FieldsSequence...>() == 1 && pack::type_count<Field2, FieldsSequence...>() == 1, "Each unique field shall only be specified once within a record!");
		});

		return true;
	}

	static constexpr bool checkIdUniqueness() noexcept
	{
		constexpr size_t nFields = sizeof...(FieldsSequence);
		if constexpr (nFields == 1)
			return true;

		using FirstField = pack::first_type<FieldsSequence...>;
		using IdType = std::remove_const_t<decltype(FirstField::id)>;
		std::array<IdType, nFields> fieldIds{};
		static_for<0, nFields>([&fieldIds](auto index) {
			constexpr int i = static_cast<int>(index);
			fieldIds[i] = pack::type_by_index<i, FieldsSequence...>::id;
		});

		constexpr_sort(fieldIds);

		for (size_t i = 0; i < nFields - 1; ++i)
		{
			if (fieldIds[i] == fieldIds[i + 1])
				return false;
		}

		return true;
	}
	
private:
	static_assert(checkAssertions());
	static_assert(checkIdUniqueness(), "IDs of all fields in a DbRecord must be unique!");

	static_assert(std::numeric_limits<unsigned char>::digits == 8, "No funny business!");
};
