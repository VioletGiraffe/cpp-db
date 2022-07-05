#pragma once

#include "db_type_concepts.hpp"

#include <optional>
#include <stdint.h>
#include <tuple>
#include <utility>

enum class OpCode : uint8_t {
	Insert,
	Find,
	UpdateFull,
	AppendToArray,
	Delete
};

namespace Operation {

	template <RecordConcept Record>
	struct Insert
	{
		using RecordType = Record;

		static constexpr auto op = OpCode::Insert;

		explicit constexpr Insert(Record r) noexcept :
			_record{ std::move(r) }
		{}

		const Record _record;
	};

	template <RecordConcept Record, typename... Fields>
	struct Find
	{
		using RecordType = Record;

		using TupleOfFields = std::tuple<Fields...>;

		static constexpr auto op = OpCode::Find;

		static constexpr size_t maxFieldCount = 2;
		static_assert(sizeof...(Fields) <= maxFieldCount);

		template <typename... V>
		explicit constexpr Find(V&&... values) noexcept :
			_fields{ std::forward<V>(values)... }
		{}

		// TODO: static_assert that all the fields are different (unique)
		const TupleOfFields _fields;
	};

	template <RecordConcept Record, class K, bool InsertIfNotPresent = false>
	struct UpdateFull
	{
		using RecordType = Record;
		using KeyField = K;

		static constexpr auto op = OpCode::UpdateFull;
		static constexpr bool insertIfNotPresent() noexcept { return InsertIfNotPresent; };

		explicit constexpr UpdateFull(Record r, typename KeyField::ValueType key) noexcept :
			record{ std::move(r) },
			keyValue{ std::move(key) }
		{}

		const Record record;
		const typename KeyField::ValueType keyValue;

	private:
		static_assert(Record::template has_field_v<KeyField>);
	};

	namespace detail {
		template <RecordConcept Record>
		struct RecordMember {
			explicit constexpr RecordMember(Record r) noexcept :
				record{ std::move(r) }
			{}

			const Record record;
		};

		template <class ArrayField>
		struct ArrayMember {
			static_assert(ArrayField::isArray());
			explicit constexpr ArrayMember(typename ArrayField::ValueType a) noexcept :
				array{ std::move(a) }
			{}

			const typename ArrayField::ValueType array;
		};
	}

	template <RecordConcept Record, class Key, class Array, bool InsertIfNotPresent = false>
	struct AppendToArray final : public std::conditional_t<InsertIfNotPresent, detail::RecordMember<Record>, detail::ArrayMember<Array>>
	{
		using RecordType = Record;
		using KeyField = Key;
		using ArrayField = Array;
		using KeyValueType = typename KeyField::ValueType;
		using ArrayValueType = typename ArrayField::ValueType;

		static constexpr auto op = OpCode::AppendToArray;

		static consteval bool insertIfNotPresent() noexcept { return InsertIfNotPresent; };

		constexpr AppendToArray(KeyValueType k, ArrayValueType a) noexcept requires(InsertIfNotPresent == false) :
			detail::ArrayMember<Array>{std::move(a)},
			keyValue{ std::move(k) }
		{}

		constexpr AppendToArray(KeyValueType k, Record r) noexcept requires(InsertIfNotPresent == true) :
			detail::RecordMember<Record>{ std::move(r) },
			keyValue{ std::move(k) }
		{}

		constexpr const ArrayValueType& updatedArray() const noexcept {
			if constexpr (!InsertIfNotPresent)
				return this->array;
			else
				return this->record.template fieldValue<ArrayField>();
		}

		const KeyValueType keyValue;
	};

	template <RecordConcept Record, class K>
	struct Delete
	{
		using RecordType = Record;
		using KeyField = K;

		static constexpr auto op = OpCode::Delete;

		explicit constexpr Delete(typename KeyField::ValueType k) noexcept :
			keyValue{ std::move(k) }
		{}

		const typename KeyField::ValueType keyValue;

		static_assert(Record::template has_field_v<KeyField>);
	};
}
