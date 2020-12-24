// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include <array>
#include <chrono>
#include <iterator>
#include <list>
#include <string>

#include "fmt/chrono.h"
#include "fmt/color.h"
#include "fmt/format.h"
#include "fmt/locale.h"
#include "fmt/ostream.h"
#include "fmt/ranges.h"

// Exercise the API to verify that everything we expect to can compile.
void test_format_api() {
  fmt::format(FMT_STRING("{}"), 42);
  fmt::format(FMT_STRING(L"{}"), 42);
  fmt::format(FMT_STRING("noop"));

  fmt::to_string(42);
  fmt::to_wstring(42);

  std::list<char> out;
  fmt::format_to(std::back_inserter(out), FMT_STRING("{}"), 42);

  char buffer[4];
  fmt::format_to_n(buffer, 3, FMT_STRING("{}"), 12345);

  wchar_t wbuffer[4];
  fmt::format_to_n(wbuffer, 3, FMT_STRING(L"{}"), 12345);
}

void test_literals_api() {
#if FMT_USE_UDL_TEMPLATE
  using namespace fmt::literals;
  "{}c{}"_format("ab", 1);
  L"{}c{}"_format(L"ab", 1);
#endif
}

void test_chrono() {
  fmt::format(FMT_STRING("{}"), std::chrono::seconds(42));
  fmt::format(FMT_STRING(L"{}"), std::chrono::seconds(42));
}

void test_text_style() {
  fmt::print(fg(fmt::rgb(255, 20, 30)), FMT_STRING("{}"), "rgb(255,20,30)");
  fmt::format(fg(fmt::rgb(255, 20, 30)), FMT_STRING("{}"), "rgb(255,20,30)");

  fmt::text_style ts = fg(fmt::rgb(255, 20, 30));
  std::string out;
  fmt::format_to(std::back_inserter(out), ts,
                 FMT_STRING("rgb(255,20,30){}{}{}"), 1, 2, 3);
}

void test_range() {
  std::array<char, 5> hello = {'h', 'e', 'l', 'l', 'o'};
  fmt::format(FMT_STRING("{}"), hello);
}

int main() {
  test_format_api();
  test_literals_api();
  test_chrono();
  test_text_style();
  test_range();
}
