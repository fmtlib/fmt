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

#include <map>
#include <queue>
#include <stack>
#include <string>
#include <utility>
#include <vector>

#include "gtest/gtest.h"

#if !FMT_GCC_VERSION || FMT_GCC_VERSION >= 601
#  define FMT_RANGES_TEST_ENABLE_C_STYLE_ARRAY
#endif

#if !FMT_MSC_VERSION || FMT_MSC_VERSION > 1910
#  define FMT_RANGES_TEST_ENABLE_JOIN
#  define FMT_RANGES_TEST_ENABLE_FORMAT_STRUCT
#endif

#ifdef FMT_RANGES_TEST_ENABLE_C_STYLE_ARRAY
TEST(ranges_test, format_array) {
  int arr[] = {1, 2, 3, 5, 7, 11};
  EXPECT_EQ(fmt::format("{}", arr), "[1, 2, 3, 5, 7, 11]");
}

TEST(ranges_test, format_2d_array) {
  int arr[][2] = {{1, 2}, {3, 5}, {7, 11}};
  EXPECT_EQ(fmt::format("{}", arr), "[[1, 2], [3, 5], [7, 11]]");
}

TEST(ranges_test, format_array_of_literals) {
  const char* arr[] = {"1234", "abcd"};
  EXPECT_EQ(fmt::format("{}", arr), "[\"1234\", \"abcd\"]");
  EXPECT_EQ(fmt::format("{:n}", arr), "\"1234\", \"abcd\"");
  EXPECT_EQ(fmt::format("{:n:}", arr), "1234, abcd");
}
#endif  // FMT_RANGES_TEST_ENABLE_C_STYLE_ARRAY

TEST(ranges_test, format_vector) {
  auto v = std::vector<int>{1, 2, 3, 5, 7, 11};
  EXPECT_EQ(fmt::format("{}", v), "[1, 2, 3, 5, 7, 11]");
  EXPECT_EQ(fmt::format("{::#x}", v), "[0x1, 0x2, 0x3, 0x5, 0x7, 0xb]");
  EXPECT_EQ(fmt::format("{:n:#x}", v), "0x1, 0x2, 0x3, 0x5, 0x7, 0xb");

  auto vc = std::vector<char>{'a', 'b', 'c'};
  auto vvc = std::vector<std::vector<char>>{vc, vc};
  EXPECT_EQ(fmt::format("{}", vc), "['a', 'b', 'c']");
  EXPECT_EQ(fmt::format("{}", vvc), "[['a', 'b', 'c'], ['a', 'b', 'c']]");
  EXPECT_EQ(fmt::format("{:n}", vvc), "['a', 'b', 'c'], ['a', 'b', 'c']");
  EXPECT_EQ(fmt::format("{:n:n}", vvc), "'a', 'b', 'c', 'a', 'b', 'c'");
  EXPECT_EQ(fmt::format("{:n:n:}", vvc), "a, b, c, a, b, c");
}

TEST(ranges_test, format_vector2) {
  auto v = std::vector<std::vector<int>>{{1, 2}, {3, 5}, {7, 11}};
  EXPECT_EQ(fmt::format("{}", v), "[[1, 2], [3, 5], [7, 11]]");
  EXPECT_EQ(fmt::format("{:::#x}", v), "[[0x1, 0x2], [0x3, 0x5], [0x7, 0xb]]");
  EXPECT_EQ(fmt::format("{:n:n:#x}", v), "0x1, 0x2, 0x3, 0x5, 0x7, 0xb");
}

TEST(ranges_test, format_map) {
  auto m = std::map<std::string, int>{{"one", 1}, {"two", 2}};
  EXPECT_EQ(fmt::format("{}", m), "{\"one\": 1, \"two\": 2}");
  EXPECT_EQ(fmt::format("{:n}", m), "\"one\": 1, \"two\": 2");
}

TEST(ranges_test, format_set) {
  EXPECT_EQ(fmt::format("{}", std::set<std::string>{"one", "two"}),
            "{\"one\", \"two\"}");
}

