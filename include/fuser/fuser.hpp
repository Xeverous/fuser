#pragma once

#include <nlohmann/json.hpp>

#include <boost/mpl/range_c.hpp>
#include <boost/fusion/include/for_each.hpp>
#include <boost/fusion/include/adapt_struct.hpp>
#include <boost/fusion/include/at_c.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/include/is_sequence.hpp>
#include <boost/fusion/include/at.hpp>
#include <boost/type_index.hpp>

#include <array>
#include <vector>
#include <deque>
#include <cstdint>
#include <limits>
#include <string>
#include <memory>
#include <map>
#include <unordered_map>
#include <type_traits>

#if __cplusplus >= 201703L && !defined(FUSER_ONLY_CXX_11)
#include <optional>
#endif

namespace fuser {

template <typename TypeLackingImplementation>
static void make_compile_error()
{
	static_assert(
		sizeof(TypeLackingImplementation) == 0,
		"TypeLackingImplementation needs to have a (de)serializer implementation");
}

inline std::string dump_json(nlohmann::json const& json)
{
	return json.dump(4, ' ', false, nlohmann::json::error_handler_t::replace);
}

///////////////////////////////////////////////////////////////////////////////

/*
 * Default implementations: error that there is no implementation for this T.
 * The second (unused) template parameter to allow SFINAE for specializations.
 */
template <typename T, typename /* Enable */ = void>
struct serializer
{
	static nlohmann::json serialize(T const& /* val */)
	{
		make_compile_error<T>();
		return {}; // shut no return warning
	}
};

template <typename T, typename /* Enable */ = void>
struct deserializer
{
	static T deserialize(nlohmann::json const& /* json */)
	{
		make_compile_error<T>();
		return {}; // shut no return warning
	}
};

///////////////////////////////////////////////////////////////////////////////

/*
 * basic implementations: just read/write the value of specified type
 */
template <typename T>
struct basic_serializer
{
	static nlohmann::json serialize(T const& val)
	{
		return val;
	}
};

template <typename T>
struct basic_deserializer
{
	static T deserialize(nlohmann::json const& json)
	{
		return json.get<T>();
	}
};

template <>
struct serializer<bool> : basic_serializer<bool> {};
template <>
struct deserializer<bool> : basic_deserializer<bool> {};

template <>
struct serializer<std::string> : basic_serializer<std::string> {};
template <>
struct deserializer<std::string> : basic_deserializer<std::string> {};

///////////////////////////////////////////////////////////////////////////////

struct null_serializer
{
	static nlohmann::json serialize(std::nullptr_t)
	{
		return nullptr;
	}
};

struct null_deserializer
{
	static std::nullptr_t deserialize(nlohmann::json const& json)
	{
		if (!json.is_null()) {
			throw std::invalid_argument("Null serializer should only get JSONs of type null");
		}

		return nullptr;
	}
};

template <>
struct serializer<std::nullptr_t> : null_serializer {};
template <>
struct deserializer<std::nullptr_t> : null_deserializer {};

///////////////////////////////////////////////////////////////////////////////

/*
 * implementation for numeric types:
 * basic implementation but checks if the value fits into the target type
 */
template <typename NumericType, typename /* LargestNumericType */>
struct numeric_serializer : basic_serializer<NumericType> {};

template <typename NumericType, typename LargestNumericType>
struct numeric_deserializer
{
	static NumericType deserialize(nlohmann::json const& json)
	{
		auto const val = json.get<LargestNumericType>();
		constexpr auto min_allowed = std::numeric_limits<NumericType>::min();
		constexpr auto max_allowed = std::numeric_limits<NumericType>::max();

		if (val > max_allowed || val < min_allowed) {
			throw std::invalid_argument("value " + std::to_string(val)
				+ " is outside allowed range: " + std::to_string(min_allowed)
				+ " - " + std::to_string(max_allowed));
		}

		return static_cast<NumericType>(val);
	}
};

template <>
struct serializer<std::uint8_t> : numeric_serializer<std::uint8_t, std::uintmax_t> {};
template <>
struct serializer<std::uint16_t> : numeric_serializer<std::uint16_t, std::uintmax_t> {};
template <>
struct serializer<std::uint32_t> : numeric_serializer<std::uint32_t, std::uintmax_t> {};
template <>
struct serializer<std::uint64_t> : numeric_serializer<std::uint64_t, std::uintmax_t> {};

template <>
struct deserializer<std::uint8_t> : numeric_deserializer<std::uint8_t, std::uintmax_t> {};
template <>
struct deserializer<std::uint16_t> : numeric_deserializer<std::uint16_t, std::uintmax_t> {};
template <>
struct deserializer<std::uint32_t> : numeric_deserializer<std::uint32_t, std::uintmax_t> {};
template <>
struct deserializer<std::uint64_t> : numeric_deserializer<std::uint64_t, std::uintmax_t> {};

template <>
struct serializer<std::int8_t> : numeric_serializer<std::int8_t, std::intmax_t> {};
template <>
struct serializer<std::int16_t> : numeric_serializer<std::int16_t, std::intmax_t> {};
template <>
struct serializer<std::int32_t> : numeric_serializer<std::int32_t, std::intmax_t> {};
template <>
struct serializer<std::int64_t> : numeric_serializer<std::int64_t, std::intmax_t> {};

template <>
struct deserializer<std::int8_t> : numeric_deserializer<std::int8_t, std::intmax_t> {};
template <>
struct deserializer<std::int16_t> : numeric_deserializer<std::int16_t, std::intmax_t> {};
template <>
struct deserializer<std::int32_t> : numeric_deserializer<std::int32_t, std::intmax_t> {};
template <>
struct deserializer<std::int64_t> : numeric_deserializer<std::int64_t, std::intmax_t> {};

template <>
struct serializer<float> : numeric_serializer<float, long double> {};
template <>
struct serializer<double> : numeric_serializer<double, long double> {};
template <>
struct serializer<long double> : numeric_serializer<long double, long double> {};

template <>
struct deserializer<float> : numeric_deserializer<float, long double> {};
template <>
struct deserializer<double> : numeric_deserializer<double, long double> {};
template <>
struct deserializer<long double> : numeric_deserializer<long double, long double> {};

///////////////////////////////////////////////////////////////////////////////

/*
 * implementation for pointers: convert to/from std::uintptr_t
 */
template <typename T>
struct pointer_serializer
{
	static nlohmann::json serialize(T const& val)
	{
		return reinterpret_cast<std::uintptr_t>(val);
	}
};

template <typename T>
struct pointer_deserializer
{
	static T deserialize(nlohmann::json const& json)
	{
		return reinterpret_cast<T>(json.get<std::uintptr_t>());
	}
};

template <>
struct serializer<void*> : pointer_serializer<void*> {};
template <>
struct deserializer<void*> : pointer_deserializer<void*> {};

template <>
struct serializer<void const*> : pointer_serializer<void const*> {};
template <>
struct deserializer<void const*> : pointer_deserializer<void const*> {};

///////////////////////////////////////////////////////////////////////////////

/*
 * implementation for unique pointers: if null output null, else forward implementation to T
 * shared_ptr intentionally not implemented, because there can be cycles or duplicated data
 */
template <typename T>
struct unique_pointer_serializer
{
	static nlohmann::json serialize(T const& val)
	{
		if (val)
			return serializer<typename T::element_type>::serialize(*val);
		else
			return nullptr;
	}
};

template <typename T>
struct unique_pointer_deserializer
{
	static T deserialize(nlohmann::json const& json)
	{
		using element_type = typename T::element_type;

		if (json.is_null())
			return nullptr;
		else
			// new instead of std::make_unique to lower requirements to C++11
			return T(new element_type(deserializer<element_type>::deserialize(json)));
	}
};

template <typename T>
struct serializer<std::unique_ptr<T>> : unique_pointer_serializer<std::unique_ptr<T>> {};
template <typename T>
struct deserializer<std::unique_ptr<T>> : unique_pointer_deserializer<std::unique_ptr<T>> {};

///////////////////////////////////////////////////////////////////////////////

#if __cplusplus >= 201703L && !defined(FUSER_ONLY_CXX_11)
/*
 * implementation for std::optional
 */
template <typename T>
struct optional_serializer
{
	static nlohmann::json serialize(T const& val)
	{
		if (val)
			return serializer<typename T::value_type>::serialize(*val);
		else
			return nullptr;
	}
};

template <typename T>
struct optional_deserializer
{
	static T deserialize(nlohmann::json const& json)
	{
		if (json.is_null())
			return std::nullopt;
		else
			return T(deserializer<typename T::value_type>::deserialize(json));
	}
};

template <typename T>
struct serializer<std::optional<T>> : optional_serializer<std::optional<T>> {};
template <typename T>
struct deserializer<std::optional<T>> : optional_deserializer<std::optional<T>> {};

#endif

///////////////////////////////////////////////////////////////////////////////

/*
 * map implementation: very similar to fusion, but check if the key serializes to a string
 */
template <typename Map>
struct map_serializer
{
	static nlohmann::json serialize(Map const& map)
	{
		nlohmann::json json = nlohmann::json::object();

		for (auto const& p : map) {
			using key_type = typename Map::key_type;
			using mapped_type = typename Map::mapped_type;

			key_type const& key = p.first;
			mapped_type const& val = p.second;

			nlohmann::json key_serialized = serializer<key_type>::serialize(key);
			if (!key_serialized.is_string())
				throw std::logic_error("Map key serializer must output a JSON of type string.");

			using string_ref = nlohmann::json::string_t&;
			json[std::move(key_serialized.get_ref<string_ref>())] = serializer<mapped_type>::serialize(val);
		}

		return json;
	}
};

template <typename Map>
struct map_deserializer
{
	static Map deserialize(nlohmann::json const& json)
	{
		if (!json.is_object()) {
			throw std::invalid_argument("Map deserializer must get a JSON of type object.");
		}

		Map map;

		for (auto const& obj : json.items()) {
			using key_type = typename Map::key_type;
			using mapped_type = typename Map::mapped_type;

			map[deserializer<key_type>::deserialize(obj.key())] = deserializer<mapped_type>::deserialize(obj.value());
		}

		return map;
	}
};

/*
 * allocator type intentionally ommited - the current implementation
 * does not provide any guuarantees for stateful allocators
 */
template <typename K, typename V, typename C>
struct serializer<std::map<K, V, C>> : map_serializer<std::map<K, V, C>> {};
template <typename K, typename V, typename C>
struct deserializer<std::map<K, V, C>> : map_deserializer<std::map<K, V, C>> {};

template <typename K, typename V, typename H, typename E>
struct serializer<std::unordered_map<K, V, H, E>> : map_serializer<std::unordered_map<K, V, H, E>> {};
template <typename K, typename V, typename H, typename E>
struct deserializer<std::unordered_map<K, V, H, E>> : map_deserializer<std::unordered_map<K, V, H, E>> {};

///////////////////////////////////////////////////////////////////////////////

/*
 * fusion implementation: use boost fusion library to recursively iterate over each member
 */
template <typename FusionSequence>
struct fusion_serializer_impl
{
	fusion_serializer_impl(nlohmann::json& json, FusionSequence const& seq)
	: json(json), seq(seq) {}

