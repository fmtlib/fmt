// Formatting library for C++ - Unicode tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/core.h"
#include "gtest/gtest.h"

TEST(unicode_test, is_utf8) { EXPECT_TRUE(fmt::detail::is_utf8()); }