// Formatting library for C++ - time formatting tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifdef WIN32
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include "fmt/chrono.h"

#include <iomanip>

#include "gtest-extra.h"

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

std::string format_tm(const std::tm& time, const char* spec,
                      const std::locale& loc) {
  auto& facet = std::use_facet<std::time_put<char>>(loc);
  std::ostringstream os;
  os.imbue(loc);
  facet.put(os, os, ' ', &time, spec, spec + std::strlen(spec));
  return os.str();
}

TEST(TimeTest, Format) {
  std::tm tm = std::tm();
  tm.tm_year = 116;
  tm.tm_mon = 3;
  tm.tm_mday = 25;
  EXPECT_EQ("The date is 2016-04-25.",
            fmt::format("The date is {:%Y-%m-%d}.", tm));
}

TEST(TimeTest, GrowBuffer) {
  std::string s = "{:";
  for (int i = 0; i < 30; ++i) s += "%c";
  s += "}\n";
  std::time_t t = std::time(nullptr);
  fmt::format(s, *std::localtime(&t));
}

TEST(TimeTest, FormatToEmptyContainer) {
  std::string s;
  auto time = std::tm();
  time.tm_sec = 42;
  fmt::format_to(std::back_inserter(s), "{:%S}", time);
  EXPECT_EQ(s, "42");
}

TEST(TimeTest, EmptyResult) { EXPECT_EQ("", fmt::format("{}", std::tm())); }

static bool EqualTime(const std::tm& lhs, const std::tm& rhs) {
  return lhs.tm_sec == rhs.tm_sec && lhs.tm_min == rhs.tm_min &&
         lhs.tm_hour == rhs.tm_hour && lhs.tm_mday == rhs.tm_mday &&
         lhs.tm_mon == rhs.tm_mon && lhs.tm_year == rhs.tm_year &&
         lhs.tm_wday == rhs.tm_wday && lhs.tm_yday == rhs.tm_yday &&
         lhs.tm_isdst == rhs.tm_isdst;
}

TEST(TimeTest, LocalTime) {
  std::time_t t = std::time(nullptr);
  std::tm tm = *std::localtime(&t);
  EXPECT_TRUE(EqualTime(tm, fmt::localtime(t)));
}

TEST(TimeTest, GMTime) {
  std::time_t t = std::time(nullptr);
  std::tm tm = *std::gmtime(&t);
  EXPECT_TRUE(EqualTime(tm, fmt::gmtime(t)));
}

TEST(TimeTest, TimePoint) {
  std::chrono::system_clock::time_point point =
      std::chrono::system_clock::now();

  std::time_t t = std::chrono::system_clock::to_time_t(point);
  std::tm tm = *std::localtime(&t);
  char strftime_output[256];
  std::strftime(strftime_output, sizeof(strftime_output),
                "It is %Y-%m-%d %H:%M:%S", &tm);

  EXPECT_EQ(strftime_output, fmt::format("It is {:%Y-%m-%d %H:%M:%S}", point));
}

#define EXPECT_TIME(spec, time, duration)                 \
  {                                                       \
    std::locale jp_loc("ja_JP.utf8");                     \
    EXPECT_EQ(format_tm(time, spec, jp_loc),              \
              fmt::format(loc, "{:" spec "}", duration)); \
  }

#ifndef FMT_STATIC_THOUSANDS_SEPARATOR

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
  EXPECT_EQ(
      "42[15]s",
      fmt::format("{}", std::chrono::duration<int, std::ratio<15, 1>>(42)));
  EXPECT_EQ(
      "42[15/4]s",
      fmt::format("{}", std::chrono::duration<int, std::ratio<15, 4>>(42)));
}

