// Formatting library for C++ - core tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/locale.h"
#include "gmock.h"

template <typename Char>
struct numpunct : std::numpunct<Char> {
 protected:
  Char do_thousands_sep() const FMT_OVERRIDE { return '~'; }
};

TEST(LocaleTest, Format) {
  std::locale loc(std::locale(), new numpunct<char>());
  EXPECT_EQ("1,234,567", fmt::format(std::locale(), "{:n}", 1234567));
  EXPECT_EQ("1~234~567", fmt::format(loc, "{:n}", 1234567));
  fmt::format_arg_store<fmt::format_context, int> as{1234567};
  EXPECT_EQ("1~234~567", fmt::vformat(loc, "{:n}", fmt::format_args(as)));
  std::string s;
  fmt::format_to(std::back_inserter(s), loc, "{:n}", 1234567);
  EXPECT_EQ("1~234~567", s);
}

TEST(LocaleTest, WFormat) {
  std::locale loc(std::locale(), new numpunct<wchar_t>());
  EXPECT_EQ(L"1,234,567", fmt::format(std::locale(), L"{:n}", 1234567));
  EXPECT_EQ(L"1~234~567", fmt::format(loc, L"{:n}", 1234567));
  fmt::format_arg_store<fmt::wformat_context, int> as{1234567};
  EXPECT_EQ(L"1~234~567", fmt::vformat(loc, L"{:n}", fmt::wformat_args(as)));
}
