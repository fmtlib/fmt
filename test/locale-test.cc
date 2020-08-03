// Formatting library for C++ - locale tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/locale.h"

#include "gmock.h"

using fmt::detail::max_value;

#ifndef FMT_STATIC_THOUSANDS_SEPARATOR
template <typename Char> struct numpunct : std::numpunct<Char> {
 protected:
  Char do_decimal_point() const FMT_OVERRIDE { return '?'; }
  std::string do_grouping() const FMT_OVERRIDE { return "\03"; }
  Char do_thousands_sep() const FMT_OVERRIDE { return '~'; }
};

template <typename Char> struct no_grouping : std::numpunct<Char> {
 protected:
  Char do_decimal_point() const FMT_OVERRIDE { return '.'; }
  std::string do_grouping() const FMT_OVERRIDE { return ""; }
  Char do_thousands_sep() const FMT_OVERRIDE { return ','; }
};

template <typename Char> struct special_grouping : std::numpunct<Char> {
 protected:
  Char do_decimal_point() const FMT_OVERRIDE { return '.'; }
  std::string do_grouping() const FMT_OVERRIDE { return "\03\02"; }
  Char do_thousands_sep() const FMT_OVERRIDE { return ','; }
};

template <typename Char> struct small_grouping : std::numpunct<Char> {
 protected:
  Char do_decimal_point() const FMT_OVERRIDE { return '.'; }
  std::string do_grouping() const FMT_OVERRIDE { return "\01"; }
  Char do_thousands_sep() const FMT_OVERRIDE { return ','; }
};

TEST(LocaleTest, DoubleDecimalPoint) {
  std::locale loc(std::locale(), new numpunct<char>());
  EXPECT_EQ("1?23", fmt::format(loc, "{:L}", 1.23));
}

TEST(LocaleTest, Format) {
  std::locale loc(std::locale(), new numpunct<char>());
  EXPECT_EQ("1234567", fmt::format(std::locale(), "{:L}", 1234567));
  EXPECT_EQ("1~234~567", fmt::format(loc, "{:L}", 1234567));
  EXPECT_EQ("-1~234~567", fmt::format(loc, "{:L}", -1234567));
  fmt::format_arg_store<fmt::format_context, int> as{1234567};
  EXPECT_EQ("1~234~567", fmt::vformat(loc, "{:L}", fmt::format_args(as)));
  std::string s;
  fmt::format_to(std::back_inserter(s), loc, "{:L}", 1234567);
  EXPECT_EQ("1~234~567", s);

  std::locale no_grouping_loc(std::locale(), new no_grouping<char>());
  EXPECT_EQ("1234567", fmt::format(no_grouping_loc, "{:L}", 1234567));

  std::locale special_grouping_loc(std::locale(), new special_grouping<char>());
  EXPECT_EQ("1,23,45,678", fmt::format(special_grouping_loc, "{:L}", 12345678));
  EXPECT_EQ("12,345", fmt::format(special_grouping_loc, "{:L}", 12345));

  std::locale small_grouping_loc(std::locale(), new small_grouping<char>());
  EXPECT_EQ("4,2,9,4,9,6,7,2,9,5",
            fmt::format(small_grouping_loc, "{:L}", max_value<uint32_t>()));
}

TEST(LocaleTest, FormatDetaultAlign) {
  std::locale special_grouping_loc(std::locale(), new special_grouping<char>());
  EXPECT_EQ("  12,345", fmt::format(special_grouping_loc, "{:8L}", 12345));
}

TEST(LocaleTest, WFormat) {
  std::locale loc(std::locale(), new numpunct<wchar_t>());
  EXPECT_EQ(L"1234567", fmt::format(std::locale(), L"{:L}", 1234567));
  EXPECT_EQ(L"1~234~567", fmt::format(loc, L"{:L}", 1234567));
  fmt::format_arg_store<fmt::wformat_context, int> as{1234567};
  EXPECT_EQ(L"1~234~567", fmt::vformat(loc, L"{:L}", fmt::wformat_args(as)));
  EXPECT_EQ(L"1234567", fmt::format(std::locale("C"), L"{:L}", 1234567));

  std::locale no_grouping_loc(std::locale(), new no_grouping<wchar_t>());
  EXPECT_EQ(L"1234567", fmt::format(no_grouping_loc, L"{:L}", 1234567));

  std::locale special_grouping_loc(std::locale(),
                                   new special_grouping<wchar_t>());
  EXPECT_EQ(L"1,23,45,678",
            fmt::format(special_grouping_loc, L"{:L}", 12345678));

  std::locale small_grouping_loc(std::locale(), new small_grouping<wchar_t>());
  EXPECT_EQ(L"4,2,9,4,9,6,7,2,9,5",
            fmt::format(small_grouping_loc, L"{:L}", max_value<uint32_t>()));
}

TEST(LocaleTest, DoubleFormatter) {
  auto loc = std::locale(std::locale(), new special_grouping<char>());
  auto f = fmt::formatter<int>();
  auto parse_ctx = fmt::format_parse_context("L");
  f.parse(parse_ctx);
  char buf[10] = {};
  fmt::basic_format_context<char*, char> format_ctx(
      buf, {}, fmt::detail::locale_ref(loc));
  *f.format(12345, format_ctx) = 0;
  EXPECT_STREQ("12,345", buf);
}

#endif  // FMT_STATIC_THOUSANDS_SEPARATOR
