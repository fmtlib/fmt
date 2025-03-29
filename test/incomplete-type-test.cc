// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/format.h"
#include "fmt/color.h"
#include "gtest/gtest.h"

// Only defined after all the tests.
struct incomplete_type;
extern const incomplete_type& external_instance;

FMT_BEGIN_NAMESPACE

template <> struct formatter<incomplete_type> : formatter<int> {
  auto format(const incomplete_type& x, context& ctx) const
      -> decltype(ctx.out());
};

FMT_END_NAMESPACE

TEST(incomplete_type_test, format) {
  EXPECT_EQ(fmt::format("{}", external_instance), fmt::format("{}", 42));
  EXPECT_EQ(fmt::format("{:4}", external_instance), fmt::format("{:4}", 42));
  EXPECT_EQ(fmt::format("{:4}", fmt::styled(external_instance, fg(fmt::color::red))), fmt::format("{:4}", fmt::styled(42, fg(fmt::color::red))));
}

struct incomplete_type {
  int i;
};

const incomplete_type& external_instance{42};

auto fmt::formatter<incomplete_type>::format(const incomplete_type& x,
                                             fmt::context& ctx) const
    -> decltype(ctx.out()) {
  return fmt::formatter<int>::format(x.i, ctx);
}
