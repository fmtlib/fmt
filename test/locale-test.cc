// Formatting library for C++ - core tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/locale.h"
#include "gmock.h"

struct numpunct : std::numpunct<char> {
 protected:
  char do_thousands_sep() const FMT_OVERRIDE { return '~'; }
};

TEST(LocaleTest, Format) {
  std::locale loc;
  EXPECT_EQ("1~234~567",
            fmt::format(std::locale(loc, new numpunct()), "{:n}", 1234567));
  EXPECT_EQ("1,234,567", fmt::format(loc, "{:n}", 1234567));
}
