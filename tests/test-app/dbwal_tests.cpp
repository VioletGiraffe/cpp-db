#include "3rdparty/catch2/catch.hpp"
#include "dbwal.hpp"
#include "storage/storage_qt.hpp"

TEST_CASE("DbOps - basic serialization", "[dbops]") {
	using F3 = Field<double, 4>;
	using F_ull = Field<uint64_t, 5>;
	using Fs = Field<std::string, 42>;

	using Record = DbRecord<Tombstone<F_ull, std::numeric_limits<uint64_t>::max()>, F3, F_ull, Fs>;

	Operation::Insert<Record> op{ Record{3.14, 15, "Hello World!"} };
	Operation::Serializer<Record> serializer;

	StorageIO<io::QMemoryDeviceAdapter> buffer;
	REQUIRE(buffer.open(".", io::OpenMode::ReadWrite));

	REQUIRE(serializer.serialize(op, buffer));

	REQUIRE(buffer.seek(0));
	REQUIRE(serializer.deserialize(buffer, [&](auto&& newOp) {
		REQUIRE(newOp._record == op._record);
	}));
}
