// Formatting library for C++ - time formatting tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/chrono.h"

#include "gtest-extra.h"  // EXPECT_THROW_MSG

auto make_tm() -> std::tm {
  auto time = std::tm();
  time.tm_mday = 1;
  return time;
}

auto make_hour(int h) -> std::tm {
  auto time = make_tm();
  time.tm_hour = h;
  return time;
}

auto make_minute(int m) -> std::tm {
  auto time = make_tm();
  time.tm_min = m;
  return time;
}

auto make_second(int s) -> std::tm {
  auto time = make_tm();
  time.tm_sec = s;
  return time;
}

TEST(chrono_test, format_tm) {
  auto tm = std::tm();
  tm.tm_year = 116;
  tm.tm_mon = 3;
  tm.tm_mday = 25;
  tm.tm_hour = 11;
  tm.tm_min = 22;
  tm.tm_sec = 33;
  EXPECT_EQ(fmt::format("The date is {:%Y-%m-%d %H:%M:%S}.", tm),
            "The date is 2016-04-25 11:22:33.");
  EXPECT_EQ(fmt::format(L"The date is {:%Y-%m-%d %H:%M:%S}.", tm),
            L"The date is 2016-04-25 11:22:33.");
}

TEST(chrono_test, grow_buffer) {
  auto s = std::string("{:");
  for (int i = 0; i < 30; ++i) s += "%c";
  s += "}\n";
  auto t = std::time(nullptr);
  fmt::format(s, *std::localtime(&t));
}

TEST(chrono_test, format_to_empty_container) {
  auto time = std::tm();
  time.tm_sec = 42;
  auto s = std::string();
  fmt::format_to(std::back_inserter(s), "{:%S}", time);
  EXPECT_EQ(s, "42");
}

TEST(chrono_test, empty_result) { EXPECT_EQ(fmt::format("{}", std::tm()), ""); }

auto equal(const std::tm& lhs, const std::tm& rhs) -> bool {
  return lhs.tm_sec == rhs.tm_sec && lhs.tm_min == rhs.tm_min &&
         lhs.tm_hour == rhs.tm_hour && lhs.tm_mday == rhs.tm_mday &&
         lhs.tm_mon == rhs.tm_mon && lhs.tm_year == rhs.tm_year &&
         lhs.tm_wday == rhs.tm_wday && lhs.tm_yday == rhs.tm_yday &&
         lhs.tm_isdst == rhs.tm_isdst;
}

TEST(chrono_test, localtime) {
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);
  EXPECT_TRUE(equal(tm, fmt::localtime(t)));
}

TEST(chrono_test, gmtime) {
  auto t = std::time(nullptr);
  auto tm = *std::gmtime(&t);
  EXPECT_TRUE(equal(tm, fmt::gmtime(t)));
}

template <typename TimePoint> auto strftime(TimePoint tp) -> std::string {
  auto t = std::chrono::system_clock::to_time_t(tp);
  auto tm = *std::localtime(&t);
  char output[256] = {};
  std::strftime(output, sizeof(output), "%Y-%m-%d %H:%M:%S", &tm);
  return output;
}

TEST(chrono_test, time_point) {
  auto t1 = std::chrono::system_clock::now();
  EXPECT_EQ(strftime(t1), fmt::format("{:%Y-%m-%d %H:%M:%S}", t1));
  using time_point =
      std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>;
  auto t2 = time_point(std::chrono::seconds(42));
  EXPECT_EQ(strftime(t2), fmt::format("{:%Y-%m-%d %H:%M:%S}", t2));
}

#ifndef FMT_STATIC_THOUSANDS_SEPARATOR

