# fuser

1-file header-only library for automatic (de)serialization of C++ types to/from JSON.

## how it works

The library has a predefined set of (de)serializers for common types:

- `bool`
- all standard integer (`std::int*_t`, `std::uint*_t`) and floating-point (`float`, `double`, `long double`) numeric types (checks if the value fits into the destination type)
- `char` is not explicitly implemented, but it will very likely fall into one of fixed-width integer types
- `char8_t`, `char16_t` and `char32_t` are not implemented
- `void*` and `void const*` (converts to `std::uintptr_t`)
- `std::string`
- `std::array<T, N>` (checks if array size matches)
- `std::vector<T>`
- `std::deque<T>`
- `std::unique_ptr<T>` (outputs `null` when there is no value)
- `std::optional<T>` (outputs `null` when there is no value)
- `std::map<K, V, C>`
- `std::unordered_map<K, V, H, E>`

Additionally, it supports (de)serializers for structures which are valid boost fusion sequences.

Note that the above list is exactly how template specializations are implemented. The allocator template parameter is intentionally ommited because the current implementation has no guuarantees for stateful allocators. You can always define your (partial) specializations that reuse the existing serializers - just check how they are implemented.

If a type has no (de)serializer, the library will gracefully fail on a `static_assert`.

## requirements

- Boost >= 1.56 (only header-only libraries)
- [nlohmann/json](github.com/nlohmann/json) >= 3.0
- C++11
- C++17 (optional, to enable `std::optional` support)
- CMake >= 3.13

## example

```cpp
#include <fuser/fuser.hpp>

#include <vector>
#include <optional>
#include <iostream>

struct my_struct
{
	int i;
	double d;
	std::optional<bool> b;
};
// note: fusion macros must be used in global scope
BOOST_FUSION_ADAPT_STRUCT(my_struct, i, d, b)

int main()
{
	std::vector<my_struct> v = {
		{0, 3.14, {true}}, {1, 0.125, {false}}, {0, 0.0, {std::nullopt}}
	};

	std::cout << fuser::serialize(v).dump(4, ' ');
}
```

output:

```json
[
    {
        "b": true,
        "d": 3.14,
        "i": 0
    },
    {
        "b": false,
        "d": 0.125,
        "i": 1
    },
    {
        "b": null,
        "d": 0.0,
        "i": 0
    }
]
```

## library API

### basic functions

```cpp
namespace fuser {

template <typename T>
nlohmann::json serialize(T const& val);

template <typename T>
T deserialize(nlohmann::json const& json);

}
```

### defining your own (de)serializer

```cpp
template <>
struct fuser::serializer<my_type>
{
	static nlohmann::json serialize(my_type const& val)
	{
		/* ... */
	}
};

template <>
struct fuser::deserializer<my_type>
{
	static my_type deserialize(nlohmann::json const& json)
	{
		/* ... */
	}
};
```

Both class templates have a second unused template parameter to allow easy SFINAE for specializations.

## trivia

The library is named "fuser" because it is a simple merge of boost **fus**ion and **ser**ialization.

Special thanks to [cycfi/json](https://github.com/cycfi/json) for the original idea.
