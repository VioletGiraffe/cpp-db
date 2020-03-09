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
	static bool write(const Field<T, id>& field, IODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;

	template <auto id>
	static bool write(const Field<std::string, id>& field, IODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;

	template <typename T, auto id>
	static bool read(Field<T, id>& field, IODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;

	template <auto id>
	static bool read(Field<std::string, id>& field, IODevice& storageDevice, const std::optional<uint64_t> offset = {}) noexcept;
};
