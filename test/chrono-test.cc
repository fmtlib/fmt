// Formatting library for C++ - time formatting tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/chrono.h"
#include "gtest-extra.h"

#include <iomanip>

std::tm make_tm() {
  auto time = std::tm();
  time.tm_mday = 1;
  return time;
}

std::tm make_hour(int h) {
  auto time = make_tm();
  time.tm_hour = h;
  return time;
}

std::tm make_minute(int m) {
  auto time = make_tm();
  time.tm_min = m;
  return time;
}

std::tm make_second(int s) {
  auto time = make_tm();
  time.tm_sec = s;
  return time;
}

std::string format_tm(const std::tm &time, const char *spec,
                      const std::locale &loc) {
  auto &facet = std::use_facet<std::time_put<char>>(loc);
  std::ostringstream os;
  os.imbue(loc);
  facet.put(os, os, ' ', &time, spec, spec + std::strlen(spec));
  return os.str();
}

#define EXPECT_TIME(spec, time, duration) { \
    std::locale loc("ja_JP.utf8"); \
    EXPECT_EQ(format_tm(time, spec, loc), \
              fmt::format(loc, "{:" spec "}", duration)); \
  }

TEST(ChronoTest, FormatDefault) {
  EXPECT_EQ("42s", fmt::format("{}", std::chrono::seconds(42)));
  EXPECT_EQ("42as",
            fmt::format("{}", std::chrono::duration<int, std::atto>(42)));
  EXPECT_EQ("42fs",
            fmt::format("{}", std::chrono::duration<int, std::femto>(42)));
  EXPECT_EQ("42ps",
            fmt::format("{}", std::chrono::duration<int, std::pico>(42)));
  EXPECT_EQ("42ns", fmt::format("{}", std::chrono::nanoseconds(42)));
  EXPECT_EQ("42µs", fmt::format("{}", std::chrono::microseconds(42)));
  EXPECT_EQ("42ms", fmt::format("{}", std::chrono::milliseconds(42)));
  EXPECT_EQ("42cs",
            fmt::format("{}", std::chrono::duration<int, std::centi>(42)));
  EXPECT_EQ("42ds",
            fmt::format("{}", std::chrono::duration<int, std::deci>(42)));
  EXPECT_EQ("42s", fmt::format("{}", std::chrono::seconds(42)));
  EXPECT_EQ("42das",
            fmt::format("{}", std::chrono::duration<int, std::deca>(42)));
  EXPECT_EQ("42hs",
            fmt::format("{}", std::chrono::duration<int, std::hecto>(42)));
  EXPECT_EQ("42ks",
            fmt::format("{}", std::chrono::duration<int, std::kilo>(42)));
  EXPECT_EQ("42Ms",
            fmt::format("{}", std::chrono::duration<int, std::mega>(42)));
  EXPECT_EQ("42Gs",
            fmt::format("{}", std::chrono::duration<int, std::giga>(42)));
  EXPECT_EQ("42Ts",
            fmt::format("{}", std::chrono::duration<int, std::tera>(42)));
  EXPECT_EQ("42Ps",
            fmt::format("{}", std::chrono::duration<int, std::peta>(42)));
  EXPECT_EQ("42Es",
            fmt::format("{}", std::chrono::duration<int, std::exa>(42)));
  EXPECT_EQ("42m", fmt::format("{}", std::chrono::minutes(42)));
  EXPECT_EQ("42h", fmt::format("{}", std::chrono::hours(42)));
  EXPECT_EQ("42[15]s",
            fmt::format("{}",
                        std::chrono::duration<int, std::ratio<15, 1>>(42)));
  EXPECT_EQ("42[15/4]s",
            fmt::format("{}",
                        std::chrono::duration<int, std::ratio<15, 4>>(42)));
}

TEST(ChronoTest, Align) {
  auto s = std::chrono::seconds(42);
  EXPECT_EQ("42s  ", fmt::format("{:5}", s));
  EXPECT_EQ("42s  ", fmt::format("{:{}}", s, 5));
  EXPECT_EQ("  42s", fmt::format("{:>5}", s));
  EXPECT_EQ("**42s**", fmt::format("{:*^7}", s));
  EXPECT_EQ("03:25:45    ",
            fmt::format("{:12%H:%M:%S}", std::chrono::seconds(12345)));
  EXPECT_EQ("    03:25:45",
            fmt::format("{:>12%H:%M:%S}", std::chrono::seconds(12345)));
  EXPECT_EQ("~~03:25:45~~",
            fmt::format("{:~^12%H:%M:%S}", std::chrono::seconds(12345)));

}

TEST(ChronoTest, FormatSpecs) {
  EXPECT_EQ("%", fmt::format("{:%%}", std::chrono::seconds(0)));
  EXPECT_EQ("\n", fmt::format("{:%n}", std::chrono::seconds(0)));
  EXPECT_EQ("\t", fmt::format("{:%t}", std::chrono::seconds(0)));
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
  EXPECT_EQ("03:25", fmt::format("{:%R}", std::chrono::seconds(12345)));
  EXPECT_EQ("03:25:45", fmt::format("{:%T}", std::chrono::seconds(12345)));
}

TEST(ChronoTest, InvalidSpecs) {
  auto sec = std::chrono::seconds(0);
  EXPECT_THROW_MSG(fmt::format("{:%a}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format("{:%A}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format("{:%c}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format("{:%x}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format("{:%Ex}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format("{:%X}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format("{:%EX}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format("{:%D}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format("{:%F}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format("{:%Ec}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format("{:%w}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format("{:%u}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format("{:%b}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format("{:%B}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format("{:%z}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format("{:%Z}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format("{:%q}", sec), fmt::format_error,
                   "invalid format");
  EXPECT_THROW_MSG(fmt::format("{:%Eq}", sec), fmt::format_error,
                   "invalid format");
  EXPECT_THROW_MSG(fmt::format("{:%Oq}", sec), fmt::format_error,
                   "invalid format");
}

TEST(ChronoTest, Locale) {
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
  EXPECT_TIME("%OH", make_hour(14), std::chrono::hours(14));
  EXPECT_TIME("%OI", make_hour(14), std::chrono::hours(14));
  EXPECT_TIME("%OM", make_minute(42), std::chrono::minutes(42));
  EXPECT_TIME("%OS", make_second(42), std::chrono::seconds(42));
  auto time = make_tm();
  time.tm_hour = 3;
  time.tm_min = 25;
  time.tm_sec = 45;
  auto sec = std::chrono::seconds(12345);
  EXPECT_TIME("%r", time, sec);
  EXPECT_TIME("%p", time, sec);
}
