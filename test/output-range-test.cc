// Formatting library for C++ - std::ostream support tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/format.h"

#if FMT_OUTPUT_RANGES

#  include <array>
#  include <deque>
#  include <list>
#  include <span>
#  include <vector>

#  include "gmock/gmock.h"
#  include "gtest-extra.h"
#  include "util.h"

TEST(output_range_c_array_char_test, format_into) {
  char buffer[4];
  buffer[3] = 'x';
  auto result = fmt::format_into(buffer, "{}", "abc");
  EXPECT_EQ(buffer + 3, result.begin());
  EXPECT_EQ("abc", fmt::string_view(buffer, 3));
  EXPECT_EQ("abcx", fmt::string_view(buffer, 4));
  result = fmt::format_into(buffer, "x{}y", "abc");
  EXPECT_EQ(buffer + 4, result.begin());
  EXPECT_EQ("xabc", fmt::string_view(buffer, 4));
}

TEST(output_range_array_char_test, format_into) {
  std::array<char, 4> buffer;
  buffer[3] = 'x';
  auto result = fmt::format_into(buffer, "{}", "abc");
  EXPECT_EQ(buffer.begin() + 3, result.begin());
  EXPECT_EQ("abc", fmt::string_view(buffer.data(), 3));
  EXPECT_EQ("abcx", fmt::string_view(buffer.data(), 4));
  result = fmt::format_into(buffer, "x{}y", "abc");
  EXPECT_EQ(buffer.begin() + 4, result.begin());
  EXPECT_EQ("xabc", fmt::string_view(buffer.data(), 4));
}

TEST(output_range_span_array_char_test, format_into) {
  std::array<char, 4> storage;
  std::span<char> buffer(storage);
  buffer[3] = 'x';
  auto result = fmt::format_into(buffer, "{}", "abc");
  EXPECT_EQ(buffer.begin() + 3, result.begin());
  EXPECT_EQ("abc", fmt::string_view(buffer.data(), 3));
  EXPECT_EQ("abcx", fmt::string_view(buffer.data(), 4));
  result = fmt::format_into(buffer, "x{}y", "abc");
  EXPECT_EQ(buffer.begin() + 4, result.begin());
  EXPECT_EQ("xabc", fmt::string_view(buffer.data(), 4));
}

TEST(output_range_vector_char_test, format_into) {
  std::vector<char> buffer(4, '\0');
  buffer[3] = 'x';
  auto result = fmt::format_into(buffer, "{}", "abc");
  EXPECT_EQ(buffer.begin() + 3, result.begin());
  EXPECT_EQ("abc", fmt::string_view(buffer.data(), 3));
  EXPECT_EQ("abcx", fmt::string_view(buffer.data(), 4));
  result = fmt::format_into(buffer, "x{}y", "abc");
  EXPECT_EQ(buffer.begin() + 4, result.begin());
  EXPECT_EQ("xabc", fmt::string_view(buffer.data(), 4));
}

TEST(output_range_list_char_test, format_into) {
  std::list<char> buffer(4, '\0');
  using subrng = std::ranges::subrange<typename decltype(buffer)::iterator>;
  auto fourth_it = std::ranges::next(buffer.begin(), 3);
  auto end = buffer.end();
  subrng first3_subrange(buffer.begin(), fourth_it);
  *fourth_it = 'x';
  auto result = fmt::format_into(buffer, "{}", "abc");
  EXPECT_EQ(fourth_it, result.begin());
  EXPECT_TRUE(std::ranges::equal(fmt::string_view("abc"), first3_subrange));
  EXPECT_TRUE(std::ranges::equal(fmt::string_view("abcx"), buffer));
  result = fmt::format_into(buffer, "x{}y", "abc");
  EXPECT_EQ(end, result.begin());
  EXPECT_TRUE(std::ranges::equal(fmt::string_view("xabc"), buffer));
}

TEST(output_range_deque_char_test, format_into) {
  std::deque<char> buffer(4, '\0');
  using subrng = std::ranges::subrange<typename decltype(buffer)::iterator>;
  auto fourth_it = std::ranges::next(buffer.begin(), 3);
  auto end = buffer.end();
  subrng first3_subrange(buffer.begin(), fourth_it);
  *fourth_it = 'x';
  auto result = fmt::format_into(buffer, "{}", "abc");
  EXPECT_EQ(fourth_it, result.begin());
  EXPECT_TRUE(std::ranges::equal(fmt::string_view("abc"), first3_subrange));
  EXPECT_TRUE(std::ranges::equal(fmt::string_view("abcx"), buffer));
  result = fmt::format_into(buffer, "x{}y", "abc");
  EXPECT_EQ(end, result.begin());
  EXPECT_TRUE(std::ranges::equal(fmt::string_view("xabc"), buffer));
}

