#pragma once

#include <optional>
#include <tuple>
#include <vector>

#define DEFINE_FIELD(Type, name)

enum class TestCollectionFields {
	Id,
	Text
};

template <typename T, auto fieldIdValue>
struct Field {
	T value;
	static constexpr auto id = fieldIdValue;
};

template <class... Fields>
class Collection
{
public:
	using Record = std::tuple<Fields...>;

	template <auto N>
	using FieldTypeById = typename std::tuple_element<static_cast<size_t>(N), Record>::type;

	template <auto fieldTypeEnumValue>
	void write(const FieldTypeById<fieldTypeEnumValue>& value) {

	}

	template <auto fieldTypeEnumValue>
	std::vector<Record> find(const FieldTypeById<fieldTypeEnumValue>& queryValue) {
		return {};
	}
};
