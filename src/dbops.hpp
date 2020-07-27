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

	template <class Record, class KeyField, bool InsertIfNotPresent = false>
	struct UpdateFull
	{
		static_assert(Record::isRecord());
		static_assert(Record::template has_field_v<KeyField>);

		static constexpr auto op = OpCode::UpdateFull;
		static constexpr bool insertIfNotPresent = InsertIfNotPresent;

		explicit constexpr UpdateFull(typename KeyField::ValueType key, Record r = {}) noexcept : record{ std::move(r) }, keyValue{ std::move(key) }
		{}

		const Record record;
		const typename KeyField::ValueType keyValue;
	};

	template <class Record, class KeyField, class ArrayField, bool InsertIfNotPresent = false>
	struct AppendToArray
	{
		static_assert(Record::isRecord());
		static_assert(Record::template has_field_v<KeyField>);
		static_assert(Record::template has_field_v<ArrayField>);
		static_assert(ArrayField::isArray());

		static constexpr auto op = OpCode::AppendToArray;

		AppendToArray(KeyField k, ArrayField a, Record r = {}) noexcept : key{std::move(k)}, array{std::move(a)}, record{std::move(r)}
		{}

		const KeyField key;
		const ArrayField array;
		const Record record;
	};

	template <class Record, class KeyField>
	struct Delete
	{
		static_assert(Record::isRecord());
		static_assert(Record::template has_field_v<KeyField>);

		static constexpr auto op = OpCode::Delete;

		explicit constexpr Delete(typename KeyField::ValueType k) noexcept : key{ std::move(k) }
		{}

		const KeyField key;
	};

}