// Models std::flat_set close enough to test if no ambiguous lookup of a
// formatter happens due to the flat_set type matching is_set and
// is_container_adaptor_like
template <typename T> class flat_set {
 public:
  using key_type = T;
  using container_type = std::vector<T>;

  using iterator = typename std::vector<T>::iterator;
  using const_iterator = typename std::vector<T>::const_iterator;

  template <typename... Ts>
  explicit flat_set(Ts&&... args) : c{std::forward<Ts>(args)...} {}

  iterator begin() { return c.begin(); }
  const_iterator begin() const { return c.begin(); }

  iterator end() { return c.end(); }
  const_iterator end() const { return c.end(); }

 private:
  std::vector<T> c;
};

TEST(ranges_test, format_flat_set) {
  EXPECT_EQ(fmt::format("{}", flat_set<std::string>{"one", "two"}),
            "{\"one\", \"two\"}");
}

namespace adl {
struct box {
  int value;
};

auto begin(const box& b) -> const int* { return &b.value; }

auto end(const box& b) -> const int* { return &b.value + 1; }
}  // namespace adl

TEST(ranges_test, format_adl_begin_end) {
  auto b = adl::box{42};
  EXPECT_EQ(fmt::format("{}", b), "[42]");
}

TEST(ranges_test, format_pair) {
  auto p = std::pair<int, float>(42, 1.5f);
  EXPECT_EQ(fmt::format("{}", p), "(42, 1.5)");
}

struct unformattable {};

TEST(ranges_test, format_tuple) {
  auto t =
      std::tuple<int, float, std::string, char>(42, 1.5f, "this is tuple", 'i');
  EXPECT_EQ(fmt::format("{}", t), "(42, 1.5, \"this is tuple\", 'i')");

  EXPECT_EQ(fmt::format("{}", std::tuple<>()), "()");

  EXPECT_TRUE((fmt::is_formattable<std::tuple<>>::value));
  EXPECT_FALSE((fmt::is_formattable<unformattable>::value));
  EXPECT_FALSE((fmt::is_formattable<std::tuple<unformattable>>::value));
  EXPECT_FALSE((fmt::is_formattable<std::tuple<unformattable, int>>::value));
  EXPECT_FALSE((fmt::is_formattable<std::tuple<int, unformattable>>::value));
  EXPECT_FALSE(
      (fmt::is_formattable<std::tuple<unformattable, unformattable>>::value));
  EXPECT_TRUE((fmt::is_formattable<std::tuple<int, float>>::value));
}

struct not_default_formattable {};
struct bad_format {};

FMT_BEGIN_NAMESPACE
template <> struct formatter<not_default_formattable> {
  auto parse(format_parse_context&) -> const char* { throw bad_format(); }
  auto format(not_default_formattable, format_context& ctx)
      -> format_context::iterator {
    return ctx.out();
  }
};
FMT_END_NAMESPACE

TEST(ranges_test, tuple_parse_calls_element_parse) {
  auto f = fmt::formatter<std::tuple<not_default_formattable>>();
  auto ctx = fmt::format_parse_context("");
  EXPECT_THROW(f.parse(ctx), bad_format);
}

#ifdef FMT_RANGES_TEST_ENABLE_FORMAT_STRUCT
struct tuple_like {
  int i;
  std::string str;

  template <size_t N> fmt::enable_if_t<N == 0, int> get() const noexcept {
    return i;
  }
  template <size_t N>
  fmt::enable_if_t<N == 1, fmt::string_view> get() const noexcept {
    return str;
  }
};

template <size_t N>
auto get(const tuple_like& t) noexcept -> decltype(t.get<N>()) {
  return t.get<N>();
}

namespace std {
template <>
struct tuple_size<tuple_like> : std::integral_constant<size_t, 2> {};

template <size_t N> struct tuple_element<N, tuple_like> {
  using type = decltype(std::declval<tuple_like>().get<N>());
};
}  // namespace std

TEST(ranges_test, format_struct) {
  auto t = tuple_like{42, "foo"};
  EXPECT_EQ(fmt::format("{}", t), "(42, \"foo\")");
}
#endif  // FMT_RANGES_TEST_ENABLE_FORMAT_STRUCT

TEST(ranges_test, format_to) {
  char buf[10];
  auto end = fmt::format_to(buf, "{}", std::vector<int>{1, 2, 3});
  *end = '\0';
  EXPECT_STREQ(buf, "[1, 2, 3]");
}

template <typename Char> struct path_like {
  const path_like* begin() const;
  const path_like* end() const;

  operator std::basic_string<Char>() const;
};

