// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.


#ifdef WIN32
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include "fmt/chrono.h"
#include "fmt/format.h"

#include "gmock.h"
#include "gtest-extra.h"

#ifndef FMT_STATIC_THOUSANDS_SEPARATOR

TEST(ChronoTest, FormatDefault) {
  EXPECT_EQ("42s", fmt::format(FMT_STRING("{}"), std::chrono::seconds(42)));
}

TEST(ChronoTest, FormatWide) {
  EXPECT_EQ(L"42s", fmt::format(FMT_STRING(L"{}"), std::chrono::seconds(42)));
}

#endif  // FMT_STATIC_THOUSANDS_SEPARATOR
