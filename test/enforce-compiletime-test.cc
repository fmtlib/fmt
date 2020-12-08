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

// Exercise the API to verify that everything we expect to can compile.
void TestFormatApi() {
  (void)fmt::format(FMT_STRING("noop"));
  (void)fmt::format(FMT_STRING("{}"), 42);
  (void)fmt::format(FMT_STRING(L"{}"), 42);

  (void)fmt::to_string(42);
  (void)fmt::to_wstring(42);

  std::list<char> out;
  fmt::format_to(std::back_inserter(out), FMT_STRING("{}"), 42);
  std::stringstream s;
  fmt::format_to(std::ostream_iterator<char>(s), FMT_STRING("{}"), 42);

  char buffer[4];
  (void)fmt::format_to_n(buffer, 3, FMT_STRING("{}"), 12345);

  wchar_t wbuffer[4];
  (void)fmt::format_to_n(wbuffer, 3, FMT_STRING(L"{}"), 12345);

  (void)fmt::formatted_size(FMT_STRING("{}"), 42);
}
void TestLiteralsApi() {
#if FMT_USE_UDL_TEMPLATE
  // Passing user-defined literals directly to EXPECT_EQ causes problems
  // with macro argument stringification (#) on some versions of GCC.
  // Workaround: Assing the UDL result to a variable before the macro.

  using namespace fmt::literals;

  auto udl_format = "{}c{}"_format("ab", 1);
  auto udl_format_w = L"{}c{}"_format(L"ab", 1);
  (void)udl_format;
  (void)udl_format_w;
#endif
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

void FormatToNOutputIteratorTest() {
  char buf[10] = {};
  fmt::format_to_n(test_output_iterator{buf}, 10, FMT_STRING("{}"), 42);
}

void TestChrono() {
#ifndef FMT_STATIC_THOUSANDS_SEPARATOR
  (void)fmt::format(FMT_STRING("{}"), std::chrono::seconds(42));
  (void)fmt::format(FMT_STRING(L"{}"), std::chrono::seconds(42));
#endif  // FMT_STATIC_THOUSANDS_SEPARATOR
}

void TestTextStyle() {
  fmt::print(fg(fmt::rgb(255, 20, 30)), FMT_STRING("{}"), "rgb(255,20,30)");
  (void)fmt::format(fg(fmt::rgb(255, 20, 30)), FMT_STRING("{}"),
                    "rgb(255,20,30)");

  fmt::text_style ts = fg(fmt::rgb(255, 20, 30));
  std::string out;
  fmt::format_to(std::back_inserter(out), ts,
                 FMT_STRING("rgb(255,20,30){}{}{}"), 1, 2, 3);
}

struct zstring_sentinel {};

bool operator==(const char* p, zstring_sentinel) { return *p == '\0'; }
bool operator!=(const char* p, zstring_sentinel) { return *p != '\0'; }

struct zstring {
  const char* p;
  const char* begin() const { return p; }
  zstring_sentinel end() const { return {}; }
};

void TestZString() {
  zstring hello{"hello"};
  (void)fmt::format(FMT_STRING("{}"), hello);
  (void)fmt::format(FMT_STRING("{}"), fmt::join(hello, "_"));
}

int main(int argc, char** argv) {
  TestFormatApi();
  TestLiteralsApi();
  FormatToNOutputIteratorTest();
  TestChrono();
  TestTextStyle();
  TestZString();

  return 0;
}
