// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/wchar.h"

#include "gtest/gtest.h"

// std::is_constructible is broken in MSVC until version 2015.
#if !FMT_MSC_VER || FMT_MSC_VER >= 1900
struct explicitly_convertible_to_wstring_view {
  explicit operator fmt::wstring_view() const { return L"foo"; }
};

TEST(wchar_test, format_explicitly_convertible_to_wstring_view) {
  EXPECT_EQ(L"foo",
            fmt::format(L"{}", explicitly_convertible_to_wstring_view()));
}
#endif

TEST(wchar_test, vformat_to) {
  using wcontext = fmt::wformat_context;
  fmt::basic_format_arg<wcontext> warg = fmt::detail::make_arg<wcontext>(42);
  auto wargs = fmt::basic_format_args<wcontext>(&warg, 1);
  auto w = std::wstring();
  fmt::vformat_to(std::back_inserter(w), L"{}", wargs);
  EXPECT_EQ(L"42", w);
  w.clear();
  fmt::vformat_to(std::back_inserter(w), FMT_STRING(L"{}"), wargs);
  EXPECT_EQ(L"42", w);
}

TEST(format_test, wide_format_to_n) {
  wchar_t buffer[4];
  buffer[3] = L'x';
  auto result = fmt::format_to_n(buffer, 3, L"{}", 12345);
  EXPECT_EQ(5u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ(L"123x", fmt::wstring_view(buffer, 4));
  buffer[0] = L'x';
  buffer[1] = L'x';
  buffer[2] = L'x';
  result = fmt::format_to_n(buffer, 3, L"{}", L'A');
  EXPECT_EQ(1u, result.size);
  EXPECT_EQ(buffer + 1, result.out);
  EXPECT_EQ(L"Axxx", fmt::wstring_view(buffer, 4));
  result = fmt::format_to_n(buffer, 3, L"{}{} ", L'B', L'C');
  EXPECT_EQ(3u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ(L"BC x", fmt::wstring_view(buffer, 4));
}

#if FMT_USE_USER_DEFINED_LITERALS
TEST(wchar_test, format_udl) {
  using namespace fmt::literals;
  EXPECT_EQ(L"{}c{}"_format(L"ab", 1), fmt::format(L"{}c{}", L"ab", 1));
}

TEST(wchar_test, named_arg_udl) {
  using namespace fmt::literals;
  auto udl_a =
      fmt::format(L"{first}{second}{first}{third}", L"first"_a = L"abra",
                  L"second"_a = L"cad", L"third"_a = 99);
  EXPECT_EQ(
      fmt::format(L"{first}{second}{first}{third}", fmt::arg(L"first", L"abra"),
                  fmt::arg(L"second", L"cad"), fmt::arg(L"third", 99)),
      udl_a);
}
#endif  // FMT_USE_USER_DEFINED_LITERALS

TEST(wchar_test, print) {
  // Check that the wide print overload compiles.
  if (fmt::detail::const_check(false)) fmt::print(L"test");
}

TEST(wchar_test, join) {
  int v[3] = {1, 2, 3};
  EXPECT_EQ(fmt::format(L"({})", fmt::join(v, v + 3, L", ")), L"(1, 2, 3)");
}

TEST(wchar_test, to_wstring) { EXPECT_EQ(L"42", fmt::to_wstring(42)); }