TEST(ranges_test, disabled_range_formatting_of_path) {
  // Range formatting of path is disabled because of infinite recursion
  // (path element is itself a path).
  EXPECT_EQ((fmt::range_format_kind<path_like<char>, char>::value),
            fmt::range_format::disabled);
  EXPECT_EQ((fmt::range_format_kind<path_like<wchar_t>, char>::value),
            fmt::range_format::disabled);
}

// A range that provides non-const only begin()/end() to test fmt::join
// handles that.
//
// Some ranges (e.g. those produced by range-v3's views::filter()) can cache
// information during iteration so they only provide non-const begin()/end().
template <typename T> class non_const_only_range {
 private:
  std::vector<T> vec;

 public:
  using const_iterator = typename ::std::vector<T>::const_iterator;

  template <typename... Args>
  explicit non_const_only_range(Args&&... args)
      : vec(std::forward<Args>(args)...) {}

  const_iterator begin() { return vec.begin(); }
  const_iterator end() { return vec.end(); }
};

template <typename T> class noncopyable_range {
 private:
  std::vector<T> vec;

 public:
  using iterator = typename ::std::vector<T>::iterator;

  template <typename... Args>
  explicit noncopyable_range(Args&&... args)
      : vec(std::forward<Args>(args)...) {}

  noncopyable_range(noncopyable_range const&) = delete;
  noncopyable_range(noncopyable_range&) = delete;

  iterator begin() { return vec.begin(); }
  iterator end() { return vec.end(); }
};

TEST(ranges_test, range) {
  noncopyable_range<int> w(3u, 0);
  EXPECT_EQ(fmt::format("{}", w), "[0, 0, 0]");
  EXPECT_EQ(fmt::format("{}", noncopyable_range<int>(3u, 0)), "[0, 0, 0]");

  non_const_only_range<int> x(3u, 0);
  EXPECT_EQ(fmt::format("{}", x), "[0, 0, 0]");
  EXPECT_EQ(fmt::format("{}", non_const_only_range<int>(3u, 0)), "[0, 0, 0]");

  auto y = std::vector<int>(3u, 0);
  EXPECT_EQ(fmt::format("{}", y), "[0, 0, 0]");
  EXPECT_EQ(fmt::format("{}", std::vector<int>(3u, 0)), "[0, 0, 0]");

  const auto z = std::vector<int>(3u, 0);
  EXPECT_EQ(fmt::format("{}", z), "[0, 0, 0]");
}

enum test_enum { foo };
auto format_as(test_enum e) -> int { return e; }

TEST(ranges_test, enum_range) {
  auto v = std::vector<test_enum>{test_enum::foo};
  EXPECT_EQ(fmt::format("{}", v), "[0]");
}

#if !FMT_MSC_VERSION
TEST(ranges_test, unformattable_range) {
  EXPECT_FALSE((fmt::has_formatter<std::vector<unformattable>,
                                   fmt::format_context>::value));
}
#endif

#ifdef FMT_RANGES_TEST_ENABLE_JOIN
TEST(ranges_test, join_tuple) {
  // Value tuple args.
  auto t1 = std::tuple<char, int, float>('a', 1, 2.0f);
  EXPECT_EQ(fmt::format("({})", fmt::join(t1, ", ")), "(a, 1, 2)");

  // Testing lvalue tuple args.
  int x = 4;
  auto t2 = std::tuple<char, int&>('b', x);
  EXPECT_EQ(fmt::format("{}", fmt::join(t2, " + ")), "b + 4");

  // Empty tuple.
  auto t3 = std::tuple<>();
  EXPECT_EQ(fmt::format("{}", fmt::join(t3, "|")), "");

  // Single element tuple.
  auto t4 = std::tuple<float>(4.0f);
  EXPECT_EQ(fmt::format("{}", fmt::join(t4, "/")), "4");

#  if FMT_TUPLE_JOIN_SPECIFIERS
  // Specs applied to each element.
  auto t5 = std::tuple<int, int, long>(-3, 100, 1);
  EXPECT_EQ(fmt::format("{:+03}", fmt::join(t5, ", ")), "-03, +100, +01");

  auto t6 = std::tuple<float, double, long double>(3, 3.14, 3.1415);
  EXPECT_EQ(fmt::format("{:5.5f}", fmt::join(t6, ", ")),
            "3.00000, 3.14000, 3.14150");

  // Testing lvalue tuple args.
  int y = -1;
  auto t7 = std::tuple<int, int&, const int&>(3, y, y);
  EXPECT_EQ(fmt::format("{:03}", fmt::join(t7, ", ")), "003, -01, -01");
#  endif
}

