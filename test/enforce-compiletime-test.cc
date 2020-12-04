// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include <array>
#include <iterator>
#include <list>
#include <map>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#ifdef WIN32
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include "fmt/chrono.h"
#include "fmt/color.h"
#include "fmt/format.h"
#include "fmt/locale.h"
#include "fmt/ostream.h"
#include "fmt/ranges.h"

#undef index

#include "gmock.h"

TEST(CompileTimeTest, FormatApi) {
  EXPECT_EQ("42", fmt::format(FMT_STRING("{}"), 42));
  EXPECT_EQ(L"42", fmt::format(FMT_STRING(L"{}"), 42));
  EXPECT_EQ("42", fmt::to_string(42));
  EXPECT_EQ(L"42", fmt::to_wstring(42));

  std::list<char> out;
  fmt::format_to(std::back_inserter(out), FMT_STRING("{}"), 42);
  EXPECT_EQ("42", std::string(out.begin(), out.end()));
  std::stringstream s;
  fmt::format_to(std::ostream_iterator<char>(s), FMT_STRING("{}"), 42);
  EXPECT_EQ("42", s.str());
}

#if FMT_USE_UDL_TEMPLATE
// Passing user-defined literals directly to EXPECT_EQ causes problems
// with macro argument stringification (#) on some versions of GCC.
// Workaround: Assing the UDL result to a variable before the macro.

using namespace fmt::literals;

TEST(CompileTimeTest, Literals) {
  auto udl_format = "{}c{}"_format("ab", 1);
  EXPECT_EQ("abc1", udl_format);
  auto udl_format_w = L"{}c{}"_format(L"ab", 1);
  EXPECT_EQ(L"abc1", udl_format_w);
}
#endif

TEST(CompileTimeTest, FormattedSize) {
  EXPECT_EQ(2u, fmt::formatted_size(FMT_STRING("{}"), 42));
}

TEST(CompileTimeTest, FormatTo) {
  std::vector<char> v;
  fmt::format_to(std::back_inserter(v), FMT_STRING("{}"), "foo");
  EXPECT_EQ(fmt::string_view(v.data(), v.size()), "foo");
}

TEST(CompileTimeTest, FormatToN) {
  char buffer[4];
  buffer[3] = 'x';
  auto result = fmt::format_to_n(buffer, 3, FMT_STRING("{}"), 12345);
  EXPECT_EQ(5u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("123x", fmt::string_view(buffer, 4));
}

TEST(CompileTimeTest, WideFormatToN) {
  wchar_t buffer[4];
  buffer[3] = L'x';
  auto result = fmt::format_to_n(buffer, 3, FMT_STRING(L"{}"), 12345);
  EXPECT_EQ(5u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ(L"123x", fmt::wstring_view(buffer, 4));
  buffer[0] = L'x';
  buffer[1] = L'x';
  buffer[2] = L'x';
  result = fmt::format_to_n(buffer, 3, FMT_STRING(L"{}"), L'A');
  EXPECT_EQ(1u, result.size);
  EXPECT_EQ(buffer + 1, result.out);
  EXPECT_EQ(L"Axxx", fmt::wstring_view(buffer, 4));
  result = fmt::format_to_n(buffer, 3, FMT_STRING(L"{}{} "), L'B', L'C');
  EXPECT_EQ(3u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ(L"BC x", fmt::wstring_view(buffer, 4));
}

struct test_output_iterator {
  char* data;

  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = void;
  using pointer = void;
  using reference = void;

  test_output_iterator& operator++() {
    ++data;
    return *this;
  }
  test_output_iterator operator++(int) {
    auto tmp = *this;
    ++data;
    return tmp;
  }
  char& operator*() { return *data; }
};

TEST(CompileTimeTest, FormatToNOutputIterator) {
  char buf[10] = {};
  fmt::format_to_n(test_output_iterator{buf}, 10, FMT_STRING("{}"), 42);
  EXPECT_STREQ(buf, "42");
}

#ifndef FMT_STATIC_THOUSANDS_SEPARATOR

TEST(CompileTimeTest, Chrono) {
  EXPECT_EQ("42s", fmt::format(FMT_STRING("{}"), std::chrono::seconds(42)));
}

TEST(CompileTimeTest, ChronoWide) {
  EXPECT_EQ(L"42s", fmt::format(FMT_STRING(L"{}"), std::chrono::seconds(42)));
}

#endif  // FMT_STATIC_THOUSANDS_SEPARATOR

TEST(CompileTimeTest, PrintTextStyle) {
  EXPECT_WRITE(
      stdout,
      fmt::print(fg(fmt::rgb(255, 20, 30)), FMT_STRING("{}"), "rgb(255,20,30)"),
      "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m");
}

TEST(CompileTimeTest, FormatTextStyle) {
  EXPECT_EQ("\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m",
            fmt::format(fg(fmt::rgb(255, 20, 30)), FMT_STRING("{}"),
                        "rgb(255,20,30)"));
}

TEST(CompileTimeTest, FormatToOutAcceptsTextStyle) {
  fmt::text_style ts = fg(fmt::rgb(255, 20, 30));
  std::string out;
  fmt::format_to(std::back_inserter(out), ts,
                 FMT_STRING("rgb(255,20,30){}{}{}"), 1, 2, 3);

  EXPECT_EQ(fmt::to_string(out),
            "\x1b[38;2;255;020;030mrgb(255,20,30)123\x1b[0m");
}

struct test {};

// Check if  'if constexpr' is supported.
#if (__cplusplus > 201402L) || \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L && _MSC_VER >= 1910)

struct zstring_sentinel {};

bool operator==(const char* p, zstring_sentinel) { return *p == '\0'; }
bool operator!=(const char* p, zstring_sentinel) { return *p != '\0'; }

struct zstring {
  const char* p;
  const char* begin() const { return p; }
  zstring_sentinel end() const { return {}; }
};

TEST(CompileTimeTest, JoinSentinel) {
  zstring hello{"hello"};
  EXPECT_EQ("{'h', 'e', 'l', 'l', 'o'}", fmt::format(FMT_STRING("{}"), hello));
  EXPECT_EQ("h_e_l_l_o", fmt::format(FMT_STRING("{}"), fmt::join(hello, "_")));
}