TEST(ChronoTest, FormatWide) {
  EXPECT_EQ(L"42s", fmt::format(L"{}", std::chrono::seconds(42)));
  EXPECT_EQ(L"42as",
            fmt::format(L"{}", std::chrono::duration<int, std::atto>(42)));
  EXPECT_EQ(L"42fs",
            fmt::format(L"{}", std::chrono::duration<int, std::femto>(42)));
  EXPECT_EQ(L"42ps",
            fmt::format(L"{}", std::chrono::duration<int, std::pico>(42)));
  EXPECT_EQ(L"42ns", fmt::format(L"{}", std::chrono::nanoseconds(42)));
  EXPECT_EQ(L"42\u00B5s", fmt::format(L"{}", std::chrono::microseconds(42)));
  EXPECT_EQ(L"42ms", fmt::format(L"{}", std::chrono::milliseconds(42)));
  EXPECT_EQ(L"42cs",
            fmt::format(L"{}", std::chrono::duration<int, std::centi>(42)));
  EXPECT_EQ(L"42ds",
            fmt::format(L"{}", std::chrono::duration<int, std::deci>(42)));
  EXPECT_EQ(L"42s", fmt::format(L"{}", std::chrono::seconds(42)));
  EXPECT_EQ(L"42das",
            fmt::format(L"{}", std::chrono::duration<int, std::deca>(42)));
  EXPECT_EQ(L"42hs",
            fmt::format(L"{}", std::chrono::duration<int, std::hecto>(42)));
  EXPECT_EQ(L"42ks",
            fmt::format(L"{}", std::chrono::duration<int, std::kilo>(42)));
  EXPECT_EQ(L"42Ms",
            fmt::format(L"{}", std::chrono::duration<int, std::mega>(42)));
  EXPECT_EQ(L"42Gs",
            fmt::format(L"{}", std::chrono::duration<int, std::giga>(42)));
  EXPECT_EQ(L"42Ts",
            fmt::format(L"{}", std::chrono::duration<int, std::tera>(42)));
  EXPECT_EQ(L"42Ps",
            fmt::format(L"{}", std::chrono::duration<int, std::peta>(42)));
  EXPECT_EQ(L"42Es",
            fmt::format(L"{}", std::chrono::duration<int, std::exa>(42)));
  EXPECT_EQ(L"42m", fmt::format(L"{}", std::chrono::minutes(42)));
  EXPECT_EQ(L"42h", fmt::format(L"{}", std::chrono::hours(42)));
  EXPECT_EQ(
      L"42[15]s",
      fmt::format(L"{}", std::chrono::duration<int, std::ratio<15, 1>>(42)));
  EXPECT_EQ(
      L"42[15/4]s",
      fmt::format(L"{}", std::chrono::duration<int, std::ratio<15, 4>>(42)));
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
  EXPECT_EQ("03:25:45    ",
            fmt::format("{:{}%H:%M:%S}", std::chrono::seconds(12345), 12));
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
  EXPECT_EQ("12345", fmt::format("{:%Q}", std::chrono::seconds(12345)));
  EXPECT_EQ("s", fmt::format("{:%q}", std::chrono::seconds(12345)));
}

