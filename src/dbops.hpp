#pragma once

#include "hash/jenkins_hash.hpp"

namespace Operation {

	template <class Record>
	struct Insert
	{
		static constexpr auto op = jenkins_hash("Insert");

		explicit Insert(Record r) noexcept : _record{ std::move(r) } {}
		const auto& record() const & noexcept
		{
			return _record;
		}

	private:
		const Record _record;

		static_assert(Record::isRecord());
	};

	template <typename... Fields>
	struct Find
	{
		static constexpr auto op = jenkins_hash("Find");
		template <typename... V>
		explicit Find(V&&... values) noexcept : _fields(std::forward<V>(values)...) {}

	private:
		const std::tuple<Fields...> _fields;
	};

	template <class Record>
	struct Update
	{
		static constexpr auto op = jenkins_hash("Update");

		bool insertIfNotPresent = false;
	};

	template <class Record>
	struct Delete
	{
		static constexpr auto op = jenkins_hash("Delete");
	};

}