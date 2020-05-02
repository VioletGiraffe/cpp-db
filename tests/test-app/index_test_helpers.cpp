#include "index/dbindex.hpp"
#include "random/randomnumbergenerator.h"
#include "container/std_container_helpers.hpp"

template <typename T1, typename U1, typename T2, typename U2>
inline bool operator==(const std::pair<T1, U1>& p1, const std::pair<T2, U2>& p2)
{
	return p1.first == p2.first && p1.second == p2.second;
}

template <typename Field>
inline auto fillIndexRandomly(DbIndex<Field>& index, const uint64_t nItems)
{
	using IndexEntryType = std::pair<typename DbIndex<Field>::FieldValueType, StorageLocation>;
	std::vector<IndexEntryType> items;

	RandomNumberGenerator<int64_t> rng(0, -static_cast<int64_t>(nItems / 5), static_cast<int64_t>(nItems / 5));
	for (uint64_t i = 0; i < nItems; ++i)
	{
		if constexpr (std::is_same_v<std::string, typename Field::ValueType>)
			items.emplace_back(std::to_string(rng.rand()), i);
		else
			items.emplace_back(rng.rand(), i);
	}

	std::mt19937 randomizer;
	randomizer.seed(0);
	std::shuffle(begin_to_end(items), randomizer);

	for (const auto& item : items)
		index.addLocationForValue(item.first, item.second);

	const auto indexSorter = [](const IndexEntryType& l, const IndexEntryType& r) { return l.first < r.first; };
	std::stable_sort(begin_to_end(items), indexSorter);

	return items;
}

template <typename Index, typename ReferenceVector>
bool verifyIndexContents(const Index& index, const ReferenceVector& reference)
{
	return std::equal(cbegin_to_end(index), cbegin_to_end(reference));
}
