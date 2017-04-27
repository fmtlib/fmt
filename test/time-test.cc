/*
 Time formatting tests

 Copyright (c) 2012 - 2016, Victor Zverovich
 All rights reserved.

 For the license information refer to format.h.
 */
#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

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

bool EqualTime(const std::tm &lhs, const std::tm &rhs) {
  return lhs.tm_sec == rhs.tm_sec &&
         lhs.tm_min == rhs.tm_min &&
         lhs.tm_hour == rhs.tm_hour &&
         lhs.tm_mday == rhs.tm_mday &&
         lhs.tm_mon == rhs.tm_mon &&
         lhs.tm_year == rhs.tm_year &&
         lhs.tm_wday == rhs.tm_wday &&
         lhs.tm_yday == rhs.tm_yday &&
         lhs.tm_isdst == rhs.tm_isdst;
}

TEST(TimeTest, LocalTime) {
  std::time_t t = std::time(0);
  std::tm tm = *std::localtime(&t);
  EXPECT_TRUE(EqualTime(tm, fmt::localtime(t)));
}

TEST(TimeTest, GMTime) {
  std::time_t t = std::time(0);
  std::tm tm = *std::gmtime(&t);
  EXPECT_TRUE(EqualTime(tm, fmt::gmtime(t)));
}
