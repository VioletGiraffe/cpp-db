#pragma once

#include <optional>
#include <stdint.h>
#include <tuple>
#include <utility>

enum class OpCode : uint8_t {
	Insert,
	Find,
	UpdateFull,
	AppendToArray,
	Delete,

	Invalid
};

namespace Operation {

	namespace detail {
		template <class Record>
		struct RecordMember {
			static_assert(Record::isRecord());
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

	template <class Record>
	struct Insert
	{
		static_assert(Record::isRecord());

		static constexpr auto op = OpCode::Insert;

		explicit constexpr Insert(Record r) noexcept : _record{ std::move(r) }
		{}

		const Record _record;
	};

	template <typename... Fields>
	struct Find
	{
		static constexpr auto op = OpCode::Find;

		static constexpr size_t maxFieldCount = 8;
		static_assert(sizeof...(Fields) < maxFieldCount);

		template <typename... V>
		explicit constexpr Find(V&&... values) noexcept : _fields{ std::forward<V>(values)... }
		{}

		constexpr Find() noexcept = default; // TODO: remove - this is for debugging only

		// TODO: static_assert that all the fields are different (unique)
		const std::tuple<Fields...> _fields;
	};

	template <class Record, class K, bool InsertIfNotPresent = false>
	struct UpdateFull
	{
		using KeyField = K;

		static constexpr auto op = OpCode::UpdateFull;
		static constexpr bool insertIfNotPresent() noexcept { return InsertIfNotPresent; };

		explicit constexpr UpdateFull(Record r, typename KeyField::ValueType key) noexcept : record{ std::move(r) }, keyValue{ std::move(key) }
		{}

		const Record record;
		const typename KeyField::ValueType keyValue;

	private:
		static_assert(Record::isRecord());
		static_assert(Record::template has_field_v<KeyField>);
	};

	template <class Record, class Key, class Array, bool InsertIfNotPresent = false>
	struct AppendToArray final : public std::conditional_t<InsertIfNotPresent, detail::RecordMember<Record>, detail::ArrayMember<Array>>
	{
		using KeyField = Key;
		using ArrayField = Array;
		using KeyValueType = typename KeyField::ValueType;
		using ArrayValueType = typename ArrayField::ValueType;

		static_assert(Record::template has_field_v<KeyField>);

		static constexpr auto op = OpCode::AppendToArray;

		static constexpr bool insertIfNotPresent() noexcept { return InsertIfNotPresent; };

		AppendToArray(KeyValueType k, ArrayValueType a) noexcept requires(InsertIfNotPresent == false) :
			detail::ArrayMember<Array>{std::move(a)}, keyValue{ std::move(k) }
		{}

		AppendToArray(KeyValueType k, Record r) noexcept requires(InsertIfNotPresent == true) :
			detail::RecordMember<Record>{ std::move(r) }, keyValue{ std::move(k) }
		{}

		constexpr const ArrayValueType& updatedArray() const noexcept {
			if constexpr (!InsertIfNotPresent)
				return this->array;
			else
				return this->record.template fieldValue<ArrayField>();
		}

		const KeyValueType keyValue;
	};

	template <class Record, class K>
	struct Delete
	{
		using KeyField = K;

		static_assert(Record::isRecord());
		static_assert(Record::template has_field_v<KeyField>);

		static constexpr auto op = OpCode::Delete;

		explicit constexpr Delete(typename KeyField::ValueType k) noexcept : keyValue{ std::move(k) }
		{}

		const typename KeyField::ValueType keyValue;
	};

}
