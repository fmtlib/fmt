// Formatting library for C++ - Unicode tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include <iomanip>
#include <locale>
#include <vector>

#include "fmt/chrono.h"
#include "gmock/gmock.h"
#include "util.h"  // get_locale

using testing::Contains;

TEST(unicode_test, is_utf8) { EXPECT_TRUE(fmt::detail::is_utf8()); }

TEST(unicode_test, legacy_locale) {
  auto loc = get_locale("ru_RU.CP1251", "Russian_Russia.1251");
  if (loc == std::locale::classic()) return;

  auto s = std::string();
  try {
    s = fmt::format(loc, "День недели: {:L}", fmt::weekday(1));
  } catch (const fmt::format_error& e) {
    // Formatting can fail due to an unsupported encoding.
    fmt::print("Format error: {}\n", e.what());
    return;
  }

#if !FMT_GCC_VERSION || FMT_GCC_VERSION >= 500
  auto&& os = std::ostringstream();
  os.imbue(loc);
  auto tm = std::tm();
  tm.tm_wday = 1;
  os << std::put_time(&tm, "%a");
  auto wd = os.str();
  if (wd == "??") {
    EXPECT_EQ(s, "День недели: ??");
    fmt::print("std::locale gives ?? as a weekday.\n");
    return;
  }
#endif
  EXPECT_THAT((std::vector<std::string>{"День недели: пн", "День недели: Пн"}),
              Contains(s));
}
