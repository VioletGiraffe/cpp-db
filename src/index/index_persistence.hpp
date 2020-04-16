#pragma once

#include "../storage/storage_qt.hpp"
#include "container/std_container_helpers.hpp"
#include "hash/sha3_hasher.hpp"
#include "utility/extra_type_traits.hpp"

#include <QFile>

#include <ctype.h>
#include <optional>
#include <string>

namespace Index {

namespace detail {
	std::string normalizedFileName(std::string name) {
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
template <typename IndexType>
std::optional<std::string> store(const IndexType& index, const std::string indexStorageFolder) noexcept
{
	const std::string indexFileName = detail::normalizedFileName(std::string{ typeid(index).name() });

	const auto filePath = indexStorageFolder + "/" + indexFileName + ".index";

	QFile file{QString::fromStdString(filePath)};
	assert_and_return_r(file.open(QFile::WriteOnly), {});

	Sha3_Hasher<256> hasher;

	const auto numIndexEntries = index.size();
	assert_and_return_r(StorageQt::write(numIndexEntries, file), {});

	hasher.update(numIndexEntries);

	for (const auto& indexEntry : index)
	{
		static_assert(is_trivially_serializable_v<decltype(indexEntry.second)>);
		assert_and_return_r(StorageQt::write(indexEntry.first, file), {});
		hasher.update(indexEntry.first);
		assert_and_return_r(file.write(reinterpret_cast<const char*>(&indexEntry.second), sizeof(indexEntry.second)) == sizeof(indexEntry.second), {});
		hasher.update(indexEntry.second);
	}

	const uint64_t hash = hasher.get64BitHash();
	assert_and_return_r(StorageQt::write(hash, file), {});

	return filePath;
}

// On success, returns the full path to the stored file; else - empty optional.
template <typename IndexType>
std::optional<std::string> load(IndexType& index, const std::string indexStorageFolder) noexcept
{
	const std::string indexFileName = detail::normalizedFileName(std::string{ typeid(index).name() });

	const auto filePath = indexStorageFolder + "/" + indexFileName + ".index";

	QFile file{QString::fromStdString(filePath)};
	assert_and_return_r(file.open(QFile::ReadOnly), {});

	uint64_t numIndexEntries = 0;
	assert_and_return_r(StorageQt::read(numIndexEntries, file), {});

	Sha3_Hasher<256> hasher;
	hasher.update(numIndexEntries);

	for (uint64_t i = 0; i < numIndexEntries; ++i)
	{
		using IndexMultiMapType = std::decay_t<decltype(index)>;
		static_assert(is_trivially_serializable_v<IndexMultiMapType::mapped_type>);

		auto field = typename IndexMultiMapType::key_type{};
		auto offset = typename IndexMultiMapType::mapped_type{ 0 };

		assert_and_return_r(StorageQt::read(field, file), {});
		assert_and_return_r(file.read(reinterpret_cast<char*>(&offset), sizeof(offset)) == sizeof(offset), {});

		hasher.update(field);
		hasher.update(offset);

		const bool emplacementSiccess = index.addLocationForValue(std::move(field), std::move(offset));
		assert_r(emplacementSiccess);
	}

	uint64_t hash = 0;
	assert_and_return_r(StorageQt::read(hash, file), {});
	assert_and_return_r(hasher.get64BitHash() == hash, {});

	assert_r(file.atEnd());

	return filePath;
}

}
