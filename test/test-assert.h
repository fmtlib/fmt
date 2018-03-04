// Formatting library for C++ - test version of FMT_ASSERT
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_TEST_ASSERT_H
#define FMT_TEST_ASSERT_H

#include <stdexcept>

class AssertionFailure : public std::logic_error {
 public:
  explicit AssertionFailure(const char *message) : std::logic_error(message) {}
};

#define FMT_ASSERT(condition, message) \
  if (!(condition)) throw AssertionFailure(message);

#include "gtest-extra.h"

// Expects an assertion failure.
#define EXPECT_ASSERT(stmt, message) \
  EXPECT_THROW_MSG(stmt, AssertionFailure, message)

#endif  // FMT_TEST_ASSERT_H
