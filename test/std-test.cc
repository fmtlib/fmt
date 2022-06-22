// Formatting library for C++ - tests of formatters for standard library types
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/std.h"

#include <string>

#include "gtest/gtest.h"

TEST(std_test, path) {
#ifdef __cpp_lib_filesystem
  EXPECT_EQ(fmt::format("{:8}", std::filesystem::path("foo")), "\"foo\"   ");
  EXPECT_EQ(fmt::format("{}", std::filesystem::path("foo\"bar.txt")),
            "\"foo\\\"bar.txt\"");

#  ifdef _WIN32
  // File.txt in Russian.
  const wchar_t unicode_path[] = {0x424, 0x430, 0x439, 0x43b, 0x2e,
                                  0x74,  0x78,  0x74,  0};
  const char unicode_u8path[] = {'"',        char(0xd0), char(0xa4), char(0xd0),
                                 char(0xb0), char(0xd0), char(0xb9), char(0xd0),
                                 char(0xbb), '.',        't',        'x',
                                 't',        '"',        '\0'};
  EXPECT_EQ(fmt::format("{}", std::filesystem::path(unicode_path)),
            unicode_u8path);
#  endif
#endif
}

TEST(std_test, thread_id) {
  EXPECT_FALSE(fmt::format("{}", std::this_thread::get_id()).empty());
}

TEST(std_test, variant) {
#ifdef __cpp_lib_variant
  EXPECT_EQ(fmt::format("{}", std::monostate{}), " ");
  using V0 = std::variant<int, float, std::string, char>;
  V0 v0(42);
  V0 v1(1.5f);
  V0 v2("hello");
  V0 v3('i');
  EXPECT_EQ(fmt::format("{}", v0), "<42>");
  EXPECT_EQ(fmt::format("{}", v1), "<1.5>");
  EXPECT_EQ(fmt::format("{}", v2), "<\"hello\">");
  EXPECT_EQ(fmt::format("{}", v3), "<'i'>");

  struct unformattable {};
  EXPECT_FALSE((fmt::is_formattable<unformattable>::value));
  EXPECT_FALSE((fmt::is_formattable<std::variant<unformattable>>::value));
  EXPECT_FALSE((fmt::is_formattable<std::variant<unformattable, int>>::value));
  EXPECT_FALSE((fmt::is_formattable<std::variant<int, unformattable>>::value));
  EXPECT_FALSE(
      (fmt::is_formattable<std::variant<unformattable, unformattable>>::value));
  EXPECT_TRUE((fmt::is_formattable<std::variant<int, float>>::value));

  using V1 = std::variant<std::monostate, std::string, std::string>;
  V1 v4{};
  V1 v5{std::in_place_index<1>, "yes, this is variant"};

  EXPECT_EQ(fmt::format("{}", v4), "< >");
  EXPECT_EQ(fmt::format("{}", v5), "<\"yes, this is variant\">");
#endif
}
