// Formatting library for C++ - the core API
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.
//
// Copyright (c) 2018 - present, Remotion (Igor Schulz)
// All Rights Reserved
// {fmt} support for ranges, containers and types tuple interface.

#include <map>
#include <string>
#include <unordered_map>

#include "fmt/ranges.h"
#include "gtest/gtest.h"

template <typename T> struct is_std_map_like : std::false_type {};

template <typename... Ts>
struct is_std_map_like<std::map<Ts...>> : std::true_type {};

template <typename... Ts>
struct is_std_map_like<std::unordered_map<Ts...>> : std::true_type {};

template <typename T> struct is_std_pair : std::false_type {};
template <typename... Ts>
struct is_std_pair<std::pair<Ts...>> : std::true_type {};

namespace fmt {

// specialized map format based on range_formatter
template <typename T, typename Char>
requires FormattableRange<T, Char> &&
    is_std_map_like<T>::value struct formatter<T, Char>
    : range_formatter<T, Char> {
  using Base = range_formatter<T, Char>;
  formatter() : Base{{"{", ", ", "}"}} {}
  using Base::format;
  using Base::parse;
};

// specialized pair format based on tuple_formatter
template <typename T, typename Char>
requires fmt::TupleLike<T> &&
    is_std_pair<T>::value struct formatter<T, Char> : tuple_formatter<T, Char> {
  using Base = tuple_formatter<T, Char>;
  formatter() : Base{{"{", ": ", "}"}} {}
  using Base::format;
  using Base::parse;
};

}  // namespace fmt

TEST(ranges_custom_formatter_test, format_map) {
  auto m = std::map<std::string, int>{{"one", 1}, {"two", 2}};
  EXPECT_EQ(fmt::format("{}", m), "{{\"one\": 1}, {\"two\": 2}}");
}

TEST(ranges_custom_formatter_test, format_pair) {
  auto p = std::pair<int, float>(42, 1.5f);
  EXPECT_EQ(fmt::format("{}", p), "{42: 1.5}");
}
