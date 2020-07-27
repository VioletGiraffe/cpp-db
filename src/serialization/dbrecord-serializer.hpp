#pragma once

#include "../storage/storage_io_interface.hpp"
#include "../dbrecord.hpp"

#include "assert/advanced_assert.h"
#include "utility/constexpr_algorithms.hpp"
#include "utility/template_magic.hpp"

#include <string.h> // memcpy

template <typename T>
struct RecordSerializer {
	static_assert(false_v<T>, "This shouldn't be instantiated - check the template parameter list for errors!");
};

template <typename... Args>
struct RecordSerializer<DbRecord<Args...>>
{
	using Record = DbRecord<Args...>;

	template <typename StorageImplementation>
	[[nodiscard]] static bool serialize(const Record& record, StorageIO<StorageImplementation>& io) noexcept
	{
		static_assert (Record::isRecord());

		// Statically sized fields are grouped before others and can be read or written in a single block.
		static constexpr size_t staticFieldsSize = record.staticFieldsSize();
		std::array<uint8_t, staticFieldsSize> buffer;

		size_t bufferOffset = 0;
		static_for<0, record.staticFieldsCount()>([&](auto i) {
			using FieldType = typename Record::template FieldTypeByIndex_t<i>;

			const auto& field = record.template fieldAtIndex<i>();
			static_assert(std::is_same_v<std::remove_cv_t<FieldType>, remove_cv_and_reference_t<decltype(field)>>);
			static_assert(is_trivially_serializable_v<typename FieldType::ValueType>);
			static_assert(FieldType::sizeKnownAtCompileTime());

			::memcpy(buffer.data() + bufferOffset, std::addressof(field.value), FieldType::staticSize());
			bufferOffset += FieldType::staticSize();
			});

		assert_r(bufferOffset == buffer.size());
		assert_and_return_r(io.write(buffer.data(), staticFieldsSize), false);

		bool success = true;
		static_for<record.staticFieldsCount(), record.fieldCount()>([&](auto i) {
			if (!success)
				return;

			using FieldType = typename Record::template FieldTypeByIndex_t<i>;

			const auto& field = record.template fieldAtIndex<i>();
			static_assert(std::is_same_v<std::remove_cv_t<FieldType>, remove_cv_and_reference_t<decltype(field)>>);
			static_assert(!FieldType::sizeKnownAtCompileTime());

			if (!io.writeField(field))
			{
				assert_unconditional_r("Failed to write data!");
				success = false;
			}
			});

		return success;
	}

	template <typename StorageImplementation>
	[[nodiscard]] static bool deserialize(Record& record, StorageIO<StorageImplementation>& io) noexcept
	{
		// Statically sized fields are grouped before others and can be read or written in a single block.
		static constexpr size_t staticFieldsSize = record.staticFieldsSize();
		std::array<uint8_t, staticFieldsSize> buffer;

		assert_and_return_r(io.read(buffer.data(), staticFieldsSize), false);

		size_t bufferOffset = 0;
		bool success = true;
		static_for<0, Record::fieldCount()>([&](auto i) {
			using FieldType = typename Record::template FieldTypeByIndex_t<i>;

			auto& field = record.template fieldAtIndex<i>();
			static_assert(std::is_same_v<std::remove_cv_t<FieldType>, remove_cv_and_reference_t<decltype(field)>>);

			if constexpr (FieldType::sizeKnownAtCompileTime())
			{
				static_assert(is_trivially_serializable_v<typename FieldType::ValueType>);
				::memcpy(std::addressof(field.value), buffer.data() + bufferOffset, FieldType::staticSize());
				bufferOffset += FieldType::staticSize();
			}
			else
			{
				if (!success)
					return;

				if (!io.readField(field))
				{
					assert_unconditional_r("Failed to read data!");
					success = false;
				}
			}
			});

		return success;
	}
};
