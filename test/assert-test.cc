// Formatting library for C++ - assertion tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/core.h"
#include "gtest.h"

#if GTEST_HAS_DEATH_TEST
#  define EXPECT_DEBUG_DEATH_IF_SUPPORTED(statement, regex) \
    EXPECT_DEBUG_DEATH(statement, regex)
#else
#  define EXPECT_DEBUG_DEATH_IF_SUPPORTED(statement, regex) \
    GTEST_UNSUPPORTED_DEATH_TEST_(statement, regex, )
#endif

TEST(AssertTest, Fail) {
  EXPECT_DEBUG_DEATH_IF_SUPPORTED(FMT_ASSERT(false, "don't panic!"),
                                  "don't panic!");
}

bool test_condition = false;

TEST(AssertTest, DanglingElse) {
  bool executed_else = false;
  if (test_condition)
    FMT_ASSERT(true, "");
  else
    executed_else = true;
  EXPECT_TRUE(executed_else);
}
