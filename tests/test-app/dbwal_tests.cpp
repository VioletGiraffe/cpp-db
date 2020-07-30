#include "3rdparty/catch2/catch.hpp"
#include "dbwal.hpp"
#include "storage/storage_qt.hpp"

TEST_CASE("Operation::Insert serialization", "[dbops]") {
	using F3 = Field<double, 4>;
	using F_ull = Field<uint64_t, 5>;
	using Fs = Field<std::string, 42>;

	using Record = DbRecord<Tombstone<F_ull, std::numeric_limits<uint64_t>::max()>, F3, F_ull, Fs>;

	Operation::Serializer<Record> serializer;

	Operation::Insert<Record> op{ Record{3.14, 15, "Hello World!"} };

	StorageIO<io::QMemoryDeviceAdapter> buffer;
	REQUIRE(buffer.open(".", io::OpenMode::ReadWrite));

	REQUIRE(serializer.serialize(op, buffer));

	REQUIRE(buffer.seek(0));
	const bool success = serializer.deserialize(buffer, [&](auto&& newOp) {
		if constexpr (remove_cv_and_reference_t<decltype(newOp)>::op == OpCode::Insert)
		{
			Record r = newOp._record;
			REQUIRE(r == op._record);
		}
	});
	REQUIRE(success);
}

//TEST_CASE("Operation::Find serialization", "[dbops]") {
//	using F3 = Field<double, 4>;
//	using F_ull = Field<uint64_t, 5>;
//	using Fs = Field<std::string, 42>;

//	using Record = DbRecord<Tombstone<F_ull, std::numeric_limits<uint64_t>::max()>, F3, F_ull, Fs>;

//	Operation::Serializer<Record> serializer;

//	Operation::Find<F3, F_ull, Fs> op{ F3{3.14}, F_ull{15}, Fs{"Hello World!"} };

//	StorageIO<io::QMemoryDeviceAdapter> buffer;
//	REQUIRE(buffer.open(".", io::OpenMode::ReadWrite));

//	REQUIRE(serializer.serialize(op, buffer));

//	REQUIRE(buffer.seek(0));
//	const bool success = serializer.deserialize(buffer, [&](auto&& newOp) {
//		if constexpr (remove_cv_and_reference_t<decltype(newOp)>::op == OpCode::Find)
//		{
//			if constexpr (std::is_same_v<remove_cv_and_reference_t<decltype(op._fields)>, remove_cv_and_reference_t<decltype(newOp._fields)>>)
//			{
//				REQUIRE(newOp._fields == op._fields);
//				return;
//			}
//		}
//		REQUIRE(false);
//	});


//	REQUIRE(success);
//}
