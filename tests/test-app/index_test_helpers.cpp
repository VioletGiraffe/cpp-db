#include "index/dbindex.hpp"
#include "random/randomnumbergenerator.h"
#include "container/std_container_helpers.hpp"

#include <set>

template <typename T1, typename U1, typename T2, typename U2>
inline bool operator==(const std::pair<T1, U1>& p1, const std::pair<T2, U2>& p2)
{
	return p1.first == p2.first && p1.second == p2.second;
}

template <typename Field>
inline auto fillIndexRandomly(DbIndex<Field>& index, const uint64_t nItems)
{
	using KeyType = typename DbIndex<Field>::key_type;
	using IndexEntryType = std::pair<KeyType, PageNumber>;
	using Comparator = decltype([](const IndexEntryType& l, const IndexEntryType& r) { return l.first < r.first; });
	std::set<IndexEntryType, Comparator> itemsSet;

	RandomNumberGenerator<uint64_t> rng(0, 0, nItems * 1000);
	// Only unique keys allowed!
	for (uint64_t i = 0; itemsSet.size() < nItems; ++i)
	{
		if constexpr (std::is_same_v<std::string, typename Field::ValueType>)
			itemsSet.emplace(std::to_string(rng.rand()), i);
		else
			itemsSet.emplace(static_cast<KeyType>(rng.rand()), i);
	}

	std::vector<IndexEntryType> items;
	items.reserve(nItems);
	for (auto& item : itemsSet)
		items.push_back(std::move(item));

	std::mt19937 randomizer;
	randomizer.seed(0);
	std::shuffle(begin_to_end(items), randomizer);

	for (const auto& item : items)
	{
		if (!index.addLocationForKey(item.first, item.second))
		{
			assert_debug_only(false);
		}
	}

	const auto indexSorter = [](const IndexEntryType& l, const IndexEntryType& r) { return l.first < r.first; };
	std::sort(begin_to_end(items), indexSorter);

	return items;
}

template <typename Index, typename ReferenceVector>
bool verifyIndexContents(const Index& index, const ReferenceVector& reference)
{
	return std::equal(cbegin_to_end(index), cbegin_to_end(reference));
}
