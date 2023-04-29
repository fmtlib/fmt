// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/compile.h"

#include <type_traits>

#include "fmt/chrono.h"
#include "gmock/gmock.h"
#include "gtest-extra.h"

TEST(iterator_test, counting_iterator) {
  auto it = fmt::detail::counting_iterator();
  auto prev = it++;
  EXPECT_EQ(prev.count(), 0);
  EXPECT_EQ(it.count(), 1);
  EXPECT_EQ((it + 41).count(), 42);
}

TEST(iterator_test, truncating_iterator) {
  char* p = nullptr;
  auto it = fmt::detail::truncating_iterator<char*>(p, 3);
  auto prev = it++;
  EXPECT_EQ(prev.base(), p);
  EXPECT_EQ(it.base(), p + 1);
}

TEST(iterator_test, truncating_iterator_default_construct) {
  auto it = fmt::detail::truncating_iterator<char*>();
  EXPECT_EQ(nullptr, it.base());
  EXPECT_EQ(std::size_t{0}, it.count());
}

#ifdef __cpp_lib_ranges
TEST(iterator_test, truncating_iterator_is_output_iterator) {
  static_assert(
      std::output_iterator<fmt::detail::truncating_iterator<char*>, char>);
}
#endif

TEST(iterator_test, truncating_back_inserter) {
  auto buffer = std::string();
  auto bi = std::back_inserter(buffer);
  auto it = fmt::detail::truncating_iterator<decltype(bi)>(bi, 2);
  *it++ = '4';
  *it++ = '2';
  *it++ = '1';
  EXPECT_EQ(buffer.size(), 2);
  EXPECT_EQ(buffer, "42");
}

TEST(compile_test, compile_fallback) {
  // FMT_COMPILE should fallback on runtime formatting when `if constexpr` is
  // not available.
  EXPECT_EQ("42", fmt::format(FMT_COMPILE("{}"), 42));
}

struct type_with_get {
  template <int> friend void get(type_with_get);
};

FMT_BEGIN_NAMESPACE
template <> struct formatter<type_with_get> : formatter<int> {
  template <typename FormatContext>
  auto format(type_with_get, FormatContext& ctx) -> decltype(ctx.out()) {
    return formatter<int>::format(42, ctx);
  }
};
FMT_END_NAMESPACE

TEST(compile_test, compile_type_with_get) {
  EXPECT_EQ("42", fmt::format(FMT_COMPILE("{}"), type_with_get()));
}

#if defined(__cpp_if_constexpr) && defined(__cpp_return_type_deduction)
struct test_formattable {};

FMT_BEGIN_NAMESPACE
template <> struct formatter<test_formattable> : formatter<const char*> {
  char word_spec = 'f';
  constexpr auto parse(format_parse_context& ctx) {
    auto it = ctx.begin(), end = ctx.end();
    if (it == end || *it == '}') return it;
    if (it != end && (*it == 'f' || *it == 'b')) word_spec = *it++;
    if (it != end && *it != '}') throw format_error("invalid format");
    return it;
  }
  template <typename FormatContext>
  constexpr auto format(test_formattable, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    return formatter<const char*>::format(word_spec == 'f' ? "foo" : "bar",
                                          ctx);
  }
};
FMT_END_NAMESPACE

TEST(compile_test, format_default) {
  EXPECT_EQ("42", fmt::format(FMT_COMPILE("{}"), 42));
  EXPECT_EQ("42", fmt::format(FMT_COMPILE("{}"), 42u));
  EXPECT_EQ("42", fmt::format(FMT_COMPILE("{}"), 42ll));
  EXPECT_EQ("42", fmt::format(FMT_COMPILE("{}"), 42ull));
  EXPECT_EQ("true", fmt::format(FMT_COMPILE("{}"), true));
  EXPECT_EQ("x", fmt::format(FMT_COMPILE("{}"), 'x'));
  EXPECT_EQ("4.2", fmt::format(FMT_COMPILE("{}"), 4.2));
  EXPECT_EQ("foo", fmt::format(FMT_COMPILE("{}"), "foo"));
  EXPECT_EQ("foo", fmt::format(FMT_COMPILE("{}"), std::string("foo")));
  EXPECT_EQ("foo", fmt::format(FMT_COMPILE("{}"), test_formattable()));
  auto t = std::chrono::system_clock::now();
  EXPECT_EQ(fmt::format("{}", t), fmt::format(FMT_COMPILE("{}"), t));
#  ifdef __cpp_lib_byte
  EXPECT_EQ("42", fmt::format(FMT_COMPILE("{}"), std::byte{42}));
#  endif
}

TEST(compile_test, format_wide_string) {
  EXPECT_EQ(L"42", fmt::format(FMT_COMPILE(L"{}"), 42));
}

