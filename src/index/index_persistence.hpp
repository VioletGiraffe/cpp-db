#pragma once

#include "../storage/storage_io_interface.hpp"
#include "../utils/dbutilities.hpp"

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

	StorageAdapter file;
	StorageIO io{ file };
	assert_and_return_r(io.open(filePath, io::OpenMode::Write), {});

	Sha3_Hasher<256> hasher;

	const auto numIndexEntries = index.size();
	assert_and_return_r(io.write(numIndexEntries), {});

	hasher.update(numIndexEntries);

	for (const auto& indexEntry : index)
	{
		static_assert(is_trivially_serializable_v<decltype(indexEntry.second)>);
		assert_and_return_r(io.write(indexEntry.first), {});
		hasher.update(indexEntry.first);
		assert_and_return_r(io.write(indexEntry.second), {});
		hasher.update(indexEntry.second);
	}

	const uint64_t hash = hasher.get64BitHash();
	assert_and_return_r(io.write(hash), {});

	return filePath;
}

// On success, returns the full path to the stored file; else - empty optional.
template <typename StorageAdapter, typename IndexType>
std::optional<std::string> load(IndexType& index, const std::string indexStorageFolder) noexcept
{
	const std::string indexFileName = detail::normalizedFileName(std::string{ typeid(index).name() });

	const auto filePath = indexStorageFolder + "/" + indexFileName + ".index";

	StorageAdapter file;
	StorageIO io{ file };

	assert_and_return_r(io.open(filePath, io::OpenMode::Read), {});

	uint64_t numIndexEntries = 0;
	assert_and_return_r(io.read(numIndexEntries), {});

	Sha3_Hasher<256> hasher;
	hasher.update(numIndexEntries);

	for (uint64_t i = 0; i < numIndexEntries; ++i)
	{
		static_assert(is_trivially_serializable_v< PageNumber>);

		auto field = typename IndexType::key_type{};
		auto location = PageNumber{};

		assert_and_return_r(io.read(field), {});
		assert_and_return_r(io.read(location), {});

		hasher.update(field);
		hasher.update(location);

		const bool emplacementSuccess = index.addLocationForKey(std::move(field), std::move(location));
		if (!emplacementSuccess)
			fatalAbort("Failed to add key to index!");
	}

	uint64_t hash = 0;
	assert_and_return_r(io.read(hash), {});
	assert_and_return_r(hasher.get64BitHash() == hash, {});

	assert_r(io.pos() == io.size());

	return filePath;
}

}
