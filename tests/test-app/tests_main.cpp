#define CATCH_CONFIG_ENABLE_BENCHMARKING
#define CATCH_CONFIG_RUNNER
#include "3rdparty/catch2/catch.hpp"

#include "assert/advanced_assert.h"

#include <iostream>

#if defined(CATCH_CONFIG_WCHAR) && defined(CATCH_PLATFORM_WINDOWS) && defined(_UNICODE)
// Standard C/C++ Win32 Unicode wmain entry point
extern "C" int wmain(int argc, wchar_t* argv[], wchar_t* []) {
#else
// Standard C/C++ main entry point
int main(int argc, char* argv[]) {
#endif

	AdvancedAssert::setLoggingFunc([](const char* msg) {
		std::cout << msg << std::endl;
	});

	return Catch::Session().run(argc, argv);
}