TEST(compile_test, format_specs) {
  EXPECT_EQ("42", fmt::format(FMT_COMPILE("{:x}"), 0x42));
  EXPECT_EQ("1.2 ms ",
            fmt::format(FMT_COMPILE("{:7.1%Q %q}"),
                        std::chrono::duration<double, std::milli>(1.234)));
}

TEST(compile_test, dynamic_format_specs) {
  EXPECT_EQ("foo  ", fmt::format(FMT_COMPILE("{:{}}"), "foo", 5));
  EXPECT_EQ("  3.14", fmt::format(FMT_COMPILE("{:{}.{}f}"), 3.141592, 6, 2));
  EXPECT_EQ(
      "=1.234ms=",
      fmt::format(FMT_COMPILE("{:=^{}.{}}"),
                  std::chrono::duration<double, std::milli>(1.234), 9, 3));
}

TEST(compile_test, manual_ordering) {
  EXPECT_EQ("42", fmt::format(FMT_COMPILE("{0}"), 42));
  EXPECT_EQ(" -42", fmt::format(FMT_COMPILE("{0:4}"), -42));
  EXPECT_EQ("41 43", fmt::format(FMT_COMPILE("{0} {1}"), 41, 43));
  EXPECT_EQ("41 43", fmt::format(FMT_COMPILE("{1} {0}"), 43, 41));
  EXPECT_EQ("41 43", fmt::format(FMT_COMPILE("{0} {2}"), 41, 42, 43));
  EXPECT_EQ("  41   43", fmt::format(FMT_COMPILE("{1:{2}} {0:4}"), 43, 41, 4));
  EXPECT_EQ("42 1.2 ms ",
            fmt::format(FMT_COMPILE("{0} {1:7.1%Q %q}"), 42,
                        std::chrono::duration<double, std::milli>(1.234)));
  EXPECT_EQ(
      "true 42 42 foo 0x1234 foo",
      fmt::format(FMT_COMPILE("{0} {1} {2} {3} {4} {5}"), true, 42, 42.0f,
                  "foo", reinterpret_cast<void*>(0x1234), test_formattable()));
  EXPECT_EQ(L"42", fmt::format(FMT_COMPILE(L"{0}"), 42));
}

TEST(compile_test, named) {
  auto runtime_named_field_compiled =
      fmt::detail::compile<decltype(fmt::arg("arg", 42))>(FMT_COMPILE("{arg}"));
  static_assert(std::is_same_v<decltype(runtime_named_field_compiled),
                               fmt::detail::runtime_named_field<char>>);

  EXPECT_EQ("42", fmt::format(FMT_COMPILE("{}"), fmt::arg("arg", 42)));
  EXPECT_EQ("41 43", fmt::format(FMT_COMPILE("{} {}"), fmt::arg("arg", 41),
                                 fmt::arg("arg", 43)));

  EXPECT_EQ("foobar",
            fmt::format(FMT_COMPILE("{a0}{a1}"), fmt::arg("a0", "foo"),
                        fmt::arg("a1", "bar")));
  EXPECT_EQ("foobar", fmt::format(FMT_COMPILE("{}{a1}"), fmt::arg("a0", "foo"),
                                  fmt::arg("a1", "bar")));
  EXPECT_EQ("foofoo", fmt::format(FMT_COMPILE("{a0}{}"), fmt::arg("a0", "foo"),
                                  fmt::arg("a1", "bar")));
  EXPECT_EQ("foobar", fmt::format(FMT_COMPILE("{0}{a1}"), fmt::arg("a0", "foo"),
                                  fmt::arg("a1", "bar")));
  EXPECT_EQ("foobar", fmt::format(FMT_COMPILE("{a0}{1}"), fmt::arg("a0", "foo"),
                                  fmt::arg("a1", "bar")));

  EXPECT_EQ("foobar",
            fmt::format(FMT_COMPILE("{}{a1}"), "foo", fmt::arg("a1", "bar")));
  EXPECT_EQ("foobar",
            fmt::format(FMT_COMPILE("{a0}{a1}"), fmt::arg("a1", "bar"),
                        fmt::arg("a2", "baz"), fmt::arg("a0", "foo")));
  EXPECT_EQ(" bar foo ",
            fmt::format(FMT_COMPILE(" {foo} {bar} "), fmt::arg("foo", "bar"),
                        fmt::arg("bar", "foo")));

  EXPECT_THROW(fmt::format(FMT_COMPILE("{invalid}"), fmt::arg("valid", 42)),
               fmt::format_error);

#  if FMT_USE_NONTYPE_TEMPLATE_ARGS
  using namespace fmt::literals;
  auto statically_named_field_compiled =
      fmt::detail::compile<decltype("arg"_a = 42)>(FMT_COMPILE("{arg}"));
  static_assert(std::is_same_v<decltype(statically_named_field_compiled),
                               fmt::detail::field<char, int, 0>>);

  EXPECT_EQ("41 43",
            fmt::format(FMT_COMPILE("{a0} {a1}"), "a0"_a = 41, "a1"_a = 43));
  EXPECT_EQ("41 43",
            fmt::format(FMT_COMPILE("{a1} {a0}"), "a0"_a = 43, "a1"_a = 41));
#  endif
}

