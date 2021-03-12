#pragma once

#include "../storage/storage_io_interface.hpp"
#include "container/std_container_helpers.hpp"
#include "hash/sha3_hasher.hpp"
#include "utility/extra_type_traits.hpp"

#include <ctype.h>
#include <optional>
#include <string>
#include <typeinfo>

namespace Index {

namespace detail {
	inline std::string normalizedFileName(std::string&& name) {
		for (auto& ch : name)
		{
			if (::isalnum(ch) || ch == ' ' || ch == ',' || ch == '_')
				continue;
			else if (ch == '<')
				ch = '[';
			else if (ch == '>')
				ch = ']';
			else
				ch = '-';
		}

		return name;
	};
}

// On success, returns the full path to the stored file; else - empty optional.
template <typename StorageAdapter, typename IndexType>
std::optional<std::string> store(const IndexType& index, std::string indexStorageFolder) noexcept
{
	const std::string indexFileName = detail::normalizedFileName(std::string{ typeid(index).name() });

	const auto filePath = indexStorageFolder + "/" + indexFileName + ".index";

	StorageIO<StorageAdapter> file;
	assert_and_return_r(file.open(filePath, io::OpenMode::Write), {});

	Sha3_Hasher<256> hasher;

	const auto numIndexEntries = index.size();
	assert_and_return_r(file.write(numIndexEntries), {});

	hasher.update(numIndexEntries);

	for (const auto& indexEntry : index)
	{
		static_assert(is_trivially_serializable_v<decltype(indexEntry.second)>);
		assert_and_return_r(file.write(indexEntry.first), {});
		hasher.update(indexEntry.first);
		assert_and_return_r(file.write(indexEntry.second), {});
		hasher.update(indexEntry.second);
	}

	const uint64_t hash = hasher.get64BitHash();
	assert_and_return_r(file.write(hash), {});

	return filePath;
}

// On success, returns the full path to the stored file; else - empty optional.
template <typename StorageAdapter, typename IndexType>
std::optional<std::string> load(IndexType& index, const std::string indexStorageFolder) noexcept
{
	const std::string indexFileName = detail::normalizedFileName(std::string{ typeid(index).name() });

	const auto filePath = indexStorageFolder + "/" + indexFileName + ".index";

	StorageIO<StorageAdapter> file;
	assert_and_return_r(file.open(filePath, io::OpenMode::Read), {});

	uint64_t numIndexEntries = 0;
	assert_and_return_r(file.read(numIndexEntries), {});

	Sha3_Hasher<256> hasher;
	hasher.update(numIndexEntries);

	for (uint64_t i = 0; i < numIndexEntries; ++i)
	{
		using IndexMultiMapType = std::decay_t<decltype(index)>;
		static_assert(is_trivially_serializable_v<typename IndexMultiMapType::mapped_type>);

		auto field = typename IndexMultiMapType::key_type{};
		auto offset = typename IndexMultiMapType::mapped_type{ 0 };

		assert_and_return_r(file.read(field), {});
		assert_and_return_r(file.read(offset), {});

		hasher.update(field);
		hasher.update(offset);

		const bool emplacementSuccess = index.addLocationForValue(std::move(field), std::move(offset));
		assert_r(emplacementSuccess);
	}

	uint64_t hash = 0;
	assert_and_return_r(file.read(hash), {});
	assert_and_return_r(hasher.get64BitHash() == hash, {});

	assert_r(file.pos() == file.size());

	return filePath;
}

}