	template <typename Index>
	void operator()(Index)
	{
		using member_type = typename boost::fusion::result_of::value_at<FusionSequence, Index>::type;
		auto const member_name = boost::fusion::extension::struct_member_name<FusionSequence, Index::value>::call();

		auto const& member = boost::fusion::at<Index>(seq);
		json[member_name] = serializer<member_type>::serialize(member);
	}

	nlohmann::json& json;
	FusionSequence const& seq;
};

template <typename FusionSequence>
struct fusion_deserializer_impl
{
	fusion_deserializer_impl(nlohmann::json const& json, FusionSequence& seq)
	: json(json), seq(seq) {}

	template <typename Index>
	void operator()(Index)
	{
		using member_type = typename boost::fusion::result_of::value_at<FusionSequence, Index>::type;
		auto const member_name = boost::fusion::extension::struct_member_name<FusionSequence, Index::value>::call();

		// wrap member_type_name in a lambda so that it is only invoked when an exception is thrown
		// this avoids (potentially expensive) RTTI use + demangling call
		auto member_type_name = []() { return boost::typeindex::type_id<member_type>().pretty_name(); };

		auto& member = boost::fusion::at<Index>(seq);
		try {
			member = deserializer<member_type>::deserialize(json.at(member_name));
		}
		catch (nlohmann::json::exception const&) {
			throw std::invalid_argument(std::string("no value of name \"") + member_name
				+ "\" of type \"" + member_type_name() + "\" in the following json: "
				+ dump_json(json));
		}
		catch (std::invalid_argument const& e) {
			throw std::invalid_argument(std::string("value of name \"") + member_name
				+ "\" of type \"" + member_type_name() + "\" in the following json: "
				+ dump_json(json) + " is invalid: " + e.what());
		}
	}