TEST(compile_test, join) {
  unsigned char data[] = {0x1, 0x2, 0xaf};
  EXPECT_EQ("0102af", fmt::format(FMT_COMPILE("{:02x}"), fmt::join(data, "")));
}

TEST(compile_test, format_to) {
  char buf[8];
  auto end = fmt::format_to(buf, FMT_COMPILE("{}"), 42);
  *end = '\0';
  EXPECT_STREQ("42", buf);
  end = fmt::format_to(buf, FMT_COMPILE("{:x}"), 42);
  *end = '\0';
  EXPECT_STREQ("2a", buf);
}

TEST(compile_test, format_to_n) {
  constexpr auto buffer_size = 8;
  char buffer[buffer_size];
  auto res = fmt::format_to_n(buffer, buffer_size, FMT_COMPILE("{}"), 42);
  *res.out = '\0';
  EXPECT_STREQ("42", buffer);
  res = fmt::format_to_n(buffer, buffer_size, FMT_COMPILE("{:x}"), 42);
  *res.out = '\0';
  EXPECT_STREQ("2a", buffer);
}

#  ifdef __cpp_lib_bit_cast
TEST(compile_test, constexpr_formatted_size) {
  FMT_CONSTEXPR20 size_t s1 = fmt::formatted_size(FMT_COMPILE("{0}"), 42);
  EXPECT_EQ(2, s1);
  FMT_CONSTEXPR20 size_t s2 =
      fmt::formatted_size(FMT_COMPILE("{0:<4.2f}"), 42.0);
  EXPECT_EQ(5, s2);
}
#  endif

TEST(compile_test, text_and_arg) {
  EXPECT_EQ(">>>42<<<", fmt::format(FMT_COMPILE(">>>{}<<<"), 42));
  EXPECT_EQ("42!", fmt::format(FMT_COMPILE("{}!"), 42));
}

TEST(compile_test, unknown_format_fallback) {
  EXPECT_EQ(" 42 ",
            fmt::format(FMT_COMPILE("{name:^4}"), fmt::arg("name", 42)));

  std::vector<char> v;
  fmt::format_to(std::back_inserter(v), FMT_COMPILE("{name:^4}"),
                 fmt::arg("name", 42));
  EXPECT_EQ(" 42 ", fmt::string_view(v.data(), v.size()));

  char buffer[4];
  auto result = fmt::format_to_n(buffer, 4, FMT_COMPILE("{name:^5}"),
                                 fmt::arg("name", 42));
  EXPECT_EQ(5u, result.size);
  EXPECT_EQ(buffer + 4, result.out);
  EXPECT_EQ(" 42 ", fmt::string_view(buffer, 4));
}

TEST(compile_test, empty) { EXPECT_EQ("", fmt::format(FMT_COMPILE(""))); }

struct to_stringable {
  friend fmt::string_view to_string_view(to_stringable) { return {}; }
};

FMT_BEGIN_NAMESPACE
template <> struct formatter<to_stringable> {
  auto parse(format_parse_context& ctx) const -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const to_stringable&, FormatContext& ctx) -> decltype(ctx.out()) {
    return ctx.out();
  }
};
FMT_END_NAMESPACE

TEST(compile_test, to_string_and_formatter) {
  fmt::format(FMT_COMPILE("{}"), to_stringable());
}

TEST(compile_test, print) {
  EXPECT_WRITE(stdout, fmt::print(FMT_COMPILE("Don't {}!"), "panic"),
               "Don't panic!");
  EXPECT_WRITE(stderr, fmt::print(stderr, FMT_COMPILE("Don't {}!"), "panic"),
               "Don't panic!");
}
#endif

#if FMT_USE_NONTYPE_TEMPLATE_ARGS
TEST(compile_test, compile_format_string_literal) {
  using namespace fmt::literals;
  EXPECT_EQ("", fmt::format(""_cf));
  EXPECT_EQ("42", fmt::format("{}"_cf, 42));
  EXPECT_EQ(L"42", fmt::format(L"{}"_cf, 42));
}
#endif

// MSVS 2019 19.29.30145.0 - Support C++20 and OK.
// MSVS 2022 19.32.31332.0 - compile-test.cc(362,3): fatal error C1001: Internal
// compiler error.
//  (compiler file
//  'D:\a\_work\1\s\src\vctools\Compiler\CxxFE\sl\p1\c\constexpr\constexpr.cpp',
//  line 8635)
#if ((FMT_CPLUSPLUS >= 202002L) &&                           \
     (!defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE > 9) && \
     (!FMT_MSC_VERSION || FMT_MSC_VERSION < 1930)) ||        \
    (FMT_CPLUSPLUS >= 201709L && FMT_GCC_VERSION >= 1002)
