#pragma once

enum class Operation {
	Insert,
	Find,
	Update,
	Delete
};

struct OpInsert
{
	static constexpr auto op = Operation::Insert;

	template <class Wal>
	bool commitToWal(Wal&);

	template <class Index>
	bool commitToIndex(Index& index);

	template <class Storage>
	bool commitToStorage(Storage& index);
};

struct OpFind
{
	static constexpr auto op = Operation::Find;

	template <class Wal>
	bool commitToWal(Wal&);

	template <class Index>
	bool commitToIndex(Index& index);

	template <class Storage>
	bool commitToStorage(Storage& index);
};

struct OpUpdate
{
	static constexpr auto op = Operation::Update;

	bool insertIfNotPresent = false;

	template <class Wal>
	bool commitToWal(Wal&);

	template <class Index>
	bool commitToIndex(Index& index);

	template <class Storage>
	bool commitToStorage(Storage& index);
};

struct OpDelete
{
	static constexpr auto op = Operation::Delete;

	template <class Wal>
	bool commitToWal(Wal&);

	template <class Index>
	bool commitToIndex(Index& index);

	template <class Storage>
	bool commitToStorage(Storage& index);
};