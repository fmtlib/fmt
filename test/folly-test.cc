// Formatting library for C++ - folly::StringPiece formatter tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include <fmt/folly.h>
#include "gtest.h"

TEST(FollyTest, FormatStringPiece) {
  EXPECT_EQ(fmt::format("{}", "foo"), "foo");
  EXPECT_EQ(fmt::format("{:>5}", "foo"), "  foo");
}
