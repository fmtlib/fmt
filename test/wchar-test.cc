// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/wchar.h"

#include "gtest/gtest.h"

TEST(wchar_test, print) {
  // Check that the wide print overload compiles.
  if (fmt::detail::const_check(false)) fmt::print(L"test");
}