	nlohmann::json const& json;
	FusionSequence& seq;
};

template <typename FusionSequence>
struct fusion_impl_base
{
	static constexpr auto seq_size = boost::fusion::result_of::size<FusionSequence>::value;
	using indices_type = boost::mpl::range_c<std::size_t, 0, seq_size>;
};

template <typename FusionSequence>
struct fusion_serializer : fusion_impl_base<FusionSequence>
{
	using base_type = fusion_impl_base<FusionSequence>;
	using indices_type = typename base_type::indices_type;

	static nlohmann::json serialize(FusionSequence const& seq)
	{
		nlohmann::json json = nlohmann::json::object();
		boost::fusion::for_each(indices_type{}, fusion_serializer_impl<FusionSequence>(json, seq));
		return json;
	}
};

template <typename FusionSequence>
struct fusion_deserializer : fusion_impl_base<FusionSequence>
{
	using base_type = fusion_impl_base<FusionSequence>;
	using indices_type = typename base_type::indices_type;

	static FusionSequence deserialize(nlohmann::json const& json)
	{
		FusionSequence seq;
		boost::fusion::for_each(indices_type{}, fusion_deserializer_impl<FusionSequence>(json, seq));
		return seq;
	}
};

// (de)serialize as fusion sequence only when T is a fusion sequence
template <typename T>
struct serializer<
	T,
	typename std::enable_if<
		boost::fusion::traits::is_sequence<T>::type::value
	>::type
> : fusion_serializer<T> {};

template <typename T>
struct deserializer<
	T,
	typename std::enable_if<
		boost::fusion::traits::is_sequence<T>::type::value
	>::type
> : fusion_deserializer<T> {};

///////////////////////////////////////////////////////////////////////////////

/*
 * implementation for common containers: just loop over each value
 */
template <typename Container>
struct sequence_container_serializer
{
	static nlohmann::json serialize(Container const& container)
	{
		nlohmann::json result = nlohmann::json::array();
		// there is no reserve() on public API of the JSON library
		// instead, obtain a reference to the internal array type and call it there
		// https://github.com/nlohmann/json/issues/1935
		result.get_ref<nlohmann::json::array_t&>().reserve(container.size());

		for (auto const& obj : container) {
			using value_type = typename Container::value_type;
			auto val = serializer<value_type>::serialize(obj);
			result.push_back(std::move(val));
		}

		return result;
	}
};

namespace impl {

template <typename...>
using void_t = void;

template <typename, typename = void>
struct has_reserve_member : std::false_type {};

template <typename T>
struct has_reserve_member<T, void_t<decltype(std::declval<T>().reserve)>> : std::true_type {};

}

template <typename Container>
struct sequence_container_deserializer
{
	static void reserve(Container& container, std::size_t n, std::true_type)
	{
		container.reserve(n);
	}

