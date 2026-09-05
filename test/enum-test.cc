// Formatting library for C++ - enum formatting tests
//
// Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/enum.h"

#include <climits>
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
enum class [[=fmt::as_identifiers()]] color { red, green, blue };
enum class color_without_annotation { red, green, blue };
enum [[=fmt::as_identifiers()]] unscoped_color { unscoped_red, unscoped_green };
enum class [[=fmt::as_identifiers()]] level : unsigned char {
  low = 1,
  high = 2
};
enum class [[=fmt::as_identifiers()]] byte_enum : char { one = 1 };
enum class [[=fmt::as_identifiers()]] signed_byte_enum : signed char {
  minus_one = -1
};
enum class [[=fmt::as_identifiers()]] bool_enum : bool { off = false };
enum class [[=fmt::as_identifiers()]] alias { one = 1, uno = 1 };
enum class [[=fmt::as_identifiers()]] empty_enum {};

// Dense values: formatted via a lookup table.
enum class [[=fmt::as_identifiers()]] dense { d0, d1, d2, d3, d4 };
// 3 holes out of 10: the sparsest case that still uses a lookup table.
enum class [[=fmt::as_identifiers()]] holey {
  h0, h1, h2, h3, h4, h5, h6 = 9
};
// 4 holes out of 11: just too sparse, formatted via a hash table.
enum class [[=fmt::as_identifiers()]] sparse {
  s0, s1, s2, s3, s4, s5, s6 = 10
};
// Values spanning both signs and the extremes of the underlying type.
enum class [[=fmt::as_identifiers()]] signed_enum {
  minus_two = -2,
  minus_one = -1,
  one = 1
};
// Many scattered values, exercising collisions in the hash table.
enum class [[=fmt::as_identifiers()]] scattered {
  a = 1, b = 17, c = 33, d = 49, e = 65, f = 81,
  g = 97, h = 113, i = 129, j = 145, k = 161, l = 177
};
// Values that collide in the hash table: c0, c7 and c15 share a slot, and c6
// occupies the next one, so probing for c7 and c15 has to step over it.
enum class [[=fmt::as_identifiers()]] collision {
  c0 = 0, c6 = 6, c7 = 7, c15 = 15
};
// Values that collide in the last slot of the hash table, so the probe
// sequence wraps around to the beginning.
enum class [[=fmt::as_identifiers()]] wrapping_collision {
  w8 = 8, w16 = 16, w24 = 24
};
// Aliased values in an enum that is too sparse for a lookup table.
enum class [[=fmt::as_identifiers()]] sparse_alias {
  one = 1, dup = 1, far = 1000
};
enum class [[=fmt::as_identifiers()]] extremes : int {
  lowest = INT_MIN,
  highest = INT_MAX
};
enum class [[=fmt::as_identifiers()]] big : unsigned long long {
  huge = ULLONG_MAX
};
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

TEST(enum_test, format_dense_enum) {
  // A dense enum is formatted using a lookup table.
  static_assert(fmt::detail::identifier_table_size<dense>() == 5);
  EXPECT_EQ(fmt::format("{}", dense::d0), "d0");
  EXPECT_EQ(fmt::format("{}", dense::d4), "d4");
  EXPECT_EQ(fmt::format("{}", static_cast<dense>(5)), "5");
}

TEST(enum_test, format_holey_enum) {
  // Up to 30% of holes are still formatted using a lookup table.
  static_assert(fmt::detail::identifier_table_size<holey>() == 10);
  EXPECT_EQ(fmt::format("{}", holey::h0), "h0");
  EXPECT_EQ(fmt::format("{}", holey::h5), "h5");
  EXPECT_EQ(fmt::format("{}", holey::h6), "h6");
  // Values in the holes and outside of the table fall back to the number.
  EXPECT_EQ(fmt::format("{}", static_cast<holey>(6)), "6");
  EXPECT_EQ(fmt::format("{}", static_cast<holey>(8)), "8");
  EXPECT_EQ(fmt::format("{}", static_cast<holey>(10)), "10");
  EXPECT_EQ(fmt::format("{}", static_cast<holey>(-1)), "-1");
}

TEST(enum_test, format_sparse_enum) {
  // One more hole than holey, which is too many for a lookup table.
  static_assert(fmt::detail::identifier_table_size<sparse>() == 0);
  // 7 enumerators need 16 slots to keep the load factor at or below 0.5.
  static_assert(fmt::detail::identifier_map_size<sparse>() == 16);
  EXPECT_EQ(fmt::format("{}", sparse::s0), "s0");
  EXPECT_EQ(fmt::format("{}", sparse::s5), "s5");
  EXPECT_EQ(fmt::format("{}", sparse::s6), "s6");
  EXPECT_EQ(fmt::format("{}", static_cast<sparse>(6)), "6");
  EXPECT_EQ(fmt::format("{}", static_cast<sparse>(11)), "11");
}

