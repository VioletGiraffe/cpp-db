#include "dbutilities.hpp"

#include <stdexcept>

static size_t queryBlockSize()
{
	// TODO: implement!
	return 4096;
}

void checkBlockSize()
{
	if (queryBlockSize() != 4096)
		throw std::runtime_error{"Unexpected cluster size!!!"};
}
