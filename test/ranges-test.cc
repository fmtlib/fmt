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

#include "fmt/ranges.h"

#include "gtest.h"

// Check if  'if constexpr' is supported.
#if (__cplusplus > 201402L) || \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L && _MSC_VER >= 1910)

#  include <array>
#  include <map>
#  include <string>
#  include <vector>

TEST(RangesTest, FormatArray) {
  int32_t ia[] = {1, 2, 3, 5, 7, 11};
  auto iaf = fmt::format("{}", ia);
  EXPECT_EQ("{1, 2, 3, 5, 7, 11}", iaf);
}

TEST(RangesTest, Format2dArray) {
  int32_t ia[][2] = {{1, 2}, {3, 5}, {7, 11}};
  auto iaf = fmt::format("{}", ia);
  EXPECT_EQ("{{1, 2}, {3, 5}, {7, 11}}", iaf);
}

TEST(RangesTest, FormatVector) {
  std::vector<int32_t> iv{1, 2, 3, 5, 7, 11};
  auto ivf = fmt::format("{}", iv);
  EXPECT_EQ("{1, 2, 3, 5, 7, 11}", ivf);
}

TEST(RangesTest, FormatVector2) {
  std::vector<std::vector<int32_t>> ivv{{1, 2}, {3, 5}, {7, 11}};
  auto ivf = fmt::format("{}", ivv);
  EXPECT_EQ("{{1, 2}, {3, 5}, {7, 11}}", ivf);
}

TEST(RangesTest, FormatMap) {
  std::map<std::string, int32_t> simap{{"one", 1}, {"two", 2}};
  EXPECT_EQ("{(\"one\", 1), (\"two\", 2)}", fmt::format("{}", simap));
}

TEST(RangesTest, FormatPair) {
  std::pair<int64_t, float> pa1{42, 1.5f};
  EXPECT_EQ("(42, 1.5)", fmt::format("{}", pa1));
}

TEST(RangesTest, FormatTuple) {
  std::tuple<int64_t, float, std::string, char> t{42, 1.5f, "this is tuple",
                                                  'i'};
  EXPECT_EQ("(42, 1.5, \"this is tuple\", 'i')", fmt::format("{}", t));
  EXPECT_EQ("()", fmt::format("{}", std::tuple<>()));
}

TEST(RangesTest, JoinTuple) {
  // Value tuple args
  std::tuple<char, int, float> t1 = std::make_tuple('a', 1, 2.0f);
  EXPECT_EQ("(a, 1, 2)", fmt::format("({})", fmt::join(t1, ", ")));

  // Testing lvalue tuple args
  int x = 4;
  std::tuple<char, int&> t2{'b', x};
  EXPECT_EQ("b + 4", fmt::format("{}", fmt::join(t2, " + ")));

  // Empty tuple
  std::tuple<> t3;
  EXPECT_EQ("", fmt::format("{}", fmt::join(t3, "|")));

  // Single element tuple
  std::tuple<float> t4{4.0f};
  EXPECT_EQ("4", fmt::format("{}", fmt::join(t4, "/")));
}

TEST(RangesTest, JoinInitializerList) {
  EXPECT_EQ("1, 2, 3", fmt::format("{}", fmt::join({1, 2, 3}, ", ")));
  EXPECT_EQ("fmt rocks !",
            fmt::format("{}", fmt::join({"fmt", "rocks", "!"}, " ")));
}

struct my_struct {
  int32_t i;
  std::string str;  // can throw
  template <size_t N> decltype(auto) get() const noexcept {
    if constexpr (N == 0)
      return i;
    else if constexpr (N == 1)
      return fmt::string_view{str};
  }
};

template <size_t N> decltype(auto) get(const my_struct& s) noexcept {
  return s.get<N>();
}

namespace std {

template <> struct tuple_size<my_struct> : std::integral_constant<size_t, 2> {};

template <size_t N> struct tuple_element<N, my_struct> {
  using type = decltype(std::declval<my_struct>().get<N>());
};

}  // namespace std

TEST(RangesTest, FormatStruct) {
  my_struct mst{13, "my struct"};
  EXPECT_EQ("(13, \"my struct\")", fmt::format("{}", mst));
}

TEST(RangesTest, FormatTo) {
  char buf[10];
  auto end = fmt::format_to(buf, "{}", std::vector{1, 2, 3});
  *end = '\0';
  EXPECT_STREQ(buf, "{1, 2, 3}");
}

struct path_like {
  const path_like* begin() const;
  const path_like* end() const;

  operator std::string() const;
};

TEST(RangesTest, PathLike) {
  EXPECT_FALSE((fmt::is_range<path_like, char>::value));
}