TEST(enum_test, format_enum_with_negative_values) {
  static_assert(fmt::detail::identifier_table_size<signed_enum>() == 4);
  EXPECT_EQ(fmt::format("{}", signed_enum::minus_two), "minus_two");
  EXPECT_EQ(fmt::format("{}", signed_enum::minus_one), "minus_one");
  EXPECT_EQ(fmt::format("{}", signed_enum::one), "one");
  // A hole and values below and above the range of the table.
  EXPECT_EQ(fmt::format("{}", static_cast<signed_enum>(0)), "0");
  EXPECT_EQ(fmt::format("{}", static_cast<signed_enum>(-3)), "-3");
  EXPECT_EQ(fmt::format("{}", static_cast<signed_enum>(2)), "2");
}

TEST(enum_test, format_scattered_enum) {
  static_assert(fmt::detail::identifier_table_size<scattered>() == 0);
  static_assert(fmt::detail::identifier_map_size<scattered>() == 32);
  EXPECT_EQ(fmt::format("{}", scattered::a), "a");
  EXPECT_EQ(fmt::format("{}", scattered::b), "b");
  EXPECT_EQ(fmt::format("{}", scattered::c), "c");
  EXPECT_EQ(fmt::format("{}", scattered::d), "d");
  EXPECT_EQ(fmt::format("{}", scattered::e), "e");
  EXPECT_EQ(fmt::format("{}", scattered::f), "f");
  EXPECT_EQ(fmt::format("{}", scattered::g), "g");
  EXPECT_EQ(fmt::format("{}", scattered::h), "h");
  EXPECT_EQ(fmt::format("{}", scattered::i), "i");
  EXPECT_EQ(fmt::format("{}", scattered::j), "j");
  EXPECT_EQ(fmt::format("{}", scattered::k), "k");
  EXPECT_EQ(fmt::format("{}", scattered::l), "l");
  EXPECT_EQ(fmt::format("{}", static_cast<scattered>(0)), "0");
  EXPECT_EQ(fmt::format("{}", static_cast<scattered>(-1)), "-1");
  EXPECT_EQ(fmt::format("{}", static_cast<scattered>(999)), "999");
}

TEST(enum_test, format_enum_with_hash_collision) {
  static_assert(fmt::detail::identifier_table_size<collision>() == 0);
  static_assert(fmt::detail::identifier_map_size<collision>() == 8);
  // Three of the four values want the same slot and the fourth takes the slot
  // next to it, filling the table to its maximum load factor of 0.5.
  static_assert(fmt::detail::identifier_slot(collision::c0) ==
                fmt::detail::identifier_slot(collision::c7));
  static_assert(fmt::detail::identifier_slot(collision::c0) ==
                fmt::detail::identifier_slot(collision::c15));
  static_assert(fmt::detail::identifier_slot(collision::c6) !=
                fmt::detail::identifier_slot(collision::c0));
  EXPECT_EQ(fmt::format("{}", collision::c0), "c0");
  EXPECT_EQ(fmt::format("{}", collision::c6), "c6");
  EXPECT_EQ(fmt::format("{}", collision::c7), "c7");
  EXPECT_EQ(fmt::format("{}", collision::c15), "c15");
  // A value that collides with the enumerators but doesn't match any of them
  // is rejected after probing the whole chain.
  static_assert(fmt::detail::identifier_slot(static_cast<collision>(23)) ==
                fmt::detail::identifier_slot(collision::c0));
  EXPECT_EQ(fmt::format("{}", static_cast<collision>(23)), "23");
}

TEST(enum_test, format_enum_with_wrapping_hash_collision) {
  static_assert(fmt::detail::identifier_table_size<wrapping_collision>() == 0);
  static_assert(fmt::detail::identifier_map_size<wrapping_collision>() == 8);
  // All the values want the last slot, so the probe sequence wraps around.
  static_assert(fmt::detail::identifier_slot(wrapping_collision::w8) == 7);
  static_assert(fmt::detail::identifier_slot(wrapping_collision::w16) == 7);
  static_assert(fmt::detail::identifier_slot(wrapping_collision::w24) == 7);
  EXPECT_EQ(fmt::format("{}", wrapping_collision::w8), "w8");
  EXPECT_EQ(fmt::format("{}", wrapping_collision::w16), "w16");
  EXPECT_EQ(fmt::format("{}", wrapping_collision::w24), "w24");
  EXPECT_EQ(fmt::format("{}", static_cast<wrapping_collision>(33)), "33");
}

TEST(enum_test, format_sparse_enum_alias) {
  // The first enumerator with a matching value is used in the hash table too.
  static_assert(fmt::detail::identifier_table_size<sparse_alias>() == 0);
  EXPECT_EQ(fmt::format("{}", sparse_alias::dup), "one");
  EXPECT_EQ(fmt::format("{}", sparse_alias::far), "far");
}

TEST(enum_test, format_enum_with_extreme_values) {
  // The span of the values overflows the underlying type, so no table is used.
  static_assert(fmt::detail::identifier_table_size<extremes>() == 0);
  EXPECT_EQ(fmt::format("{}", extremes::lowest), "lowest");
  EXPECT_EQ(fmt::format("{}", extremes::highest), "highest");
  EXPECT_EQ(fmt::format("{}", static_cast<extremes>(0)), "0");

  static_assert(fmt::detail::identifier_table_size<big>() == 1);
  EXPECT_EQ(fmt::format("{}", big::huge), "huge");
  EXPECT_EQ(fmt::format("{}", static_cast<big>(0)), "0");
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
