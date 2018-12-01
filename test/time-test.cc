// Formatting library for C++ - time formatting tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "gmock.h"
#include "fmt/locale.h"
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
  std::time_t t = std::time(FMT_NULL);
  fmt::format(s, *std::localtime(&t));
}

TEST(TimeTest, EmptyResult) {
  EXPECT_EQ("", fmt::format("{}", std::tm()));
}

static bool EqualTime(const std::tm &lhs, const std::tm &rhs) {
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
  std::time_t t = std::time(FMT_NULL);
  std::tm tm = *std::localtime(&t);
  EXPECT_TRUE(EqualTime(tm, fmt::localtime(t)));
}

TEST(TimeTest, GMTime) {
  std::time_t t = std::time(FMT_NULL);
  std::tm tm = *std::gmtime(&t);
  EXPECT_TRUE(EqualTime(tm, fmt::gmtime(t)));
}

#ifdef __cpp_lib_chrono
TEST(TimeTest, Chrono) {
  EXPECT_EQ("00", fmt::format("{:%S}", std::chrono::seconds(0)));
  EXPECT_EQ("00", fmt::format("{:%S}", std::chrono::seconds(60)));
  EXPECT_EQ("42", fmt::format("{:%S}", std::chrono::seconds(42)));
  EXPECT_EQ("01.234", fmt::format("{:%S}", std::chrono::milliseconds(1234)));
  EXPECT_EQ("00", fmt::format("{:%M}", std::chrono::minutes(0)));
  EXPECT_EQ("00", fmt::format("{:%M}", std::chrono::minutes(60)));
  EXPECT_EQ("42", fmt::format("{:%M}", std::chrono::minutes(42)));
  EXPECT_EQ("01", fmt::format("{:%M}", std::chrono::seconds(61)));
  EXPECT_EQ("00", fmt::format("{:%H}", std::chrono::hours(0)));
  EXPECT_EQ("00", fmt::format("{:%H}", std::chrono::hours(24)));
  EXPECT_EQ("14", fmt::format("{:%H}", std::chrono::hours(14)));
  EXPECT_EQ("01", fmt::format("{:%H}", std::chrono::minutes(61)));
  EXPECT_EQ("12", fmt::format("{:%I}", std::chrono::hours(0)));
  EXPECT_EQ("12", fmt::format("{:%I}", std::chrono::hours(12)));
  EXPECT_EQ("12", fmt::format("{:%I}", std::chrono::hours(24)));
  EXPECT_EQ("04", fmt::format("{:%I}", std::chrono::hours(4)));
  EXPECT_EQ("02", fmt::format("{:%I}", std::chrono::hours(14)));
  EXPECT_EQ("03:25:45",
            fmt::format("{:%H:%M:%S}", std::chrono::seconds(12345)));
}

TEST(TimeTest, ChronoLocale) {
  const char *loc_name = "ja_JP.utf8";
  bool has_locale = false;
  std::locale loc;
  try {
    loc = std::locale(loc_name);
    has_locale = true;
  } catch (const std::runtime_error &) {}
  if (!has_locale) {
    fmt::print("{} locale is missing.\n", loc_name);
    return;
  }
  std::ostringstream os;
  auto str = [&] {
    auto s = os.str();
    os.str("");
    return s;
  };
  os.imbue(loc);
  auto time = std::tm();
  time.tm_hour = 14;
  os << std::put_time(&time, "%OH");
  EXPECT_EQ(str(), fmt::format(loc, "{:%OH}", std::chrono::hours(14)));
  os << std::put_time(&time, "%OI");
  EXPECT_EQ(str(), fmt::format(loc, "{:%OI}", std::chrono::hours(14)));
}
#endif