TEST(output_range_c_array_char_test, format_to) {
  char buffer[4];
  buffer[3] = 'x';
  auto result = fmt::format_to(buffer, "{}", "abc");
  EXPECT_EQ(buffer + 3, result.begin());
  EXPECT_EQ("abc", fmt::string_view(buffer, 3));
  EXPECT_EQ("abcx", fmt::string_view(buffer, 4));
  result = fmt::format_to(buffer, "x{}y", "abc");
  EXPECT_EQ(buffer + 4, result.begin());
  EXPECT_EQ("xabc", fmt::string_view(buffer, 4));
}

TEST(output_range_array_char_test, format_to) {
  std::array<char, 4> buffer;
  buffer[3] = 'x';
  auto result = fmt::format_to(buffer, "{}", "abc");
  EXPECT_EQ(buffer.begin() + 3, result.begin());
  EXPECT_EQ("abc", fmt::string_view(buffer.data(), 3));
  EXPECT_EQ("abcx", fmt::string_view(buffer.data(), 4));
  result = fmt::format_to(buffer, "x{}y", "abc");
  EXPECT_EQ(buffer.begin() + 4, result.begin());
  EXPECT_EQ("xabc", fmt::string_view(buffer.data(), 4));
}

TEST(output_range_span_array_char_test, format_to) {
  std::array<char, 4> storage;
  std::span<char> buffer(storage);
  buffer[3] = 'x';
  auto result = fmt::format_to(buffer, "{}", "abc");
  EXPECT_EQ(buffer.begin() + 3, result.begin());
  EXPECT_EQ("abc", fmt::string_view(buffer.data(), 3));
  EXPECT_EQ("abcx", fmt::string_view(buffer.data(), 4));
  result = fmt::format_to(buffer, "x{}y", "abc");
  EXPECT_EQ(buffer.begin() + 4, result.begin());
  EXPECT_EQ("xabc", fmt::string_view(buffer.data(), 4));
}

TEST(output_range_vector_char_test, format_to) {
  std::vector<char> buffer{};
  auto result = fmt::format_to(buffer, "{}", "abc");
  EXPECT_EQ(buffer.begin() + 3, result.begin());
  EXPECT_EQ("abc", fmt::string_view(buffer.data(), 3));
  result = fmt::format_to(buffer, "x{}y", "abc");
  EXPECT_EQ(buffer.begin() + 8, result.begin());
  EXPECT_EQ(buffer.begin() + (std::ptrdiff_t)buffer.size(), result.begin());
  EXPECT_EQ(buffer.end(), result.begin());
  EXPECT_EQ("abcxabcy", fmt::string_view(buffer.data(), 8));
}

TEST(output_range_basic_string_char_test, format_to) {
  std::string buffer{};
  auto result = fmt::format_to(buffer, "{}", "abc");
  EXPECT_EQ(buffer.begin() + 3, result.begin());
  EXPECT_EQ("abc", buffer);
  result = fmt::format_to(buffer, "x{}y", "abc");
  EXPECT_EQ(buffer.begin() + 8, result.begin());
  EXPECT_EQ(buffer.begin() + (std::ptrdiff_t)buffer.size(), result.begin());
  EXPECT_EQ(buffer.end(), result.begin());
  EXPECT_EQ("abcxabcy", buffer);
}

TEST(output_range_list_char_test, format_to) {
  std::list<char> buffer{};
  auto result = fmt::format_to(buffer, "{}", "abc");
  EXPECT_EQ(buffer.end(), result.begin());
  EXPECT_EQ(std::ranges::next(buffer.begin(), 3), result.begin());
  EXPECT_EQ(std::ranges::next(buffer.begin(), (std::ptrdiff_t)buffer.size()),
            result.begin());
  EXPECT_TRUE(std::ranges::equal(fmt::string_view("abc"), buffer));
  result = fmt::format_to(buffer, "x{}y", "abc");
  EXPECT_EQ(buffer.end(), result.begin());
  EXPECT_EQ(std::ranges::next(buffer.begin(), 8), result.begin());
  EXPECT_EQ(std::ranges::next(buffer.begin(), (std::ptrdiff_t)buffer.size()),
            result.begin());
  EXPECT_TRUE(std::ranges::equal(fmt::string_view("abcxabcy"), buffer));
}

TEST(output_range_deque_char_test, format_to) {
  std::deque<char> buffer{};
  auto result = fmt::format_to(buffer, "{}", "abc");
  EXPECT_EQ(buffer.end(), result.begin());
  EXPECT_EQ(std::ranges::next(buffer.begin(), 3), result.begin());
  EXPECT_EQ(std::ranges::next(buffer.begin(), (std::ptrdiff_t)buffer.size()),
            result.begin());
  EXPECT_TRUE(std::ranges::equal(fmt::string_view("abc"), buffer));
  result = fmt::format_to(buffer, "x{}y", "abc");
  EXPECT_EQ(buffer.end(), result.begin());
  EXPECT_EQ(std::ranges::next(buffer.begin(), 8), result.begin());
  EXPECT_EQ(std::ranges::next(buffer.begin(), (std::ptrdiff_t)buffer.size()),
            result.begin());
  EXPECT_TRUE(std::ranges::equal(fmt::string_view("abcxabcy"), buffer));
}

#endif