	static void reserve(Container& /* container */, std::size_t /* n */, std::false_type)
	{
	}

	static Container deserialize(nlohmann::json const& json)
	{
		if (!json.is_array()) {
			throw std::invalid_argument("JSON object should be an array.");
		}

		Container result;
		reserve(result, json.size(), impl::has_reserve_member<Container>{});

		for (auto const& obj : json) {
			using value_type = typename Container::value_type;
			auto val = deserializer<value_type>::deserialize(obj);
			result.push_back(std::move(val));
		}

		return result;
	}
};

template <typename T>
struct serializer<std::vector<T>> : sequence_container_serializer<std::vector<T>> {};
template <typename T>
struct deserializer<std::vector<T>> : sequence_container_deserializer<std::vector<T>> {};

template <typename T>
struct serializer<std::deque<T>> : sequence_container_serializer<std::deque<T>> {};
template <typename T>
struct deserializer<std::deque<T>> : sequence_container_deserializer<std::deque<T>> {};

template <typename Container>
struct array_container_serializer : sequence_container_serializer<Container> {};

template <typename Container>
struct array_container_deserializer
{
	static Container deserialize(nlohmann::json const& json)
	{
		if (!json.is_array()) {
			throw std::invalid_argument("JSON object should be an array.");
		}

		constexpr auto arr_size = std::tuple_size<Container>::value;
		if (json.size() != arr_size) {
			throw std::invalid_argument("Expected " + std::to_string(arr_size)
				+ " values but got " + std::to_string(json.size()));
		}

		Container result;

		for (std::size_t i = 0; i < result.size(); ++i) {
			using value_type = typename Container::value_type;
			result[i] = deserializer<value_type>::deserialize(json[i]);
		}

		return result;
	}
};

template <typename T, std::size_t N>
struct serializer<std::array<T, N>> : array_container_serializer<std::array<T, N>> {};
template <typename T, std::size_t N>
struct deserializer<std::array<T, N>> : array_container_deserializer<std::array<T, N>> {};

///////////////////////////////////////////////////////////////////////////////

template <typename T>
constexpr void assert_type_requirements()
{
	static_assert(std::is_default_constructible<T>::value, "T should be default constructible");
	static_assert(std::is_move_constructible<T>::value, "T should be move constructible");
	static_assert(std::is_move_assignable<T>::value, "T should be move assignable");
}

/*
 * core functions on the public API
 */
template <typename T>
nlohmann::json serialize(T const& val)
{
	assert_type_requirements<T>();
	return serializer<T>::serialize(val);
}

template <typename T>
T deserialize(nlohmann::json const& json)
{
	assert_type_requirements<T>();
	return deserializer<T>::deserialize(json);
}

} // namespace fuser
