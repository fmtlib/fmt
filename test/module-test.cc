// Formatting library for C++ - module tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.
//
// Copyright (c) 2021 - present, Daniela Engert
// All Rights Reserved
// {fmt} module.

import fmt;

// check for macros leaking from BMI
static bool macro_leaked = 
#if defined(FMT_CORE_H_) || defined(FMT_FORMAT_H)
  true;
#else
  false;
#endif

#include "gtest/gtest.h"

// an implicitly exported namespace must be visible [module.interface]/2.2
TEST(module_test, namespace) {
  using namespace fmt;
  ASSERT_TRUE(true);
}

namespace detail {
bool oops_detail_namespace_is_visible;
}

namespace fmt {
bool namespace_detail_invisible() {
#if defined(FMT_HIDE_MODULE_BUGS) && \
  defined(_MSC_FULL_VER) && _MSC_FULL_VER <= 192930036
  // bug in msvc 16.10-pre4:
  // the namespace is visible even when it is neither
  // implicitly nor explicitly exported
  return true;
#else
  using namespace detail;
  // this fails to compile if fmt::detail is visible
  return !oops_detail_namespace_is_visible;
#endif
}
} // namespace fmt

// the non-exported namespace 'detail' must be invisible [module.interface]/2
TEST(module_test, detail_namespace) {
  EXPECT_TRUE(fmt::namespace_detail_invisible());
}

// macros must not be imported from a *named* module  [cpp.import]/5.1
TEST(module_test, macros) {
#if defined(FMT_HIDE_MODULE_BUGS) && \
  defined(_MSC_FULL_VER) && _MSC_FULL_VER <= 192930036
// bug in msvc 16.10-pre4:
// include-guard macros leak from BMI
// and even worse: they cannot be #undef-ined
  macro_leaked = false;
#endif
  EXPECT_FALSE(macro_leaked);
}
