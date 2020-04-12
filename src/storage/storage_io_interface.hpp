#pragma once

#include "storage_helpers.hpp"
#include "../dbfield.hpp"
#include "assert/advanced_assert.h"

#include <optional>
#include <string>

template <typename IODevice>
struct StorageIO
{
	template <typename T, auto id>
	static bool writeField(const Field<T, id>& field, IODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;

	template <auto id>
	static bool writeField(const Field<std::string, id>& field, IODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;

	template <typename T, auto id>
	static bool readField(Field<T, id>& field, IODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;

	template <auto id>
	static bool readField(Field<std::string, id>& field, IODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;

	template <typename T>
	static bool read(T& value, IODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;

	template <typename T>
	static bool write(T&& value, IODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;
};
