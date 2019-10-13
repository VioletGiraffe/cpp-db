#include "dbfield.hpp"
#include "utility/template_magic.hpp"
#include "utility/constexpr_algorithms.hpp"
#include "parameter_pack/parameter_pack_helpers.hpp"

#include <tuple>

template <class... FieldsSequence>
struct DbRecord
{
private:
	static constexpr bool checkAssertions() noexcept
	{
		static_assert(sizeof...(FieldsSequence) > 0);
		// Checks that each input type is a Field<T>
		static_assert(((FieldsSequence::id != 0xDEADBEEF) && ...), "All template parameter types must be Fields!");

		static_for<1, sizeof...(FieldsSequence)>([](auto index) {
			constexpr int i = index;
			using Field1 = typename pack::type_by_index<i - 1, FieldsSequence...>;
			using Field2 = typename pack::type_by_index<i, FieldsSequence...>;
			static_assert(!(Field1::hasStaticSize() == false && Field2::hasStaticSize() == true), "All the fields with compile-time size must be grouped before fields with dynamic size.");
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
		IdType fieldIds[nFields]{ 0 };

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
	std::tuple<FieldsSequence...> _fields;

	static_assert(checkAssertions());
	static_assert(checkIdUniqueness(), "IDs of all fields in a DbRecord must be unique!");

	static_assert(CHAR_BIT == 8, "No funny business!");
	// TODO: uncomment this when MSVC fixes the bug (or implement with concepts)
	//static_assert((FieldsSequence::id != 0xDEADBEEF && ...));
};
