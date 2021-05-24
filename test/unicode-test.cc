// Formatting library for C++ - Unicode tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include <vector>

#include "fmt/chrono.h"
#include "gmock/gmock.h"
#include "util.h"  // get_locale

using testing::Contains;

TEST(unicode_test, is_utf8) { EXPECT_TRUE(fmt::detail::is_utf8()); }

TEST(unicode_test, legacy_locale) {
  auto loc = get_locale("ru_RU.CP1251");
  if (loc == std::locale::classic()) return;
  try {
    EXPECT_THAT(
        (std::vector<std::string>{"День недели: пн", "День недели: Пн"}),
        Contains(fmt::format(loc, "День недели: {:L}", fmt::weekday(1))));
  } catch (const fmt::format_error& e) {
    // Formatting can fail due to unsupported encoding.
    fmt::print("Format error: {}\n", e.what());
  }
}
