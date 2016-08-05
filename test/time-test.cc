/*
 Time formatting tests

 Copyright (c) 2012 - 2016, Victor Zverovich
 All rights reserved.

 For the license information refer to format.h.
 */

#include "gmock/gmock.h"
#include "fmt/time.h"

TEST(TimeTest, Format) {
  std::tm tm = std::tm();
  tm.tm_year = 116;
  tm.tm_mon  = 3;
  tm.tm_mday = 25;
  EXPECT_EQ("The date is 2016-04-25.",
            fmt::format("The date is {:%Y-%m-%d}.", tm));
}

TEST(TimeTest, GrowBuffer) {
  std::string s = "{:";
  for (int i = 0; i < 30; ++i)
    s += "%c";
  s += "}\n";
  std::time_t t = std::time(0);
  fmt::format(s, *std::localtime(&t));
}

TEST(TimeTest, EmptyResult) {
  EXPECT_EQ("", fmt::format("{}", std::tm()));
}