TEST(ranges_test, join_initializer_list) {
  EXPECT_EQ(fmt::format("{}", fmt::join({1, 2, 3}, ", ")), "1, 2, 3");
  EXPECT_EQ(fmt::format("{}", fmt::join({"fmt", "rocks", "!"}, " ")),
            "fmt rocks !");
}

struct zstring_sentinel {};

bool operator==(const char* p, zstring_sentinel) { return *p == '\0'; }
bool operator!=(const char* p, zstring_sentinel) { return *p != '\0'; }

struct zstring {
  const char* p;
  const char* begin() const { return p; }
  zstring_sentinel end() const { return {}; }
};

#  ifdef __cpp_lib_ranges
struct cpp20_only_range {
  struct iterator {
    int val = 0;

    using value_type = int;
    using difference_type = std::ptrdiff_t;
    using iterator_concept = std::input_iterator_tag;

    iterator() = default;
    iterator(int i) : val(i) {}
    int operator*() const { return val; }
    iterator& operator++() {
      ++val;
      return *this;
    }
    void operator++(int) { ++*this; }
    bool operator==(const iterator& rhs) const { return val == rhs.val; }
  };

  int lo;
  int hi;

  iterator begin() const { return iterator(lo); }
  iterator end() const { return iterator(hi); }
};

static_assert(std::input_iterator<cpp20_only_range::iterator>);
#  endif

TEST(ranges_test, join_sentinel) {
  auto hello = zstring{"hello"};
  EXPECT_EQ(fmt::format("{}", hello), "['h', 'e', 'l', 'l', 'o']");
  EXPECT_EQ(fmt::format("{::}", hello), "[h, e, l, l, o]");
  EXPECT_EQ(fmt::format("{}", fmt::join(hello, "_")), "h_e_l_l_o");
}

TEST(ranges_test, join_range) {
  noncopyable_range<int> w(3u, 0);
  EXPECT_EQ(fmt::format("{}", fmt::join(w, ",")), "0,0,0");
  EXPECT_EQ(fmt::format("{}", fmt::join(noncopyable_range<int>(3u, 0), ",")),
            "0,0,0");

  non_const_only_range<int> x(3u, 0);
  EXPECT_EQ(fmt::format("{}", fmt::join(x, ",")), "0,0,0");
  EXPECT_EQ(fmt::format("{}", fmt::join(non_const_only_range<int>(3u, 0), ",")),
            "0,0,0");

  auto y = std::vector<int>(3u, 0);
  EXPECT_EQ(fmt::format("{}", fmt::join(y, ",")), "0,0,0");
  EXPECT_EQ(fmt::format("{}", fmt::join(std::vector<int>(3u, 0), ",")),
            "0,0,0");

  const auto z = std::vector<int>(3u, 0);
  EXPECT_EQ(fmt::format("{}", fmt::join(z, ",")), "0,0,0");

#  ifdef __cpp_lib_ranges
  EXPECT_EQ(fmt::format("{}", cpp20_only_range{.lo = 0, .hi = 5}),
            "[0, 1, 2, 3, 4]");
  EXPECT_EQ(
      fmt::format("{}", fmt::join(cpp20_only_range{.lo = 0, .hi = 5}, ",")),
      "0,1,2,3,4");
#  endif
}
#endif  // FMT_RANGES_TEST_ENABLE_JOIN

TEST(ranges_test, is_printable) {
  using fmt::detail::is_printable;
  EXPECT_TRUE(is_printable(0x0323));
  EXPECT_FALSE(is_printable(0x0378));
  EXPECT_FALSE(is_printable(0x110000));
}

