#pragma once

#include "dbrecord.hpp"

template <class T>
struct DbOperation final : public T
{
	constexpr bool exec() {
		return T::exec();
	}
};

struct OpCreate
{
};

struct OpRead
{
};

struct OpUpdate
{
	bool insertIfNotPresent = false;
};

struct OpDelete
{
};

template <typename Record>
class DbWAL
{
public:


private:
};
