#pragma once

namespace Operation {

	template <class Record>
	struct Insert
	{
		static constexpr auto op = __LINE__;
	};

	template <class Record>
	struct Find
	{
		static constexpr auto op = __LINE__;
	};

	template <class Record>
	struct Update
	{
		static constexpr auto op = __LINE__;

		bool insertIfNotPresent = false;
	};

	template <class Record>
	struct Delete
	{
		static constexpr auto op = __LINE__;
	};

}