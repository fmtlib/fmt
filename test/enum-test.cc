// Formatting library for C++ - enum formatting tests
//
// Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/enum.h"

#include <vector>

#include "fmt/ranges.h"
#include "gtest/gtest.h"

#if !FMT_USE_REFLECTION
TEST(enum_test, no_reflection) {
  fmt::print("Reflection is not supported.\n");
}
#else

// clang-format doesn't support annotations yet.
// clang-format off
enum class [[=fmt::as_identifiers]] color { red, green, blue };
enum class color_without_annotation { red, green, blue };
enum [[=fmt::as_identifiers]] unscoped_color { unscoped_red, unscoped_green };
enum class [[=fmt::as_identifiers]] level : unsigned char { low = 1, high = 2 };
enum class [[=fmt::as_identifiers]] byte_enum : char { one = 1 };
enum class [[=fmt::as_identifiers]] signed_byte_enum : signed char { minus_one = -1 };
enum class [[=fmt::as_identifiers]] bool_enum : bool { off = false };
enum class [[=fmt::as_identifiers]] alias { one = 1, uno = 1 };
enum class [[=fmt::as_identifiers]] empty_enum {};
// clang-format on

TEST(enum_test, format_enum) {
  EXPECT_EQ(fmt::format("{}", color::red), "red");
  EXPECT_EQ(fmt::format("{}", color::green), "green");
  EXPECT_EQ(fmt::format("{}", color::blue), "blue");
}

TEST(enum_test, format_unscoped_enum) {
  EXPECT_EQ(fmt::format("{}", unscoped_green), "unscoped_green");
}

TEST(enum_test, format_enum_with_underlying_type) {
  EXPECT_EQ(fmt::format("{}", level::high), "high");
  EXPECT_EQ(fmt::format("{}", byte_enum::one), "one");
  EXPECT_EQ(fmt::format("{}", signed_byte_enum::minus_one), "minus_one");
  EXPECT_EQ(fmt::format("{}", bool_enum::off), "off");
}

TEST(enum_test, format_enum_alias) {
  // The first enumerator with a matching value is used.
  EXPECT_EQ(fmt::format("{}", alias::uno), "one");
}

TEST(enum_test, format_unknown_value) {
  EXPECT_EQ(fmt::format("{}", static_cast<color>(42)), "42");
  EXPECT_EQ(fmt::format("{}", static_cast<level>(42)), "42");
  EXPECT_EQ(fmt::format("{}", static_cast<empty_enum>(0)), "0");
}

TEST(enum_test, format_unknown_char_value) {
  // A char underlying type is written in decimal, not as a character.
  EXPECT_EQ(fmt::format("{}", static_cast<byte_enum>(65)), "65");
  EXPECT_EQ(fmt::format("{}", static_cast<signed_byte_enum>(-65)), "-65");
  EXPECT_EQ(fmt::format("{}", static_cast<level>(65)), "65");
  // A bool underlying type is written as 0 or 1, not as "true"/"false".
  EXPECT_EQ(fmt::format("{}", static_cast<bool_enum>(true)), "1");
}

TEST(enum_test, format_enum_specs) {
  EXPECT_EQ(fmt::format("{:>7}", color::red), "    red");
  EXPECT_EQ(fmt::format("{:*^7}", color::red), "**red**");
  EXPECT_EQ(fmt::format("{:.2}", color::green), "gr");
  EXPECT_EQ(fmt::format("{:>4}", static_cast<color>(42)), "  42");
}

TEST(enum_test, format_enum_range) {
  auto v = std::vector<color>{color::red, color::blue};
  EXPECT_EQ(fmt::format("{}", v), "[red, blue]");
}

TEST(enum_test, annotation_is_required) {
  EXPECT_TRUE(fmt::is_formattable<color>::value);
  EXPECT_FALSE(fmt::is_formattable<color_without_annotation>::value);
}
#endif  // FMT_USE_REFLECTION
