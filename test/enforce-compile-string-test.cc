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

#include "fmt/chrono.h"
#include "fmt/color.h"
#include "fmt/format.h"
#include "fmt/locale.h"
#include "fmt/ostream.h"
#include "fmt/ranges.h"

// Exercise the API to verify that everything we expect to can compile.
void test_format_api() {
  (void)fmt::format(FMT_STRING("{}"), 42);
  (void)fmt::format(FMT_STRING(L"{}"), 42);
  (void)fmt::format(FMT_STRING("noop"));

  (void)fmt::to_string(42);
  (void)fmt::to_wstring(42);

  std::list<char> out;
  fmt::format_to(std::back_inserter(out), FMT_STRING("{}"), 42);

  char buffer[4];
  (void)fmt::format_to_n(buffer, 3, FMT_STRING("{}"), 12345);

  wchar_t wbuffer[4];
  (void)fmt::format_to_n(wbuffer, 3, FMT_STRING(L"{}"), 12345);
}

void test_literals_api() {
#if FMT_USE_UDL_TEMPLATE
  using namespace fmt::literals;

  auto udl_format = "{}c{}"_format("ab", 1);
  auto udl_format_w = L"{}c{}"_format(L"ab", 1);
  (void)udl_format;
  (void)udl_format_w;
#endif
}

void test_chrono() {
  (void)fmt::format(FMT_STRING("{}"), std::chrono::seconds(42));
  (void)fmt::format(FMT_STRING(L"{}"), std::chrono::seconds(42));
}

void test_text_style() {
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

void test_zstring() {
  zstring hello{"hello"};
  (void)fmt::format(FMT_STRING("{}"), hello);
}

int main() {
  test_format_api();
  test_literals_api();
  test_chrono();
  test_text_style();
  test_zstring();
}