TEST(chrono_test, format_default) {
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

TEST(chrono_test, format_wide) {
  EXPECT_EQ(L"42s", fmt::format(L"{}", std::chrono::seconds(42)));
}

TEST(chrono_test, align) {
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

TEST(chrono_test, format_specs) {
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

TEST(chrono_test, invalid_specs) {
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

auto format_tm(const std::tm& time, fmt::string_view spec,
               const std::locale& loc) -> std::string {
  auto& facet = std::use_facet<std::time_put<char>>(loc);
  std::ostringstream os;
  os.imbue(loc);
  facet.put(os, os, ' ', &time, spec.begin(), spec.end());
  return os.str();
}

#  define EXPECT_TIME(spec, time, duration)                 \
    {                                                       \
      auto jp_loc = std::locale("ja_JP.utf8");              \
      EXPECT_EQ(format_tm(time, spec, jp_loc),              \
                fmt::format(loc, "{:" spec "}", duration)); \
    }

TEST(chrono_test, locale) {
  auto loc_name = "ja_JP.utf8";
  bool has_locale = false;
  auto loc = std::locale();
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

using dms = std::chrono::duration<double, std::milli>;

TEST(chrono_test, format_default_fp) {
  typedef std::chrono::duration<float> fs;
  EXPECT_EQ("1.234s", fmt::format("{}", fs(1.234)));
  typedef std::chrono::duration<float, std::milli> fms;
  EXPECT_EQ("1.234ms", fmt::format("{}", fms(1.234)));
  typedef std::chrono::duration<double> ds;
  EXPECT_EQ("1.234s", fmt::format("{}", ds(1.234)));
  EXPECT_EQ("1.234ms", fmt::format("{}", dms(1.234)));
}

TEST(chrono_test, format_precision) {
  EXPECT_THROW_MSG(fmt::format(+"{:.2}", std::chrono::seconds(42)),
                   fmt::format_error,
                   "precision not allowed for this argument type");
  EXPECT_EQ("1.2ms", fmt::format("{:.1}", dms(1.234)));
  EXPECT_EQ("1.23ms", fmt::format("{:.{}}", dms(1.234), 2));
}

TEST(chrono_test, format_full_specs) {
  EXPECT_EQ("1.2ms ", fmt::format("{:6.1}", dms(1.234)));
  EXPECT_EQ("  1.23ms", fmt::format("{:>8.{}}", dms(1.234), 2));
  EXPECT_EQ(" 1.2ms ", fmt::format("{:^{}.{}}", dms(1.234), 7, 1));
  EXPECT_EQ(" 1.23ms ", fmt::format("{0:^{2}.{1}}", dms(1.234), 2, 8));
  EXPECT_EQ("=1.234ms=", fmt::format("{:=^{}.{}}", dms(1.234), 9, 3));
  EXPECT_EQ("*1.2340ms*", fmt::format("{:*^10.4}", dms(1.234)));
}

TEST(chrono_test, format_simple_q) {
  typedef std::chrono::duration<float> fs;
  EXPECT_EQ("1.234 s", fmt::format("{:%Q %q}", fs(1.234)));
  typedef std::chrono::duration<float, std::milli> fms;
  EXPECT_EQ("1.234 ms", fmt::format("{:%Q %q}", fms(1.234)));
  typedef std::chrono::duration<double> ds;
  EXPECT_EQ("1.234 s", fmt::format("{:%Q %q}", ds(1.234)));
  EXPECT_EQ("1.234 ms", fmt::format("{:%Q %q}", dms(1.234)));
}

TEST(chrono_test, format_precision_q) {
  EXPECT_THROW_MSG(fmt::format(+"{:.2%Q %q}", std::chrono::seconds(42)),
                   fmt::format_error,
                   "precision not allowed for this argument type");
  EXPECT_EQ("1.2 ms", fmt::format("{:.1%Q %q}", dms(1.234)));
  EXPECT_EQ("1.23 ms", fmt::format("{:.{}%Q %q}", dms(1.234), 2));
}

TEST(chrono_test, format_full_specs_q) {
  EXPECT_EQ("1.2 ms ", fmt::format("{:7.1%Q %q}", dms(1.234)));
  EXPECT_EQ(" 1.23 ms", fmt::format("{:>8.{}%Q %q}", dms(1.234), 2));
  EXPECT_EQ(" 1.2 ms ", fmt::format("{:^{}.{}%Q %q}", dms(1.234), 8, 1));
  EXPECT_EQ(" 1.23 ms ", fmt::format("{0:^{2}.{1}%Q %q}", dms(1.234), 2, 9));
  EXPECT_EQ("=1.234 ms=", fmt::format("{:=^{}.{}%Q %q}", dms(1.234), 10, 3));
  EXPECT_EQ("*1.2340 ms*", fmt::format("{:*^11.4%Q %q}", dms(1.234)));
}

TEST(chrono_test, invalid_width_id) {
  EXPECT_THROW(fmt::format(+"{:{o}", std::chrono::seconds(0)),
               fmt::format_error);
}

TEST(chrono_test, invalid_colons) {
  EXPECT_THROW(fmt::format(+"{0}=:{0::", std::chrono::seconds(0)),
               fmt::format_error);
}

TEST(chrono_test, negative_durations) {
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

TEST(chrono_test, special_durations) {
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

TEST(chrono_test, unsigned_duration) {
  EXPECT_EQ("42s", fmt::format("{}", std::chrono::duration<unsigned>(42)));
}

#endif  // FMT_STATIC_THOUSANDS_SEPARATOR
