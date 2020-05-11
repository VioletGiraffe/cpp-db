#pragma once

enum class Operation {
	Insert,
	Find,
	Update,
	Delete
};

template <auto Op>
struct DbOperation
{
protected:

};

struct OpInsert final : public DbOperation<Operation::Insert>
{
};

struct OpFind final : public DbOperation<Operation::Find>
{
};

struct OpUpdate final : public DbOperation<Operation::Update>
{
	bool insertIfNotPresent = false;
};

struct OpDelete final : public DbOperation<Operation::Delete>
{
};