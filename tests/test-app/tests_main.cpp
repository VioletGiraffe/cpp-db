#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#define CATCH_CONFIG_ENABLE_BENCHMARKING

#include "cpp-db_sanity_checks.hpp"
#include "dbfilegaps_test.hpp"
#include "dbfilegaps_operations_test.hpp"
#include "dbfilegaps_load-store_test.hpp"
#include "dbindex_test.hpp"
#include "dbindices_test.hpp"
#include "dbrecord_tests.hpp"
#include "dbstorage_tests.hpp"


//#include "dbfilegaps_benchmarks.hpp"





























//
//#include "system/ctimeelapsed.h"
//
//#include <iostream>
//#include <map>
//
//int main()
//{
//	std::multimap<uint64_t, void*> m;
//	for (size_t i = 0; i < 200000; ++i)
//		m.emplace(i & 31, (void*)i);
//
//	const auto range = m.equal_range(1);
//	auto secondaryIt = range.first;
//
//	CTimeElapsed timer(true);
//	for (int i = 0; i < 100 * 10; ++i)
//	{
//		secondaryIt = range.first;
//		for (; secondaryIt != range.second;)
//		{
//			const auto current = secondaryIt++;
//			if (current->second == 0)
//				continue;
//		}
//	}
//
//	timer.pause();
//
//	std::cout << "equal_range iteration took " << timer.elapsed() << " ms\n";
//
//	secondaryIt = range.first;
//
//	timer.start();
//	for (int i = 0; i < 100 * 10; ++i)
//	{
//		secondaryIt = m.begin();
//		for (const auto end = m.end(); secondaryIt != end;)
//		{
//			const auto current = secondaryIt++;
//			if (current->second == 0)
//				continue;
//		}
//	}
//	timer.pause();
//
//	std::cout << "Complete multimap iteration took " << timer.elapsed() << " ms\n";
//
//	return secondaryIt != m.end();
//}
