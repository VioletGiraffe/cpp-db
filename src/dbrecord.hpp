#include "dbfield.hpp"
#include "utility/template_magic.hpp"
#include "parameter_pack/parameter_pack_helpers.hpp"

#include <tuple>

template <class... FieldsSequence>
struct DbRecord {
	
private:
	std::tuple<FieldsSequence...> _fields;

	// TODO: uncomment this when MSVC fixes the bug (or implement with concepts)
	//static_assert((FieldsSequence::id != 0xDEADBEEF && ...));
};
