#include <fuser/fuser.hpp>

#include <boost/test/unit_test.hpp>

#include <array>
#include <vector>
#include <deque>
#include <map>
#include <unordered_map>
#include <initializer_list>
#include <limits>
#include <string>
#include <memory>
#include <string_view>

#if __cplusplus >= 201703L && !defined(FUSER_ONLY_CXX_11)
#include <optional>
#endif

namespace ut = boost::unit_test;
namespace tt = boost::test_tools;

struct integer_struct
{
	std::uint8_t u8;
	std::uint64_t u64;
	std::int8_t i8;
	std::int64_t i64;
};
BOOST_FUSION_ADAPT_STRUCT(integer_struct, u8, u64, i8, i64)

bool operator==(integer_struct lhs, integer_struct rhs)
{
	return lhs.u8 == rhs.u8 && lhs.u64 == rhs.u64
		&& lhs.i8 == rhs.i8 && lhs.i64 == rhs.i64;
}

bool operator!=(integer_struct lhs, integer_struct rhs)
{
	return !(lhs == rhs);
}

enum class sample_enum : std::size_t
{
	unknown, foo, bar
};

template <>
struct fuser::serializer<sample_enum>
{
	static nlohmann::json serialize(sample_enum e)
	{
		switch (e) {
			case sample_enum::foo:
				return "foo";
			case sample_enum::bar:
				return "bar";
			default:
				return "(unknown)";
		}
	}
};

template <>
struct fuser::deserializer<sample_enum>
{
	static sample_enum deserialize(nlohmann::json const& json)
	{
		auto const& str = json.get_ref<nlohmann::json::string_t const&>();

		if (str == "foo")
			return sample_enum::foo;
		else if (str == "bar")
			return sample_enum::bar;
		else
			return sample_enum::unknown;
	}
};

struct multi_type_struct
{
	bool b;
	std::string str;
	std::vector<sample_enum> vec;
	std::deque<integer_struct> deq;
};
BOOST_FUSION_ADAPT_STRUCT(multi_type_struct, b, str, vec, deq)

bool operator==(multi_type_struct const& lhs, multi_type_struct const& rhs)
{
	return lhs.b == rhs.b && lhs.str == rhs.str
		&& lhs.vec == rhs.vec && lhs.deq == rhs.deq;
}

bool operator!=(multi_type_struct const& lhs, multi_type_struct const& rhs)
{
	return !(lhs == rhs);
}

struct smart_pointer_struct
{
	std::unique_ptr<int> i;
	std::unique_ptr<sample_enum> e;
};
BOOST_FUSION_ADAPT_STRUCT(smart_pointer_struct, i, e)

template <typename T>
bool compare_unique_ptrs(std::unique_ptr<T> const& lhs, std::unique_ptr<T> const& rhs)
{
	/*
	 * operator== in standard library compares stored pointers only
	 * we want to compare stored values if available instead
	 */
	if (lhs == nullptr && rhs == nullptr)
		return true;
	if (lhs == nullptr && rhs != nullptr)
		return false;
	if (lhs != nullptr && rhs == nullptr)
		return false;

	return *lhs == *rhs;
}

bool operator==(smart_pointer_struct const& lhs, smart_pointer_struct const& rhs)
{
	return compare_unique_ptrs(lhs.i, rhs.i) && compare_unique_ptrs(lhs.e, rhs.e);
}

bool operator!=(smart_pointer_struct const& lhs, smart_pointer_struct const& rhs)
{
	return !(lhs == rhs);
}

