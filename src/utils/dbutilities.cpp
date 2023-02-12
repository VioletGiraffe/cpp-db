#include "dbutilities.hpp"
#include "assert/advanced_assert.h"

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

void fatalAbort(std::string_view message)
{
	std::string msg = "!!! FATAL ERROR !!! ";
	msg.append(message.data(), message.size());

	assert_unconditional_r(msg);
	throw std::exception{ msg.c_str() };
}
