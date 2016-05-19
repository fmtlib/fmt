/*
 Tests of string utilities

 Copyright (c) 2012 - 2016, Victor Zverovich
 All rights reserved.

 For the license information refer to format.h.
 */

#include "fmt/string.h"
#include "gtest/gtest.h"

TEST(StringTest, ToString) {
  EXPECT_EQ("42", fmt::to_string(42));
}
