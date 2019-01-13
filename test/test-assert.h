// Formatting library for C++ - test version of FMT_ASSERT
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_TEST_ASSERT_H_
#define FMT_TEST_ASSERT_H_

#include <stdexcept>
#include "gtest.h"

class assertion_failure : public std::logic_error {
 public:
  explicit assertion_failure(const char* message) : std::logic_error(message) {}
};

#define FMT_ASSERT(condition, message) \
  if (!(condition)) throw assertion_failure(message);

// Expects an assertion failure.
#define EXPECT_ASSERT(stmt, message) \
  FMT_TEST_THROW_(stmt, assertion_failure, message, GTEST_NONFATAL_FAILURE_)

#endif  // FMT_TEST_ASSERT_H_
