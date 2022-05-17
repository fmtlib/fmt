// Formatting library for C++ - tests of formatters for standard library types
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/std.h"

#include "gtest/gtest.h"

TEST(std_test, path) {
#ifdef __cpp_lib_filesystem
  EXPECT_EQ(fmt::format("{:8}", std::filesystem::path("foo")), "foo     ");
#endif
}
