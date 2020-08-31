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
		else
			FAIL();
	});

	REQUIRE(success);
	REQUIRE(buffer.atEnd());
}

TEST_CASE("Operation::Find serialization", "[dbops]") {
	using F3 = Field<double, 4>;
	using F_ull = Field<uint64_t, 5>;
	using Fs = Field<std::string, 42>;

	using Record = DbRecord<Tombstone<F_ull, std::numeric_limits<uint64_t>::max()>, F3, F_ull, Fs>;

	Operation::Serializer<Record> serializer;

	Operation::Find<F3, Fs> op{ F3{3.14}, Fs{"Hello World!"} };

	StorageIO<io::QMemoryDeviceAdapter> buffer;
	REQUIRE(buffer.open(".", io::OpenMode::ReadWrite));

	REQUIRE(serializer.serialize(op, buffer));

	REQUIRE(buffer.seek(0));
	bool comparisonPerformed = false;
	const bool success = serializer.deserialize(buffer, [&](auto&& newOp) {
		if constexpr (remove_cv_and_reference_t<decltype(newOp)>::op == OpCode::Find)
		{
			if constexpr (std::is_same_v<remove_cv_and_reference_t<decltype(op._fields)>, remove_cv_and_reference_t<decltype(newOp._fields)>>)
			{
				REQUIRE(newOp._fields == op._fields);
				comparisonPerformed = true;
				return;
			}
			else
				REQUIRE(false);
		}
		REQUIRE(false);
	});


	REQUIRE(success);
	REQUIRE(comparisonPerformed);
	REQUIRE(buffer.atEnd());
}

TEST_CASE("Operation::UpdateFull serialization", "[dbops]") {
	using F3 = Field<double, 4>;
	using F_ull = Field<uint64_t, 5>;
	using Fs = Field<std::string, 42>;

	{
		using Record = DbRecord<Tombstone<F_ull, std::numeric_limits<uint64_t>::max()>, F3, F_ull, Fs>;

		Operation::Serializer<Record> serializer;

		using KeyField = Fs;
		Operation::UpdateFull<Record, KeyField, false> op{ Record{3.14, 15, "Hello World!"}, "123" };

		StorageIO<io::QMemoryDeviceAdapter> buffer;
		REQUIRE(buffer.open(".", io::OpenMode::ReadWrite));

		REQUIRE(serializer.serialize(op, buffer));

		REQUIRE(buffer.seek(0));
		const bool success = serializer.deserialize(buffer, [&](auto&& newOp) {
			using OpType = remove_cv_and_reference_t<decltype(newOp)>;
			if constexpr (OpType::op == OpCode::UpdateFull)
			{
				if constexpr (std::is_same_v<KeyField, typename OpType::KeyField>)
				{
					REQUIRE(newOp.insertIfNotPresent == op.insertIfNotPresent);
					REQUIRE(newOp.record == op.record);
					REQUIRE(newOp.keyValue == op.keyValue);
				}
				else
					FAIL();
			}
			else
				FAIL();
			});

		REQUIRE(success);
		REQUIRE(buffer.atEnd());
	}

	{
		using Record = DbRecord<NoTombstone, F3, Fs, Field<std::string, 45>>;

		Operation::Serializer<Record> serializer;

		using KeyField = F3;
		Operation::UpdateFull<Record, KeyField, true> op{ Record{3.14, "Hello World!", "I am alive!"}, 5.0 };

		StorageIO<io::QMemoryDeviceAdapter> buffer;
		REQUIRE(buffer.open(".", io::OpenMode::ReadWrite));

		REQUIRE(serializer.serialize(op, buffer));

		REQUIRE(buffer.seek(0));
		const bool success = serializer.deserialize(buffer, [&](auto&& newOp) {
			using OpType = remove_cv_and_reference_t<decltype(newOp)>;
			if constexpr (OpType::op == OpCode::UpdateFull)
			{
				if constexpr (std::is_same_v<KeyField, typename OpType::KeyField>)
				{
					REQUIRE(newOp.insertIfNotPresent == op.insertIfNotPresent);
					REQUIRE(newOp.record == op.record);
					REQUIRE(newOp.keyValue == op.keyValue);
				}
				else
					FAIL();
			}
			else
				FAIL();
			});

		REQUIRE(success);
		REQUIRE(buffer.atEnd());
	}
}