template <size_t max_string_length, typename Char = char> struct test_string {
  template <typename T> constexpr bool operator==(const T& rhs) const noexcept {
    return fmt::basic_string_view<Char>(rhs).compare(buffer) == 0;
  }
  Char buffer[max_string_length]{};
};

template <size_t max_string_length, typename Char = char, typename... Args>
consteval inline auto test_format(auto format, const Args&... args) {
  test_string<max_string_length, Char> string{};
  fmt::format_to(string.buffer, format, args...);
  return string;
}

#  if defined(__cpp_lib_constexpr_string) &&   \
      __cpp_lib_constexpr_string >= 201907L && \
      defined(__cpp_lib_constexpr_iterator) && \
      __cpp_lib_constexpr_iterator >= 201811L
#    define TEST_FORMAT(expected, len, str, ...)                            \
      EXPECT_EQ(expected, test_format<len>(FMT_COMPILE(str), __VA_ARGS__)); \
      static_assert(fmt::format(FMT_COMPILE(str), __VA_ARGS__).size() ==    \
                    len - 1);
#  else
#    define TEST_FORMAT(expected, len, str, ...) \
      EXPECT_EQ(expected, test_format<len>(FMT_COMPILE(str), __VA_ARGS__));
#  endif

TEST(compile_time_formatting_test, bool) {
  TEST_FORMAT("true", 5, "{}", true);
  TEST_FORMAT("true", 5, "{}", true);
  TEST_FORMAT("true", 5, "{}", true);
  TEST_FORMAT("false", 6, "{}", false);
  TEST_FORMAT("true ", 6, "{:5}", true);
  TEST_FORMAT("1", 2, "{:d}", true);
}

TEST(compile_time_formatting_test, integer) {
  TEST_FORMAT("42", 3, "{}", 42);
  TEST_FORMAT("420", 4, "{}", 420);
  TEST_FORMAT("42 42", 6, "{} {}", 42, 42);
  TEST_FORMAT("42 42", 6, "{} {}", uint32_t{42}, uint64_t{42});

  TEST_FORMAT("+42", 4, "{:+}", 42);
  TEST_FORMAT("42", 3, "{:-}", 42);
  TEST_FORMAT(" 42", 4, "{: }", 42);

  TEST_FORMAT("-0042", 6, "{:05}", -42);

  TEST_FORMAT("101010", 7, "{:b}", 42);
  TEST_FORMAT("0b101010", 9, "{:#b}", 42);
  TEST_FORMAT("0B101010", 9, "{:#B}", 42);
  TEST_FORMAT("042", 4, "{:#o}", 042);
  TEST_FORMAT("0x4a", 5, "{:#x}", 0x4a);
  TEST_FORMAT("0X4A", 5, "{:#X}", 0x4a);

  TEST_FORMAT("   42", 6, "{:5}", 42);
  TEST_FORMAT("   42", 6, "{:5}", 42ll);
  TEST_FORMAT("   42", 6, "{:5}", 42ull);

  TEST_FORMAT("42  ", 5, "{:<4}", 42);
  TEST_FORMAT("  42", 5, "{:>4}", 42);
  TEST_FORMAT(" 42 ", 5, "{:^4}", 42);
  TEST_FORMAT("**-42", 6, "{:*>5}", -42);
}

TEST(compile_time_formatting_test, char) {
  TEST_FORMAT("c", 2, "{}", 'c');

  TEST_FORMAT("c  ", 4, "{:3}", 'c');
  TEST_FORMAT("99", 3, "{:d}", 'c');
}

TEST(compile_time_formatting_test, string) {
  TEST_FORMAT("42", 3, "{}", "42");
  TEST_FORMAT("The answer is 42", 17, "{} is {}", "The answer", "42");

  TEST_FORMAT("abc**", 6, "{:*<5}", "abc");
  TEST_FORMAT("**🤡**", 9, "{:*^6}", "🤡");
}

TEST(compile_time_formatting_test, combination) {
  TEST_FORMAT("420, true, answer", 18, "{}, {}, {}", 420, true, "answer");

  TEST_FORMAT(" -42", 5, "{:{}}", -42, 4);
}

TEST(compile_time_formatting_test, custom_type) {
  TEST_FORMAT("foo", 4, "{}", test_formattable());
  TEST_FORMAT("bar", 4, "{:b}", test_formattable());
}

TEST(compile_time_formatting_test, multibyte_fill) {
  TEST_FORMAT("жж42", 7, "{:ж>4}", 42);
}
#endif