TEST(ranges_test, escape) {
  using vec = std::vector<std::string>;
  EXPECT_EQ(fmt::format("{}", vec{"\n\r\t\"\\"}), "[\"\\n\\r\\t\\\"\\\\\"]");
  EXPECT_EQ(fmt::format("{}", vec{"\x07"}), "[\"\\x07\"]");
  EXPECT_EQ(fmt::format("{}", vec{"\x7f"}), "[\"\\x7f\"]");
  EXPECT_EQ(fmt::format("{}", vec{"n\xcc\x83"}), "[\"n\xcc\x83\"]");

  if (fmt::detail::is_utf8()) {
    EXPECT_EQ(fmt::format("{}", vec{"\xcd\xb8"}), "[\"\\u0378\"]");
    // Unassigned Unicode code points.
    EXPECT_EQ(fmt::format("{}", vec{"\xf0\xaa\x9b\x9e"}), "[\"\\U0002a6de\"]");
    // Broken utf-8.
    EXPECT_EQ(fmt::format("{}", vec{"\xf4\x8f\xbf\xc0"}),
              "[\"\\xf4\\x8f\\xbf\\xc0\"]");
    EXPECT_EQ(fmt::format("{}", vec{"\xf0\x28"}), "[\"\\xf0(\"]");
    EXPECT_EQ(fmt::format("{}", vec{"\xe1\x28"}), "[\"\\xe1(\"]");
    EXPECT_EQ(fmt::format("{}", vec{std::string("\xf0\x28\0\0anything", 12)}),
              "[\"\\xf0(\\x00\\x00anything\"]");

    // Correct utf-8.
    EXPECT_EQ(fmt::format("{}", vec{"🦄"}), "[\"🦄\"]");
  }

  EXPECT_EQ(fmt::format("{}", std::vector<std::vector<char>>{{'x'}}),
            "[['x']]");
  EXPECT_EQ(fmt::format("{}", std::tuple<std::vector<char>>{{'x'}}), "(['x'])");
}

template <typename R> struct fmt_ref_view {
  R* r;

  auto begin() const -> decltype(r->begin()) { return r->begin(); }
  auto end() const -> decltype(r->end()) { return r->end(); }
};

TEST(ranges_test, range_of_range_of_mixed_const) {
  std::vector<std::vector<int>> v = {{1, 2, 3}, {4, 5}};
  EXPECT_EQ(fmt::format("{}", v), "[[1, 2, 3], [4, 5]]");

  fmt_ref_view<decltype(v)> r{&v};
  EXPECT_EQ(fmt::format("{}", r), "[[1, 2, 3], [4, 5]]");
}

TEST(ranges_test, vector_char) {
  EXPECT_EQ(fmt::format("{}", std::vector<char>{'a', 'b'}), "['a', 'b']");
}

TEST(ranges_test, container_adaptor) {
  {
    using fmt::detail::is_container_adaptor_like;
    using T = std::nullptr_t;
    static_assert(is_container_adaptor_like<std::stack<T>>::value, "");
    static_assert(is_container_adaptor_like<std::queue<T>>::value, "");
    static_assert(is_container_adaptor_like<std::priority_queue<T>>::value, "");
    static_assert(!is_container_adaptor_like<std::vector<T>>::value, "");
  }

  {
    auto s = std::stack<int>();
    s.push(1);
    s.push(2);
    EXPECT_EQ(fmt::format("{}", s), "[1, 2]");
    EXPECT_EQ(fmt::format("{}", const_cast<const decltype(s)&>(s)), "[1, 2]");
  }

  {
    auto q = std::queue<int>();
    q.push(1);
    q.push(2);
    EXPECT_EQ(fmt::format("{}", q), "[1, 2]");
  }

  {
    auto q = std::priority_queue<int>();
    q.push(3);
    q.push(1);
    q.push(2);
    q.push(4);
    EXPECT_EQ(fmt::format("{}", q), "[4, 3, 2, 1]");
  }

  {
    auto s = std::stack<char, std::string>();
    s.push('a');
    s.push('b');
    // See https://cplusplus.github.io/LWG/issue3881.
    EXPECT_EQ(fmt::format("{}", s), "['a', 'b']");
  }

  {
    struct my_container_adaptor {
      using value_type = int;
      using container_type = std::vector<value_type>;
      void push(const value_type& v) { c.push_back(v); }

     protected:
      container_type c;
    };

    auto m = my_container_adaptor();
    m.push(1);
    m.push(2);
    EXPECT_EQ(fmt::format("{}", m), "[1, 2]");
  }
}
