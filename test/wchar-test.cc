// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/wchar.h"

#include "gtest/gtest.h"

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
