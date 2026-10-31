// Formatting library for C++ - ISO digit grouping with the four-digit
// exception (FMT_ISO_GROUPING_MIN2) tests
//
// Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

// This test is built header-only with FMT_ISO_GROUPING_MIN2=1 so that the
// formatting functions are instantiated with the macro set.
#include "fmt/format.h"
#include "gtest-extra.h"

using fmt::format_error;
using fmt::runtime;

static_assert(FMT_ISO_GROUPING_MIN2, "");

static auto iso(const char* s) -> std::string {
  const char* sep = fmt::detail::is_utf8_enabled != 0 && FMT_UNICODE != 0
                        ? "\xE2\x80\xAF"
                        : " ";
  auto result = std::string();
  for (; *s; ++s) {
    if (*s == '_')
      result += sep;
    else
      result += *s;
  }
  return result;
}

TEST(iso_grouping_min2_test, four_digit_exception) {
  // A part of exactly four digits is not split; each part independently.
  EXPECT_EQ(fmt::format("{:'}", 123), "123");
  EXPECT_EQ(fmt::format("{:'}", 1234), "1234");
  EXPECT_EQ(fmt::format("{:'}", -1234), "-1234");
  EXPECT_EQ(fmt::format("{:'}", 12345), iso("12_345"));
  EXPECT_EQ(fmt::format("{:'}", 1234567), iso("1_234_567"));
  EXPECT_EQ(fmt::format("{:>6'}", 1234), "  1234");
  EXPECT_EQ(fmt::format("{:>8'}", 12345), iso("  12_345"));
  EXPECT_EQ(fmt::format("{:'.4f}", 1234.5678), "1234.5678");
  EXPECT_EQ(fmt::format("{:'.4f}", 0.1234), "0.1234");
  EXPECT_EQ(fmt::format("{:'.5f}", 0.12345), iso("0.123_45"));
  EXPECT_EQ(fmt::format("{:'.4f}", 12345.6789), iso("12_345.6789"));
  EXPECT_EQ(fmt::format("{:'.5f}", 1234.56789), iso("1234.567_89"));
  EXPECT_EQ(fmt::format("{:'.6f}", 1234567.891234), iso("1_234_567.891_234"));
  EXPECT_EQ(fmt::format("{:'.4e}", 123456.789), "1.2346e+05");
  EXPECT_EQ(fmt::format("{:'.5e}", 123456.789), iso("1.234_57e+05"));
  EXPECT_EQ(fmt::format("{:'}", 1234.0), "1234");
  EXPECT_EQ(fmt::format("{:'}", 0.1234), "0.1234");
  EXPECT_EQ(fmt::format(runtime("{:'}"), 1234), "1234");
  // Zero padding: the four-digit exception applies to the padded digits.
  EXPECT_EQ(fmt::format("{:04'}", 1), "0001");
  EXPECT_EQ(fmt::format("{:05'}", 1), iso("00_001"));
  EXPECT_EQ(fmt::format("{:010'}", 1234567), iso("01_234_567"));
  // All forms; the comma forms only group the integer part.
  EXPECT_EQ(fmt::format("{:',}", 1234), "1234");
  EXPECT_EQ(fmt::format("{:',}", 12345), "12,345");
  EXPECT_EQ(fmt::format("{:' .4f}", 1234.5678), "1234.5678");
  EXPECT_EQ(fmt::format("{:' .5f}", 1234.56789), "1234.567 89");
  EXPECT_EQ(fmt::format("{:'_.5f}", 1234.56789), "1234.56789");
  EXPECT_EQ(fmt::format("{:04',}", 1), "0001");
  EXPECT_EQ(fmt::format("{:05''}", 1), "00'001");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:'x}"), 42), format_error,
                   "invalid presentation type for digit grouping");
}
