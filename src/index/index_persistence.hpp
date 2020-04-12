#pragma once

#include "../storage/storage_qt.hpp"
#include "container/std_container_helpers.hpp"
#include "hash/sha3_hasher.hpp"

#include <QFile>

#include <algorithm>
#include <string>
#include <type_traits>

namespace Index {

template <typename IndexType>
bool store(const IndexType& index, const std::string indexStorageFolder) noexcept
{
	std::string indexFileName = std::string{ typeid(index).name() };
	std::replace_if(begin_to_end(indexFileName), [](const char ch) { return ch == ':' || ch == '.'; });

	const auto filePath = indexStorageFolder + "/" + indexFileName + ".index";

	QFile file{QString::fromStdString(filePath)};
	assert_and_return_r(file.open(QFile::WriteOnly), false);

	Sha3_Hasher<256> hasher;

	const auto numIndexEntries = index.size();
	assert_and_return_r(StorageQt::write(numIndexEntries, file), false);

	hasher.update(numIndexEntries);

	for (const auto& indexEntry : index)
	{
		static_assert(std::is_trivial_v<std::remove_reference_t<decltype(indexEntry.first)>>);
		static_assert(std::is_trivial_v<std::remove_reference_t<decltype(indexEntry.second)>>);
		assert_and_return_r(StorageQt::writeField(indexEntry.first, file), false);
		hasher.update(indexEntry.first);
		assert_and_return_r(file.write(reinterpret_cast<const char*>(&indexEntry.second), sizeof(indexEntry.second)) == sizeof(indexEntry.second), false);
		hasher.update(indexEntry.second);
	}

	const uint64_t hash = hasher.get64BitHash();
	assert_and_return_r(StorageQt::write(hash, file), false);

	return true;
}

template <typename IndexType>
bool load(IndexType& index, const std::string indexStorageFolder) noexcept
{
	std::string indexFileName = std::string{ typeid(index).name() };
	std::replace_if(begin_to_end(indexFileName), [](const char ch) { return ch == ':' || ch == '.'; });

	const auto filePath = indexStorageFolder + "/" + indexFileName + ".index";

	QFile file{QString::fromStdString(filePath)};
	assert_and_return_r(file.open(QFile::ReadOnly), false);

	uint64_t numIndexEntries = 0;
	assert_and_return_r(StorageQt::read(numIndexEntries, file), false);

	Sha3_Hasher<256> hasher;
	hasher.update(numIndexEntries);

	for (uint64_t i = 0; i < numIndexEntries; ++i)
	{
		using IndexMultiMapType = std::decay_t<decltype(index)>;
		auto field = typename IndexMultiMapType::key_type{};
		auto offset = typename IndexMultiMapType::mapped_type{ 0 };

		assert_and_return_r(StorageQt::readField(field, file), false);
		assert_and_return_r(file.read(reinterpret_cast<char*>(&offset), sizeof(offset)) == sizeof(offset), false);

		hasher.update(field);
		hasher.update(offset);

		auto emplacementResult = index.addLocationForValue(std::move(field), std::move(offset));
		assert_r(emplacementResult.second);
	}

	uint64_t hash = 0;
	assert_and_return_r(StorageQt::read(hash, file), false);
	assert_and_return_r(hasher.get64BitHash() == hash, false);

	assert_r(file.atEnd());

	return true;
}

}