#endif  // (__cplusplus > 201402L) || (defined(_MSVC_LANG) && _MSVC_LANG >
        // 201402L && _MSC_VER >= 1910)

#ifdef FMT_USE_STRING_VIEW
struct string_like {
  const char* begin();
  const char* end();
  explicit operator fmt::string_view() const { return "foo"; }
  explicit operator std::string_view() const { return "foo"; }
};

TEST(RangesTest, FormatStringLike) {
  EXPECT_EQ("foo", fmt::format("{}", string_like()));
}
#endif  // FMT_USE_STRING_VIEW

struct zstring_sentinel {};

bool operator==(const char* p, zstring_sentinel) { return *p == '\0'; }
bool operator!=(const char* p, zstring_sentinel) { return *p != '\0'; }

struct zstring {
  const char* p;
  const char* begin() const { return p; }
  zstring_sentinel end() const { return {}; }
};

TEST(RangesTest, JoinSentinel) {
  zstring hello{"hello"};
  EXPECT_EQ("{'h', 'e', 'l', 'l', 'o'}", fmt::format("{}", hello));
  EXPECT_EQ("h_e_l_l_o", fmt::format("{}", fmt::join(hello, "_")));
}

// A range that provides non-const only begin()/end() to test fmt::join handles
// that
//
// Some ranges (eg those produced by range-v3's views::filter()) can cache
// information during iteration so they only provide non-const begin()/end().
template <typename T> class non_const_only_range {
 private:
  std::vector<T> vec;

 public:
  using const_iterator = typename ::std::vector<T>::const_iterator;

  template <typename... Args>
  explicit non_const_only_range(Args&&... args)
      : vec(::std::forward<Args>(args)...) {}

  const_iterator begin() { return vec.begin(); }
  const_iterator end() { return vec.end(); }
};

template <typename T> class noncopyable_range {
 private:
  std::vector<T> vec;

 public:
  using const_iterator = typename ::std::vector<T>::const_iterator;

  template <typename... Args>
  explicit noncopyable_range(Args&&... args)
      : vec(::std::forward<Args>(args)...) {}

  noncopyable_range(noncopyable_range const&) = delete;
  noncopyable_range(noncopyable_range&) = delete;

  const_iterator begin() const { return vec.begin(); }
  const_iterator end() const { return vec.end(); }
};

TEST(RangesTest, JoinRange) {
  noncopyable_range<int> w(3u, 0);
  EXPECT_EQ("0,0,0", fmt::format("{}", fmt::join(w, ",")));
  EXPECT_EQ("0,0,0",
            fmt::format("{}", fmt::join(noncopyable_range<int>(3u, 0), ",")));

  non_const_only_range<int> x(3u, 0);
  EXPECT_EQ("0,0,0", fmt::format("{}", fmt::join(x, ",")));
  EXPECT_EQ(
      "0,0,0",
      fmt::format("{}", fmt::join(non_const_only_range<int>(3u, 0), ",")));

  std::vector<int> y(3u, 0);
  EXPECT_EQ("0,0,0", fmt::format("{}", fmt::join(y, ",")));
  EXPECT_EQ("0,0,0",
            fmt::format("{}", fmt::join(std::vector<int>(3u, 0), ",")));

  const std::vector<int> z(3u, 0);
  EXPECT_EQ("0,0,0", fmt::format("{}", fmt::join(z, ",")));
}

TEST(RangesTest, Range) {
  noncopyable_range<int> w(3u, 0);
  EXPECT_EQ("{0, 0, 0}", fmt::format("{}", w));
  EXPECT_EQ("{0, 0, 0}", fmt::format("{}", noncopyable_range<int>(3u, 0)));

  non_const_only_range<int> x(3u, 0);
  EXPECT_EQ("{0, 0, 0}", fmt::format("{}", x));
  EXPECT_EQ("{0, 0, 0}", fmt::format("{}", non_const_only_range<int>(3u, 0)));

  std::vector<int> y(3u, 0);
  EXPECT_EQ("{0, 0, 0}", fmt::format("{}", y));
  EXPECT_EQ("{0, 0, 0}", fmt::format("{}", std::vector<int>(3u, 0)));

  const std::vector<int> z(3u, 0);
  EXPECT_EQ("{0, 0, 0}", fmt::format("{}", z));
}

#if !FMT_MSC_VER || FMT_MSC_VER >= 1927
struct unformattable {};

TEST(RangesTest, UnformattableRange) {
  EXPECT_FALSE((fmt::has_formatter<std::vector<unformattable>,
                                   fmt::format_context>::value));
}
#endif
