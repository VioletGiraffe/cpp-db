#pragma once

#include "../storage/storage_qt.hpp"
#include "container/std_container_helpers.hpp"

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

	QFile file(QString::fromStdString(filePath));
	assert_and_return_r(file.open(QFile::WriteOnly), false);

	for (const auto& indexEntry : index)
	{
		assert_and_return_r(StorageQt::write(indexEntry.first, file), false);
		assert_and_return_r(file.write(reinterpret_cast<const char*>(&indexEntry.second), sizeof(indexEntry.second)) == sizeof(indexEntry.second), false);
	}

	return true;
}

template <typename IndexType>
bool load(IndexType& index, const std::string indexStorageFolder) noexcept
{
	std::string indexFileName = std::string{ typeid(index).name() };
	std::replace_if(begin_to_end(indexFileName), [](const char ch) { return ch == ':' || ch == '.'; });

	const auto filePath = indexStorageFolder + "/" + indexFileName + ".index";

	QFile file(QString::fromStdString(filePath));
	assert_and_return_r(file.open(QFile::ReadOnly), false);

	while (!file.atEnd())
	{
		using IndexMultiMapType = std::decay_t<decltype(index)>;
		auto field = typename IndexMultiMapType::key_type{};
		auto offset = typename IndexMultiMapType::mapped_type{ 0 };

		assert_and_return_r(StorageQt::read(field, file), false);
		assert_and_return_r(file.read(reinterpret_cast<char*>(&offset), sizeof(offset)) == sizeof(offset), false);

		auto emplacementResult = index.addLocationForValue(std::move(field), std::move(offset));
		assert_r(emplacementResult.second);
	}

	return true;
}

}