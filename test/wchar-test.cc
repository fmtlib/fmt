// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/wchar.h"

#include "gtest/gtest.h"

TEST(format_test, vformat_to) {
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

#if FMT_USE_USER_DEFINED_LITERALS
TEST(format_test, format_udl) {
  using namespace fmt::literals;
  EXPECT_EQ(L"{}c{}"_format(L"ab", 1), fmt::format(L"{}c{}", L"ab", 1));
}
#endif

TEST(wchar_test, print) {
  // Check that the wide print overload compiles.
  if (fmt::detail::const_check(false)) fmt::print(L"test");
}