TEST(ChronoTest, InvalidSpecs) {
  auto sec = std::chrono::seconds(0);
  EXPECT_THROW_MSG(fmt::format(+"{:%a}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format(+"{:%A}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format(+"{:%c}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format(+"{:%x}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format(+"{:%Ex}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format(+"{:%X}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format(+"{:%EX}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format(+"{:%D}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format(+"{:%F}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format(+"{:%Ec}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format(+"{:%w}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format(+"{:%u}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format(+"{:%b}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format(+"{:%B}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format(+"{:%z}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format(+"{:%Z}", sec), fmt::format_error, "no date");
  EXPECT_THROW_MSG(fmt::format(+"{:%Eq}", sec), fmt::format_error,
                   "invalid format");
  EXPECT_THROW_MSG(fmt::format(+"{:%Oq}", sec), fmt::format_error,
                   "invalid format");
}

TEST(ChronoTest, Locale) {
  const char* loc_name = "ja_JP.utf8";
  bool has_locale = false;
  std::locale loc;
  try {
    loc = std::locale(loc_name);
    has_locale = true;
  } catch (const std::runtime_error&) {
  }
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

typedef std::chrono::duration<double, std::milli> dms;

TEST(ChronoTest, FormatDefaultFP) {
  typedef std::chrono::duration<float> fs;
  EXPECT_EQ("1.234s", fmt::format("{}", fs(1.234)));
  typedef std::chrono::duration<float, std::milli> fms;
  EXPECT_EQ("1.234ms", fmt::format("{}", fms(1.234)));
  typedef std::chrono::duration<double> ds;
  EXPECT_EQ("1.234s", fmt::format("{}", ds(1.234)));
  EXPECT_EQ("1.234ms", fmt::format("{}", dms(1.234)));
}

TEST(ChronoTest, FormatPrecision) {
  EXPECT_THROW_MSG(fmt::format(+"{:.2}", std::chrono::seconds(42)),
                   fmt::format_error,
                   "precision not allowed for this argument type");
  EXPECT_EQ("1.2ms", fmt::format("{:.1}", dms(1.234)));
  EXPECT_EQ("1.23ms", fmt::format("{:.{}}", dms(1.234), 2));
}

TEST(ChronoTest, FormatFullSpecs) {
  EXPECT_EQ("1.2ms ", fmt::format("{:6.1}", dms(1.234)));
  EXPECT_EQ("  1.23ms", fmt::format("{:>8.{}}", dms(1.234), 2));
  EXPECT_EQ(" 1.2ms ", fmt::format("{:^{}.{}}", dms(1.234), 7, 1));
  EXPECT_EQ(" 1.23ms ", fmt::format("{0:^{2}.{1}}", dms(1.234), 2, 8));
  EXPECT_EQ("=1.234ms=", fmt::format("{:=^{}.{}}", dms(1.234), 9, 3));
  EXPECT_EQ("*1.2340ms*", fmt::format("{:*^10.4}", dms(1.234)));
}

TEST(ChronoTest, FormatSimpleQq) {
  typedef std::chrono::duration<float> fs;
  EXPECT_EQ("1.234 s", fmt::format("{:%Q %q}", fs(1.234)));
  typedef std::chrono::duration<float, std::milli> fms;
  EXPECT_EQ("1.234 ms", fmt::format("{:%Q %q}", fms(1.234)));
  typedef std::chrono::duration<double> ds;
  EXPECT_EQ("1.234 s", fmt::format("{:%Q %q}", ds(1.234)));
  EXPECT_EQ("1.234 ms", fmt::format("{:%Q %q}", dms(1.234)));
}

TEST(ChronoTest, FormatPrecisionQq) {
  EXPECT_THROW_MSG(fmt::format(+"{:.2%Q %q}", std::chrono::seconds(42)),
                   fmt::format_error,
                   "precision not allowed for this argument type");
  EXPECT_EQ("1.2 ms", fmt::format("{:.1%Q %q}", dms(1.234)));
  EXPECT_EQ("1.23 ms", fmt::format("{:.{}%Q %q}", dms(1.234), 2));
}

TEST(ChronoTest, FormatFullSpecsQq) {
  EXPECT_EQ("1.2 ms ", fmt::format("{:7.1%Q %q}", dms(1.234)));
  EXPECT_EQ(" 1.23 ms", fmt::format("{:>8.{}%Q %q}", dms(1.234), 2));
  EXPECT_EQ(" 1.2 ms ", fmt::format("{:^{}.{}%Q %q}", dms(1.234), 8, 1));
  EXPECT_EQ(" 1.23 ms ", fmt::format("{0:^{2}.{1}%Q %q}", dms(1.234), 2, 9));
  EXPECT_EQ("=1.234 ms=", fmt::format("{:=^{}.{}%Q %q}", dms(1.234), 10, 3));
  EXPECT_EQ("*1.2340 ms*", fmt::format("{:*^11.4%Q %q}", dms(1.234)));
}

TEST(ChronoTest, InvalidWidthId) {
  EXPECT_THROW(fmt::format(+"{:{o}", std::chrono::seconds(0)),
               fmt::format_error);
}

TEST(ChronoTest, InvalidColons) {
  EXPECT_THROW(fmt::format(+"{0}=:{0::", std::chrono::seconds(0)),
               fmt::format_error);
}

TEST(ChronoTest, NegativeDurations) {
  EXPECT_EQ("-12345", fmt::format("{:%Q}", std::chrono::seconds(-12345)));
  EXPECT_EQ("-03:25:45",
            fmt::format("{:%H:%M:%S}", std::chrono::seconds(-12345)));
  EXPECT_EQ("-00:01",
            fmt::format("{:%M:%S}", std::chrono::duration<double>(-1)));
  EXPECT_EQ("s", fmt::format("{:%q}", std::chrono::seconds(-12345)));
  EXPECT_EQ("-00.127",
            fmt::format("{:%S}",
                        std::chrono::duration<signed char, std::milli>{-127}));
  auto min = std::numeric_limits<int>::min();
  EXPECT_EQ(fmt::format("{}", min),
            fmt::format("{:%Q}", std::chrono::duration<int>(min)));
}

TEST(ChronoTest, SpecialDurations) {
  EXPECT_EQ(
      "40.",
      fmt::format("{:%S}", std::chrono::duration<double>(1e20)).substr(0, 3));
  auto nan = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(
      "nan nan nan nan nan:nan nan",
      fmt::format("{:%I %H %M %S %R %r}", std::chrono::duration<double>(nan)));
  fmt::format("{:%S}",
              std::chrono::duration<float, std::atto>(1.79400457e+31f));
  EXPECT_EQ(fmt::format("{}", std::chrono::duration<float, std::exa>(1)),
            "1Es");
  EXPECT_EQ(fmt::format("{}", std::chrono::duration<float, std::atto>(1)),
            "1as");
  EXPECT_EQ(fmt::format("{:%R}", std::chrono::duration<char, std::mega>{2}),
            "03:33");
  EXPECT_EQ(fmt::format("{:%T}", std::chrono::duration<char, std::mega>{2}),
            "03:33:20");
}

TEST(ChronoTest, UnsignedDuration) {
  EXPECT_EQ("42s", fmt::format("{}", std::chrono::duration<unsigned>(42)));
}

#endif  // FMT_STATIC_THOUSANDS_SEPARATOR
