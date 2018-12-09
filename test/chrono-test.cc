// Formatting library for C++ - time formatting tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/chrono.h"
#include "gtest.h"

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

TEST(ChronoTest, Format) {
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
  EXPECT_EQ("01/00/00", fmt::format("{:%D}", std::chrono::seconds()));
  EXPECT_EQ("0001-00-00", fmt::format("{:%F}", std::chrono::seconds()));
  EXPECT_EQ("03:25", fmt::format("{:%R}", std::chrono::seconds(12345)));
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
  EXPECT_TIME("%c", time, std::chrono::seconds(12345));
  EXPECT_TIME("%x", time, std::chrono::seconds(12345));
  EXPECT_TIME("%X", time, std::chrono::seconds(12345));
  EXPECT_TIME("%r", time, std::chrono::seconds(12345));
}
