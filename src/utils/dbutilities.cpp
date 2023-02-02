#include "dbutilities.hpp"
#include "assert/advanced_assert.h"

#include <stdexcept>

static size_t queryBlockSize()
{
	// TODO: implement!
	return 4096;
}

void checkBlockSize()
{
	if (queryBlockSize() != 4096)
		fatalAbort("Unexpected cluster size!!!");
}

void fatalAbort(const char* message)
{
	assert_unconditional_r(std::string{"!!! FATAL ERROR !!! "} + message);
	throw std::runtime_error{ message };
}