TEST_CASE("Operation::Delete serialization", "[dbops]") {
	using F3 = Field<double, 4>;
	using F_ull = Field<uint64_t, 5>;
	using Fs = Field<std::string, 42>;

	using Record = DbRecord<Tombstone<F_ull, std::numeric_limits<uint64_t>::max()>, F3, F_ull, Fs>;

	Operation::Serializer<Record> serializer;

	using KeyField = Fs;
	Operation::Delete<Record, KeyField> op{ "123" };

	StorageIO<io::QMemoryDeviceAdapter> buffer;
	REQUIRE(buffer.open(".", io::OpenMode::ReadWrite));

	REQUIRE(serializer.serialize(op, buffer));

	REQUIRE(buffer.seek(0));
	const bool success = serializer.deserialize(buffer, [&](auto&& newOp) {
		using OpType = remove_cv_and_reference_t<decltype(newOp)>;
		if constexpr (OpType::op == OpCode::Delete)
		{
			if constexpr (std::is_same_v<KeyField, typename OpType::KeyField>)
			{
				REQUIRE(newOp.keyValue == op.keyValue);
			}
			else
				FAIL();
		}
		else
			FAIL();
		});

	REQUIRE(success);
	REQUIRE(buffer.atEnd());
}

template<class A, class B>
bool compareAppendToArrayOps(A&& a, B&& b)
{
	if constexpr (a.insertIfNotPresent() && b.insertIfNotPresent())
		return a.record == b.record;
	else if constexpr (!a.insertIfNotPresent() && !b.insertIfNotPresent())
		return a.array == b.array;
	else
		return false;
}

TEST_CASE("Operation::AppendToArray serialization", "[dbops]") {

	using FKey = Field<double, 4>;
	using FArray = Field<uint64_t, 5, true>;
	using Fc = Field<char, 0>;
	using Fs = Field<std::string, 42>;
	using FSecondArray = Field<std::string, 45, true>;

	using Record = DbRecord<NoTombstone, Fc, FKey, Fs, FArray, FSecondArray>;

	Operation::Serializer<Record> serializer;

	{
		const std::vector<uint64_t> newArray{ 1, 2, 0 };
		Operation::AppendToArray<Record, FKey, FArray, false> op{ 3.14, newArray };

		StorageIO<io::QMemoryDeviceAdapter> buffer;
		REQUIRE(buffer.open(".", io::OpenMode::ReadWrite));

		REQUIRE(serializer.serialize(op, buffer));

		Operation::AppendToArray<Record, FKey, FArray, true> newop{ 3.14, Record{} };

		REQUIRE(buffer.seek(0));
		bool deserializationOccurred = false;
		const bool success = serializer.deserialize(buffer, [&](auto&& newOp) {
			using OpType = remove_cv_and_reference_t<decltype(newOp)>;
			if constexpr (OpType::op == OpCode::AppendToArray)
			{
				REQUIRE(newOp.insertIfNotPresent() == op.insertIfNotPresent());
				if constexpr (std::is_same_v<FKey, typename OpType::KeyField> && std::is_same_v<FArray, typename OpType::ArrayField>)
				{
					deserializationOccurred = true;

					REQUIRE(newOp.keyValue == op.keyValue);
					REQUIRE(compareAppendToArrayOps(newOp, op));
					REQUIRE(newOp.updatedArray() == op.updatedArray());
					REQUIRE(newOp.updatedArray() == newArray);
				}
				else
					FAIL();
			}
			else
				FAIL();
		});

		REQUIRE(success);
		REQUIRE(deserializationOccurred);
		REQUIRE(buffer.atEnd());
	}

	{
		const std::vector<std::string> newArray{ "a", "bc", "def" };
		const Record r{ 'a', -100e35, "Hello!", std::vector<uint64_t>{51, 16}, newArray };
		Operation::AppendToArray<Record, FKey, FSecondArray, true> op{ 3.14, r };

		StorageIO<io::QMemoryDeviceAdapter> buffer;
		REQUIRE(buffer.open(".", io::OpenMode::ReadWrite));

		REQUIRE(serializer.serialize(op, buffer));

		REQUIRE(buffer.seek(0));
		bool deserializationOccurred = false;
		const bool success = serializer.deserialize(buffer, [&](auto&& newOp) {
			using OpType = remove_cv_and_reference_t<decltype(newOp)>;
			if constexpr (OpType::op == OpCode::AppendToArray)
			{
				if constexpr (std::is_same_v<FKey, typename OpType::KeyField> && std::is_same_v<FSecondArray, typename OpType::ArrayField>)
				{
					deserializationOccurred = true;

					REQUIRE(newOp.insertIfNotPresent() == op.insertIfNotPresent());
					REQUIRE(newOp.keyValue == op.keyValue);
					REQUIRE(compareAppendToArrayOps(newOp, op));

					REQUIRE(newOp.updatedArray() == op.updatedArray());
					REQUIRE(newOp.updatedArray() == newArray);
				}
				else
					FAIL();
			}
			else
				FAIL();
			});

		REQUIRE(success);
		REQUIRE(deserializationOccurred);
		REQUIRE(buffer.atEnd());
	}
}
