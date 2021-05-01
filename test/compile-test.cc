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

struct test_formattable {};

FMT_BEGIN_NAMESPACE
template <> struct formatter<test_formattable> : formatter<const char*> {
  template <typename FormatContext>
  auto format(test_formattable, FormatContext& ctx) -> decltype(ctx.out()) {
    return formatter<const char*>::format("foo", ctx);
  }
};
FMT_END_NAMESPACE

TEST(compile_test, compile_fallback) {
  // FMT_COMPILE should fallback on runtime formatting when `if constexpr` is
  // not available.
  EXPECT_EQ("42", fmt::format(FMT_COMPILE("{}"), 42));
}

#ifdef __cpp_if_constexpr
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

#  if FMT_USE_NONTYPE_TEMPLATE_PARAMETERS
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

TEST(compile_test, formatted_size) {
  EXPECT_EQ(2, fmt::formatted_size(FMT_COMPILE("{0}"), 42));
  EXPECT_EQ(5, fmt::formatted_size(FMT_COMPILE("{0:<4.2f}"), 42.0));
}

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
#endif

#if FMT_USE_NONTYPE_TEMPLATE_PARAMETERS
TEST(compile_test, compile_format_string_literal) {
  using namespace fmt::literals;
  EXPECT_EQ("", fmt::format(""_cf));
  EXPECT_EQ("42", fmt::format("{}"_cf, 42));
  EXPECT_EQ(L"42", fmt::format(L"{}"_cf, 42));
}
#endif

#if __cplusplus >= 202002L || \
    (__cplusplus >= 201709L && FMT_GCC_VERSION >= 1002)
template <size_t max_string_length, typename Char = char> struct test_string {
  template <typename T> constexpr bool operator==(const T& rhs) const noexcept {
    return fmt::basic_string_view<Char>(rhs).compare(buffer) == 0;
  }
  Char buffer[max_string_length]{};
};

template <size_t max_string_length, typename Char = char, typename... Args>
consteval auto test_format(auto format, const Args&... args) {
  test_string<max_string_length, Char> string{};
  fmt::format_to(string.buffer, format, args...);
  return string;
}

TEST(compile_time_formatting_test, bool) {
  EXPECT_EQ("true", test_format<5>(FMT_COMPILE("{}"), true));
  EXPECT_EQ("false", test_format<6>(FMT_COMPILE("{}"), false));
  EXPECT_EQ("true ", test_format<6>(FMT_COMPILE("{:5}"), true));
  EXPECT_EQ("1", test_format<2>(FMT_COMPILE("{:d}"), true));
}

TEST(compile_time_formatting_test, integer) {
  EXPECT_EQ("42", test_format<3>(FMT_COMPILE("{}"), 42));
  EXPECT_EQ("420", test_format<4>(FMT_COMPILE("{}"), 420));
  EXPECT_EQ("42 42", test_format<6>(FMT_COMPILE("{} {}"), 42, 42));
  EXPECT_EQ("42 42",
            test_format<6>(FMT_COMPILE("{} {}"), uint32_t{42}, uint64_t{42}));

  EXPECT_EQ("+42", test_format<4>(FMT_COMPILE("{:+}"), 42));
  EXPECT_EQ("42", test_format<3>(FMT_COMPILE("{:-}"), 42));
  EXPECT_EQ(" 42", test_format<4>(FMT_COMPILE("{: }"), 42));

  EXPECT_EQ("-0042", test_format<6>(FMT_COMPILE("{:05}"), -42));

  EXPECT_EQ("101010", test_format<7>(FMT_COMPILE("{:b}"), 42));
  EXPECT_EQ("0b101010", test_format<9>(FMT_COMPILE("{:#b}"), 42));
  EXPECT_EQ("0B101010", test_format<9>(FMT_COMPILE("{:#B}"), 42));
  EXPECT_EQ("042", test_format<4>(FMT_COMPILE("{:#o}"), 042));
  EXPECT_EQ("0x4a", test_format<5>(FMT_COMPILE("{:#x}"), 0x4a));
  EXPECT_EQ("0X4A", test_format<5>(FMT_COMPILE("{:#X}"), 0x4a));

  EXPECT_EQ("   42", test_format<6>(FMT_COMPILE("{:5}"), 42));
  EXPECT_EQ("   42", test_format<6>(FMT_COMPILE("{:5}"), 42ll));
  EXPECT_EQ("   42", test_format<6>(FMT_COMPILE("{:5}"), 42ull));

  EXPECT_EQ("42  ", test_format<5>(FMT_COMPILE("{:<4}"), 42));
  EXPECT_EQ("  42", test_format<5>(FMT_COMPILE("{:>4}"), 42));
  EXPECT_EQ(" 42 ", test_format<5>(FMT_COMPILE("{:^4}"), 42));
  EXPECT_EQ("**-42", test_format<6>(FMT_COMPILE("{:*>5}"), -42));
}

TEST(compile_time_formatting_test, char) {
  EXPECT_EQ("c", test_format<2>(FMT_COMPILE("{}"), 'c'));

  EXPECT_EQ("c  ", test_format<4>(FMT_COMPILE("{:3}"), 'c'));
  EXPECT_EQ("99", test_format<3>(FMT_COMPILE("{:d}"), 'c'));
}

TEST(compile_time_formatting_test, string) {
  EXPECT_EQ("42", test_format<3>(FMT_COMPILE("{}"), "42"));
  EXPECT_EQ("The answer is 42",
            test_format<17>(FMT_COMPILE("{} is {}"), "The answer", "42"));

  EXPECT_EQ("abc**", test_format<6>(FMT_COMPILE("{:*<5}"), "abc"));
  EXPECT_EQ("**🤡**", test_format<9>(FMT_COMPILE("{:*^6}"), "🤡"));
}

TEST(compile_time_formatting_test, combination) {
  EXPECT_EQ("420, true, answer",
            test_format<18>(FMT_COMPILE("{}, {}, {}"), 420, true, "answer"));

  EXPECT_EQ(" -42", test_format<5>(FMT_COMPILE("{:{}}"), -42, 4));
}

TEST(compile_time_formatting_test, multibyte_fill) {
  EXPECT_EQ("жж42", test_format<8>(FMT_COMPILE("{:ж>4}"), 42));
}
#endif