template <typename T, typename... Args>
std::unique_ptr<T> make_unique(Args&&... args)
{
	return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

namespace std
{
	template <>
	struct less<sample_enum>
	{
		constexpr bool operator()(sample_enum lhs, sample_enum rhs) const
		{
			return static_cast<std::size_t>(lhs) < static_cast<std::size_t>(rhs);
		}
	};
}

struct struct_with_map
{
	std::map<sample_enum, int> m;
};
BOOST_FUSION_ADAPT_STRUCT(struct_with_map, m)

bool operator==(struct_with_map const& lhs, struct_with_map const& rhs)
{
	return lhs.m == rhs.m;
}

bool operator!=(struct_with_map const& lhs, struct_with_map const& rhs)
{
	return !(lhs == rhs);
}

namespace std
{
	template <>
	struct hash<sample_enum>
	{
		std::size_t operator()(sample_enum e) const noexcept
		{
			return static_cast<std::size_t>(e);
		}
	};
}

struct struct_with_unordered_map
{
	std::unordered_map<sample_enum, int> m;
};
BOOST_FUSION_ADAPT_STRUCT(struct_with_unordered_map, m)

bool operator==(struct_with_unordered_map const& lhs, struct_with_unordered_map const& rhs)
{
	return lhs.m == rhs.m;
}

bool operator!=(struct_with_unordered_map const& lhs, struct_with_unordered_map const& rhs)
{
	return !(lhs == rhs);
}

namespace {

template <typename T>
void bidirectional_test(nlohmann::json const& json, T const& val)
{
	auto serialized = fuser::serialize(val);
	auto deserialized = fuser::deserialize<T>(json);

	if (serialized != json) {
		BOOST_ERROR("JSONs are different (expected vs actual):\n"
			<< fuser::dump_json(json) << "\n\n" << fuser::dump_json(serialized));
	}

	BOOST_TEST((deserialized == val));
}

}

BOOST_AUTO_TEST_SUITE(fuser_suite)

	BOOST_AUTO_TEST_CASE(booleans)
	{
		bidirectional_test(true, true);
		bidirectional_test(false, false);
	}

	BOOST_AUTO_TEST_CASE(integers)
	{
		constexpr auto u8_min = std::numeric_limits<std::uint8_t>::min();
		constexpr auto u8_max = std::numeric_limits<std::uint8_t>::max();
		bidirectional_test(u8_min, u8_min);
		bidirectional_test(u8_max, u8_max);

		constexpr auto u16_min = std::numeric_limits<std::uint16_t>::min();
		constexpr auto u16_max = std::numeric_limits<std::uint16_t>::max();
		bidirectional_test(u16_min, u16_min);
		bidirectional_test(u16_max, u16_max);

		constexpr auto u32_min = std::numeric_limits<std::uint32_t>::min();
		constexpr auto u32_max = std::numeric_limits<std::uint32_t>::max();
		bidirectional_test(u32_min, u32_min);
		bidirectional_test(u32_max, u32_max);

		constexpr auto u64_min = std::numeric_limits<std::uint64_t>::min();
		constexpr auto u64_max = std::numeric_limits<std::uint64_t>::max();
		bidirectional_test(u64_min, u64_min);
		bidirectional_test(u64_max, u64_max);

		constexpr auto i8_min = std::numeric_limits<std::int8_t>::min();
		constexpr auto i8_max = std::numeric_limits<std::int8_t>::max();
		bidirectional_test(i8_min, i8_min);
		bidirectional_test(i8_max, i8_max);

		constexpr auto i16_min = std::numeric_limits<std::int16_t>::min();
		constexpr auto i16_max = std::numeric_limits<std::int16_t>::max();
		bidirectional_test(i16_min, i16_min);
		bidirectional_test(i16_max, i16_max);

		constexpr auto i32_min = std::numeric_limits<std::int32_t>::min();
		constexpr auto i32_max = std::numeric_limits<std::int32_t>::max();
		bidirectional_test(i32_min, i32_min);
		bidirectional_test(i32_max, i32_max);

		constexpr auto i64_min = std::numeric_limits<std::int64_t>::min();
		constexpr auto i64_max = std::numeric_limits<std::int64_t>::max();
		bidirectional_test(i64_min, i64_min);
		bidirectional_test(i64_max, i64_max);
	}

	BOOST_AUTO_TEST_CASE(strings)
	{
		bidirectional_test("", std::string());
		bidirectional_test("foo", std::string("foo"));
		bidirectional_test("bar", std::string("bar"));
	}

	BOOST_AUTO_TEST_CASE(pointers)
	{
		auto const ptr = reinterpret_cast<void*>(0x42);
		bidirectional_test(0x42, ptr);
	}

	BOOST_AUTO_TEST_CASE(containers)
	{
		auto const values = { 1, 2, 3, 123 };
		bidirectional_test(values, std::vector<int>(values));
		bidirectional_test(values, std::deque<int>(values));
		bidirectional_test(values, std::array<int, 4>{{ 1, 2, 3, 123 }});
	}

	BOOST_AUTO_TEST_CASE(specialized_enum_array)
	{
		std::array<sample_enum, 3> const values = {{ sample_enum::foo, sample_enum::bar, sample_enum::unknown }};
		nlohmann::json const json = nlohmann::json::array_t{"foo", "bar", "(unknown)"};
		bidirectional_test(json, values);
	}

	BOOST_AUTO_TEST_CASE(adapted_integer_struct)
	{
		nlohmann::json const json = {
			{"u8", 255}, {"u64", 1}, {"i8", -128}, {"i64", -1}
		};
		integer_struct s = { 255u, 1ull, -128, -1ll };
		bidirectional_test(json, s);
	}

	BOOST_AUTO_TEST_CASE(multi_type_struct_array)
	{
		nlohmann::json const json = {
			{
				{"b", true},
				{"str", "abc"},
				{"vec", {"foo", "bar", "(unknown)"}},
				{"deq", {
					{{"u8", 255}, {"u64", 1}, {"i8", -128}, {"i64", -1}},
					{{"u8", 2}, {"u64", 1}, {"i8", -1}, {"i64", -2}},
					{{"u8", 0}, {"u64", 0}, {"i8", 0}, {"i64", 0}},
				}},
			},
			{
				{"b", false},
				{"str", "xyz"},
				{"vec", {"bar", "(unknown)", "foo"}},
				{"deq", {
					{{"u8", 3}, {"u64", 1}, {"i8", -4}, {"i64", -15}},
					{{"u8", 1}, {"u64", 3}, {"i8", -15}, {"i64", -4}},
					{{"u8", 0}, {"u64", 0}, {"i8", 0}, {"i64", 0}},
					{{"u8", 1}, {"u64", 255}, {"i8", -1}, {"i64", -128}},
				}},
			}
		};

		std::array<multi_type_struct, 2> const values = {
			multi_type_struct{
				true,
				"abc",
				{ sample_enum::foo, sample_enum::bar, sample_enum::unknown },
				{
					{ 255u, 1ull, -128, -1ll },
					{ 2u, 1ull, -1, -2ll },
					{ 0u, 0ull, 0, 0ll }
				}
			},
			multi_type_struct{
				false,
				"xyz",
				{ sample_enum::bar, sample_enum::unknown, sample_enum::foo },
				{
					{ 3u, 1ull, -4, -15ll },
					{ 1u, 3ull, -15, -4ll },
					{ 0u, 0ull, 0, 0ll },
					{ 1, 255ull, -1, -128ll }
				}
			}
		};

		bidirectional_test(json, values);
	}

	BOOST_AUTO_TEST_CASE(smart_pointer_struct_array)
	{
		nlohmann::json const json = {
			{
				{"i", nullptr}, {"e", "foo"}
			},
			{
				{"i", 1}, {"e", "bar"}
			},
			{
				{"i", -1}, {"e", nullptr}
			}
		};

		// can not construct vector from init lists because
		// they copy and unique_ptr is not copyable
		// use push_back/emplace_back instead
		std::vector<smart_pointer_struct> values;
		values.push_back({nullptr, make_unique<sample_enum>(sample_enum::foo)});
		values.push_back({make_unique<int>(1), make_unique<sample_enum>(sample_enum::bar)});
		values.push_back({make_unique<int>(-1), nullptr});

		bidirectional_test(json, values);
	}

#if __cplusplus >= 201703L && !defined(FUSER_ONLY_CXX_11)
	BOOST_AUTO_TEST_CASE(optional)
	{
		nlohmann::json const json = { 0, nullptr, 1, 2, 3, nullptr };
		std::deque<std::optional<int>> const values = { {0}, {std::nullopt}, {1}, {2}, {3}, {std::nullopt} };
		bidirectional_test(json, values);
	}
#endif

	BOOST_AUTO_TEST_CASE(map)
	{
		nlohmann::json const json = {
			{"m", {
				{"foo", 123},
				{"bar", 456}
			}}
		};
		struct_with_map const s = {
			{{sample_enum::foo, 123}, {sample_enum::bar, 456}}
		};
		bidirectional_test(json, s);
	}

	BOOST_AUTO_TEST_CASE(unordered_map)
	{
		nlohmann::json const json = {
			{"m", {
				{"foo", 123},
				{"bar", 456}
			}}
		};
		struct_with_unordered_map const s = {
			{{sample_enum::foo, 123}, {sample_enum::bar, 456}}
		};
		bidirectional_test(json, s);
	}

BOOST_AUTO_TEST_SUITE_END()
