// Formatting library for C++ - time formatting tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/chrono.h"

#include <algorithm>
#include <ctime>
#include <vector>

#include "gtest-extra.h"  // EXPECT_THROW_MSG
#include "util.h"         // get_locale

using fmt::runtime;
using testing::Contains;

#if defined(__MINGW32__) && !defined(_UCRT)
// Only C89 conversion specifiers when using MSVCRT instead of UCRT
#  define FMT_HAS_C99_STRFTIME 0
#else
#  define FMT_HAS_C99_STRFTIME 1
#endif

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

std::string system_strftime(const std::string& format, const std::tm* timeptr,
                            std::locale* locptr = nullptr) {
  auto loc = locptr ? *locptr : std::locale::classic();
  auto& facet = std::use_facet<std::time_put<char>>(loc);
  std::ostringstream os;
  os.imbue(loc);
  facet.put(os, os, ' ', timeptr, format.c_str(),
            format.c_str() + format.size());
#ifdef _WIN32
  // Workaround a bug in older versions of Universal CRT.
  auto str = os.str();
  if (str == "-0000") str = "+0000";
  return str;
#else
  return os.str();
#endif
}

FMT_CONSTEXPR std::tm make_tm(int year, int mon, int mday, int hour, int min,
                              int sec) {
  auto tm = std::tm();
  tm.tm_sec = sec;
  tm.tm_min = min;
  tm.tm_hour = hour;
  tm.tm_mday = mday;
  tm.tm_mon = mon - 1;
  tm.tm_year = year - 1900;
  return tm;
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
  EXPECT_EQ(fmt::format("{:%Y}", tm), "2016");
  EXPECT_EQ(fmt::format("{:%C}", tm), "20");
  EXPECT_EQ(fmt::format("{:%C%y}", tm), fmt::format("{:%Y}", tm));
  EXPECT_EQ(fmt::format("{:%e}", tm), "25");
  EXPECT_EQ(fmt::format("{:%D}", tm), "04/25/16");
  EXPECT_EQ(fmt::format("{:%F}", tm), "2016-04-25");
  EXPECT_EQ(fmt::format("{:%T}", tm), "11:22:33");

  // Short year
  tm.tm_year = 999 - 1900;
  tm.tm_mon = 0;   // for %G
  tm.tm_mday = 2;  // for %G
  tm.tm_wday = 3;  // for %G
  tm.tm_yday = 1;  // for %G
  EXPECT_EQ(fmt::format("{:%Y}", tm), "0999");
  EXPECT_EQ(fmt::format("{:%C%y}", tm), "0999");
  EXPECT_EQ(fmt::format("{:%G}", tm), "0999");

  tm.tm_year = 27 - 1900;
  EXPECT_EQ(fmt::format("{:%Y}", tm), "0027");
  EXPECT_EQ(fmt::format("{:%C%y}", tm), "0027");

  // Overflow year
  tm.tm_year = 2147483647;
  EXPECT_EQ(fmt::format("{:%Y}", tm), "2147485547");

  tm.tm_year = -2147483648;
  EXPECT_EQ(fmt::format("{:%Y}", tm), "-2147481748");

  // for week on the year
  // https://www.cl.cam.ac.uk/~mgk25/iso-time.html
  std::vector<std::tm> tm_list = {
      make_tm(1975, 12, 29, 12, 14, 16),  // W01
      make_tm(1977, 1, 2, 12, 14, 16),    // W53
      make_tm(1999, 12, 27, 12, 14, 16),  // W52
      make_tm(1999, 12, 31, 12, 14, 16),  // W52
      make_tm(2000, 1, 1, 12, 14, 16),    // W52
      make_tm(2000, 1, 2, 12, 14, 16),    // W52
      make_tm(2000, 1, 3, 12, 14, 16)     // W1
  };

#if !FMT_HAS_C99_STRFTIME
  GTEST_SKIP() << "Skip the rest of this test because it relies on strftime() "
                  "conforming to C99, but on this platform, MINGW + MSVCRT, "
                  "the function conforms only to C89.";
#endif

  const std::string iso_week_spec = "%Y-%m-%d: %G %g %V";
  for (auto ctm : tm_list) {
    // Calculate tm_yday, tm_wday, etc.
    std::time_t t = std::mktime(&ctm);
    tm = *std::localtime(&t);

    auto fmt_spec = fmt::format("{{:{}}}", iso_week_spec);
    EXPECT_EQ(system_strftime(iso_week_spec, &tm),
              fmt::format(fmt::runtime(fmt_spec), tm));
  }

  // Every day from 1970-01-01
  std::time_t time_now = std::time(nullptr);
  for (std::time_t t = 6 * 3600; t < time_now; t += 86400) {
    tm = *std::localtime(&t);

    auto fmt_spec = fmt::format("{{:{}}}", iso_week_spec);
    EXPECT_EQ(system_strftime(iso_week_spec, &tm),
              fmt::format(fmt::runtime(fmt_spec), tm));
  }
}

// MSVC:
//  minkernel\crts\ucrt\src\appcrt\time\wcsftime.cpp(971) : Assertion failed:
//  timeptr->tm_year >= -1900 && timeptr->tm_year <= 8099
#ifndef _WIN32
TEST(chrono_test, format_tm_future) {
  auto tm = std::tm();
  tm.tm_year = 10445;  // 10000+ years
  tm.tm_mon = 3;
  tm.tm_mday = 25;
  tm.tm_hour = 11;
  tm.tm_min = 22;
  tm.tm_sec = 33;
  EXPECT_EQ(fmt::format("The date is {:%Y-%m-%d %H:%M:%S}.", tm),
            "The date is 12345-04-25 11:22:33.");
  EXPECT_EQ(fmt::format("{:%Y}", tm), "12345");
  EXPECT_EQ(fmt::format("{:%C}", tm), "123");
  EXPECT_EQ(fmt::format("{:%C%y}", tm), fmt::format("{:%Y}", tm));
  EXPECT_EQ(fmt::format("{:%D}", tm), "04/25/45");
  EXPECT_EQ(fmt::format("{:%F}", tm), "12345-04-25");
  EXPECT_EQ(fmt::format("{:%T}", tm), "11:22:33");
}

TEST(chrono_test, format_tm_past) {
  auto tm = std::tm();
  tm.tm_year = -2001;
  tm.tm_mon = 3;
  tm.tm_mday = 25;
  tm.tm_hour = 11;
  tm.tm_min = 22;
  tm.tm_sec = 33;
  EXPECT_EQ(fmt::format("The date is {:%Y-%m-%d %H:%M:%S}.", tm),
            "The date is -101-04-25 11:22:33.");
  EXPECT_EQ(fmt::format("{:%Y}", tm), "-101");

  // macOS  %C - "-1"
  // Linux  %C - "-2"
  // fmt    %C - "-1"
  EXPECT_EQ(fmt::format("{:%C}", tm), "-1");
  EXPECT_EQ(fmt::format("{:%C%y}", tm), fmt::format("{:%Y}", tm));

  // macOS  %D - "04/25/01" (%y)
  // Linux  %D - "04/25/99" (%y)
  // fmt    %D - "04/25/01" (%y)
  EXPECT_EQ(fmt::format("{:%D}", tm), "04/25/01");

  EXPECT_EQ(fmt::format("{:%F}", tm), "-101-04-25");
  EXPECT_EQ(fmt::format("{:%T}", tm), "11:22:33");

  tm.tm_year = -1901;  // -1
  EXPECT_EQ(fmt::format("{:%Y}", tm), "-001");
  EXPECT_EQ(fmt::format("{:%C%y}", tm), fmt::format("{:%Y}", tm));

  tm.tm_year = -1911;  // -11
  EXPECT_EQ(fmt::format("{:%Y}", tm), "-011");
  EXPECT_EQ(fmt::format("{:%C%y}", tm), fmt::format("{:%Y}", tm));
}
#endif

TEST(chrono_test, grow_buffer) {
  auto s = std::string("{:");
  for (int i = 0; i < 30; ++i) s += "%c";
  s += "}\n";
  auto t = std::time(nullptr);
  (void)fmt::format(fmt::runtime(s), *std::localtime(&t));
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

TEST(chrono_test, gmtime) {
  auto t = std::time(nullptr);
  auto tm = *std::gmtime(&t);
  EXPECT_TRUE(equal(tm, fmt::gmtime(t)));
}

template <typename TimePoint>
auto strftime_full_utc(TimePoint tp) -> std::string {
  auto t = std::chrono::system_clock::to_time_t(tp);
  auto tm = *std::gmtime(&t);
  return system_strftime("%Y-%m-%d %H:%M:%S", &tm);
}

TEST(chrono_test, system_clock_time_point) {
  auto t1 = std::chrono::time_point_cast<std::chrono::seconds>(
      std::chrono::system_clock::now());
  EXPECT_EQ(strftime_full_utc(t1), fmt::format("{:%Y-%m-%d %H:%M:%S}", t1));
  EXPECT_EQ(strftime_full_utc(t1), fmt::format("{}", t1));
  EXPECT_EQ(strftime_full_utc(t1), fmt::format("{:}", t1));
  using time_point =
      std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>;
  auto t2 = time_point(std::chrono::seconds(42));
  EXPECT_EQ(strftime_full_utc(t2), fmt::format("{:%Y-%m-%d %H:%M:%S}", t2));

  std::vector<std::string> spec_list = {
      "%%",  "%n",  "%t",  "%Y",  "%EY", "%y",  "%Oy", "%Ey", "%C",
      "%EC", "%G",  "%g",  "%b",  "%h",  "%B",  "%m",  "%Om", "%U",
      "%OU", "%W",  "%OW", "%V",  "%OV", "%j",  "%d",  "%Od", "%e",
      "%Oe", "%a",  "%A",  "%w",  "%Ow", "%u",  "%Ou", "%H",  "%OH",
      "%I",  "%OI", "%M",  "%OM", "%S",  "%OS", "%x",  "%Ex", "%X",
      "%EX", "%D",  "%F",  "%R",  "%T",  "%p"};
#ifndef _WIN32
  // Disabled on Windows because these formats are not consistent among
  // platforms.
  spec_list.insert(spec_list.end(), {"%c", "%Ec", "%r"});
#elif !FMT_HAS_C99_STRFTIME
  // Only C89 conversion specifiers when using MSVCRT instead of UCRT
  spec_list = {"%%", "%Y", "%y", "%b", "%B", "%m", "%U", "%W", "%j", "%d",
               "%a", "%A", "%w", "%H", "%I", "%M", "%S", "%x", "%X", "%p"};
#endif
  spec_list.push_back("%Y-%m-%d %H:%M:%S");

  for (const auto& spec : spec_list) {
    auto t = std::chrono::system_clock::to_time_t(t1);
    auto tm = *std::gmtime(&t);

    auto sys_output = system_strftime(spec, &tm);

    auto fmt_spec = fmt::format("{{:{}}}", spec);
    EXPECT_EQ(sys_output, fmt::format(fmt::runtime(fmt_spec), t1));
    EXPECT_EQ(sys_output, fmt::format(fmt::runtime(fmt_spec), tm));
  }

  // Timezone formatters tests makes sense for localtime.
#if FMT_HAS_C99_STRFTIME
  spec_list = {"%z", "%Z"};
#else
  spec_list = {"%Z"};
#endif
  for (const auto& spec : spec_list) {
    auto t = std::chrono::system_clock::to_time_t(t1);
    auto tm = *std::localtime(&t);

    auto sys_output = system_strftime(spec, &tm);

    auto fmt_spec = fmt::format("{{:{}}}", spec);
    EXPECT_EQ(sys_output, fmt::format(fmt::runtime(fmt_spec), tm));

    if (spec == "%z") {
      sys_output.insert(sys_output.end() - 2, 1, ':');
      EXPECT_EQ(sys_output, fmt::format("{:%Ez}", tm));
      EXPECT_EQ(sys_output, fmt::format("{:%Oz}", tm));
    }
  }

  // Separate tests for UTC, since std::time_put can use local time and ignoring
  // the timezone in std::tm (if it presents on platform).
  if (fmt::detail::has_member_data_tm_zone<std::tm>::value) {
    auto t = std::chrono::system_clock::to_time_t(t1);
    auto tm = *std::gmtime(&t);

    std::vector<std::string> tz_names = {"GMT", "UTC"};
    EXPECT_THAT(tz_names, Contains(fmt::format("{:%Z}", t1)));
    EXPECT_THAT(tz_names, Contains(fmt::format("{:%Z}", tm)));
  }

  if (fmt::detail::has_member_data_tm_gmtoff<std::tm>::value) {
    auto t = std::chrono::system_clock::to_time_t(t1);
    auto tm = *std::gmtime(&t);

    EXPECT_EQ("+0000", fmt::format("{:%z}", t1));
    EXPECT_EQ("+0000", fmt::format("{:%z}", tm));

    EXPECT_EQ("+00:00", fmt::format("{:%Ez}", t1));
    EXPECT_EQ("+00:00", fmt::format("{:%Ez}", tm));

    EXPECT_EQ("+00:00", fmt::format("{:%Oz}", t1));
    EXPECT_EQ("+00:00", fmt::format("{:%Oz}", tm));
  }
}

#if FMT_USE_LOCAL_TIME

TEST(chrono_test, localtime) {
  auto t = std::time(nullptr);
  auto tm = *std::localtime(&t);
  EXPECT_TRUE(equal(tm, fmt::localtime(t)));
}

template <typename Duration>
auto strftime_full_local(std::chrono::local_time<Duration> tp) -> std::string {
  auto t = std::chrono::system_clock::to_time_t(
      std::chrono::current_zone()->to_sys(tp));
  auto tm = *std::localtime(&t);
  return system_strftime("%Y-%m-%d %H:%M:%S", &tm);
}

TEST(chrono_test, local_system_clock_time_point) {
#  ifdef _WIN32
  return;  // Not supported on Windows.
#  endif
  auto t1 = std::chrono::time_point_cast<std::chrono::seconds>(
      std::chrono::current_zone()->to_local(std::chrono::system_clock::now()));
  EXPECT_EQ(strftime_full_local(t1), fmt::format("{:%Y-%m-%d %H:%M:%S}", t1));
  EXPECT_EQ(strftime_full_local(t1), fmt::format("{}", t1));
  EXPECT_EQ(strftime_full_local(t1), fmt::format("{:}", t1));
  using time_point = std::chrono::local_time<std::chrono::seconds>;
  auto t2 = time_point(std::chrono::seconds(86400 + 42));
  EXPECT_EQ(strftime_full_local(t2), fmt::format("{:%Y-%m-%d %H:%M:%S}", t2));

  std::vector<std::string> spec_list = {
      "%%",  "%n",  "%t",  "%Y",  "%EY", "%y",  "%Oy", "%Ey", "%C",
      "%EC", "%G",  "%g",  "%b",  "%h",  "%B",  "%m",  "%Om", "%U",
      "%OU", "%W",  "%OW", "%V",  "%OV", "%j",  "%d",  "%Od", "%e",
      "%Oe", "%a",  "%A",  "%w",  "%Ow", "%u",  "%Ou", "%H",  "%OH",
      "%I",  "%OI", "%M",  "%OM", "%S",  "%OS", "%x",  "%Ex", "%X",
      "%EX", "%D",  "%F",  "%R",  "%T",  "%p",  "%z",  "%Z"};
#  ifndef _WIN32
  // Disabled on Windows because these formats are not consistent among
  // platforms.
  spec_list.insert(spec_list.end(), {"%c", "%Ec", "%r"});
#  elif !FMT_HAS_C99_STRFTIME
  // Only C89 conversion specifiers when using MSVCRT instead of UCRT
  spec_list = {"%%", "%Y", "%y", "%b", "%B", "%m", "%U", "%W", "%j", "%d", "%a",
               "%A", "%w", "%H", "%I", "%M", "%S", "%x", "%X", "%p", "%Z"};
#  endif
  spec_list.push_back("%Y-%m-%d %H:%M:%S");

  for (const auto& spec : spec_list) {
    auto t = std::chrono::system_clock::to_time_t(
        std::chrono::current_zone()->to_sys(t1));
    auto tm = *std::localtime(&t);

    auto sys_output = system_strftime(spec, &tm);

    auto fmt_spec = fmt::format("{{:{}}}", spec);
    EXPECT_EQ(sys_output, fmt::format(fmt::runtime(fmt_spec), t1));
    EXPECT_EQ(sys_output, fmt::format(fmt::runtime(fmt_spec), tm));
  }

  if (std::find(spec_list.cbegin(), spec_list.cend(), "%z") !=
      spec_list.cend()) {
    auto t = std::chrono::system_clock::to_time_t(
        std::chrono::current_zone()->to_sys(t1));
    auto tm = *std::localtime(&t);

    auto sys_output = system_strftime("%z", &tm);
    sys_output.insert(sys_output.end() - 2, 1, ':');

    EXPECT_EQ(sys_output, fmt::format("{:%Ez}", t1));
    EXPECT_EQ(sys_output, fmt::format("{:%Ez}", tm));

    EXPECT_EQ(sys_output, fmt::format("{:%Oz}", t1));
    EXPECT_EQ(sys_output, fmt::format("{:%Oz}", tm));
  }
}

#endif  // FMT_USE_LOCAL_TIME

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

TEST(chrono_test, duration_align) {
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

TEST(chrono_test, tm_align) {
  auto t = make_tm(1975, 12, 29, 12, 14, 16);
  EXPECT_EQ("1975-12-29 12:14:16", fmt::format("{:%F %T}", t));
  EXPECT_EQ("1975-12-29 12:14:16           ", fmt::format("{:30%F %T}", t));
  EXPECT_EQ("1975-12-29 12:14:16           ", fmt::format("{:{}%F %T}", t, 30));
  EXPECT_EQ("1975-12-29 12:14:16           ", fmt::format("{:<30%F %T}", t));
  EXPECT_EQ("     1975-12-29 12:14:16      ", fmt::format("{:^30%F %T}", t));
  EXPECT_EQ("           1975-12-29 12:14:16", fmt::format("{:>30%F %T}", t));

  EXPECT_EQ("1975-12-29 12:14:16***********", fmt::format("{:*<30%F %T}", t));
  EXPECT_EQ("*****1975-12-29 12:14:16******", fmt::format("{:*^30%F %T}", t));
  EXPECT_EQ("***********1975-12-29 12:14:16", fmt::format("{:*>30%F %T}", t));
}

TEST(chrono_test, tp_align) {
  auto tp = std::chrono::time_point_cast<std::chrono::microseconds>(
      std::chrono::system_clock::from_time_t(0));
  EXPECT_EQ("00:00.000000", fmt::format("{:%M:%S}", tp));
  EXPECT_EQ("00:00.000000   ", fmt::format("{:15%M:%S}", tp));
  EXPECT_EQ("00:00.000000   ", fmt::format("{:{}%M:%S}", tp, 15));
  EXPECT_EQ("00:00.000000   ", fmt::format("{:<15%M:%S}", tp));
  EXPECT_EQ(" 00:00.000000  ", fmt::format("{:^15%M:%S}", tp));
  EXPECT_EQ("   00:00.000000", fmt::format("{:>15%M:%S}", tp));

  EXPECT_EQ("00:00.000000***", fmt::format("{:*<15%M:%S}", tp));
  EXPECT_EQ("*00:00.000000**", fmt::format("{:*^15%M:%S}", tp));
  EXPECT_EQ("***00:00.000000", fmt::format("{:*>15%M:%S}", tp));
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
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%a}"), sec), fmt::format_error,
                   "no date");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%A}"), sec), fmt::format_error,
                   "no date");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%c}"), sec), fmt::format_error,
                   "no date");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%x}"), sec), fmt::format_error,
                   "no date");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%Ex}"), sec), fmt::format_error,
                   "no date");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%X}"), sec), fmt::format_error,
                   "no date");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%EX}"), sec), fmt::format_error,
                   "no date");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%D}"), sec), fmt::format_error,
                   "no date");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%F}"), sec), fmt::format_error,
                   "no date");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%Ec}"), sec), fmt::format_error,
                   "no date");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%w}"), sec), fmt::format_error,
                   "no date");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%u}"), sec), fmt::format_error,
                   "no date");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%b}"), sec), fmt::format_error,
                   "no date");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%B}"), sec), fmt::format_error,
                   "no date");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%z}"), sec), fmt::format_error,
                   "no date");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%Z}"), sec), fmt::format_error,
                   "no date");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%Eq}"), sec), fmt::format_error,
                   "invalid format");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%Oq}"), sec), fmt::format_error,
                   "invalid format");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:abc}"), sec), fmt::format_error,
                   "invalid format");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:.2f}"), sec), fmt::format_error,
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

TEST(chrono_test, locale) {
  auto loc = get_locale("ja_JP.utf8");
  if (loc == std::locale::classic()) return;
#  define EXPECT_TIME(spec, time, duration)                     \
    {                                                           \
      auto jp_loc = std::locale("ja_JP.utf8");                  \
      EXPECT_EQ(format_tm(time, spec, jp_loc),                  \
                fmt::format(jp_loc, "{:L" spec "}", duration)); \
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
  EXPECT_THROW_MSG(
      (void)fmt::format(runtime("{:.2%Q}"), std::chrono::seconds(42)),
      fmt::format_error, "precision not allowed for this argument type");
  EXPECT_EQ("1ms", fmt::format("{:.0}", dms(1.234)));
  EXPECT_EQ("1.2ms", fmt::format("{:.1}", dms(1.234)));
  EXPECT_EQ("1.23ms", fmt::format("{:.{}}", dms(1.234), 2));

  EXPECT_EQ("13ms", fmt::format("{:.0}", dms(12.56)));
  EXPECT_EQ("12.6ms", fmt::format("{:.1}", dms(12.56)));
  EXPECT_EQ("12.56ms", fmt::format("{:.2}", dms(12.56)));
}

TEST(chrono_test, format_full_specs) {
  EXPECT_EQ("1ms   ", fmt::format("{:6.0}", dms(1.234)));
  EXPECT_EQ("1.2ms ", fmt::format("{:6.1}", dms(1.234)));
  EXPECT_EQ("  1.23ms", fmt::format("{:>8.{}}", dms(1.234), 2));
  EXPECT_EQ(" 1.2ms ", fmt::format("{:^{}.{}}", dms(1.234), 7, 1));
  EXPECT_EQ(" 1.23ms ", fmt::format("{0:^{2}.{1}}", dms(1.234), 2, 8));
  EXPECT_EQ("=1.234ms=", fmt::format("{:=^{}.{}}", dms(1.234), 9, 3));
  EXPECT_EQ("*1.2340ms*", fmt::format("{:*^10.4}", dms(1.234)));

  EXPECT_EQ("13ms  ", fmt::format("{:6.0}", dms(12.56)));
  EXPECT_EQ("    13ms", fmt::format("{:>8.{}}", dms(12.56), 0));
  EXPECT_EQ(" 13ms ", fmt::format("{:^{}.{}}", dms(12.56), 6, 0));
  EXPECT_EQ("  13ms  ", fmt::format("{0:^{2}.{1}}", dms(12.56), 0, 8));
  EXPECT_EQ("==13ms===", fmt::format("{:=^{}.{}}", dms(12.56), 9, 0));
  EXPECT_EQ("***13ms***", fmt::format("{:*^10.0}", dms(12.56)));
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
  EXPECT_THROW_MSG(
      (void)fmt::format(runtime("{:.2%Q %q}"), std::chrono::seconds(42)),
      fmt::format_error, "precision not allowed for this argument type");
  EXPECT_EQ("1.2 ms", fmt::format("{:.1%Q %q}", dms(1.234)));
  EXPECT_EQ("1.23 ms", fmt::format("{:.{}%Q %q}", dms(1.234), 2));
}

TEST(chrono_test, format_full_specs_q) {
  EXPECT_EQ("1 ms   ", fmt::format("{:7.0%Q %q}", dms(1.234)));
  EXPECT_EQ("1.2 ms ", fmt::format("{:7.1%Q %q}", dms(1.234)));
  EXPECT_EQ(" 1.23 ms", fmt::format("{:>8.{}%Q %q}", dms(1.234), 2));
  EXPECT_EQ(" 1.2 ms ", fmt::format("{:^{}.{}%Q %q}", dms(1.234), 8, 1));
  EXPECT_EQ(" 1.23 ms ", fmt::format("{0:^{2}.{1}%Q %q}", dms(1.234), 2, 9));
  EXPECT_EQ("=1.234 ms=", fmt::format("{:=^{}.{}%Q %q}", dms(1.234), 10, 3));
  EXPECT_EQ("*1.2340 ms*", fmt::format("{:*^11.4%Q %q}", dms(1.234)));

  EXPECT_EQ("13 ms  ", fmt::format("{:7.0%Q %q}", dms(12.56)));
  EXPECT_EQ("   13 ms", fmt::format("{:>8.{}%Q %q}", dms(12.56), 0));
  EXPECT_EQ(" 13 ms  ", fmt::format("{:^{}.{}%Q %q}", dms(12.56), 8, 0));
  EXPECT_EQ("  13 ms  ", fmt::format("{0:^{2}.{1}%Q %q}", dms(12.56), 0, 9));
  EXPECT_EQ("==13 ms==", fmt::format("{:=^{}.{}%Q %q}", dms(12.56), 9, 0));
  EXPECT_EQ("***13 ms***", fmt::format("{:*^11.0%Q %q}", dms(12.56)));
}

TEST(chrono_test, invalid_width_id) {
  EXPECT_THROW((void)fmt::format(runtime("{:{o}"), std::chrono::seconds(0)),
               fmt::format_error);
}

TEST(chrono_test, invalid_colons) {
  EXPECT_THROW((void)fmt::format(runtime("{0}=:{0::"), std::chrono::seconds(0)),
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
  auto value = fmt::format("{:%S}", std::chrono::duration<double>(1e20));
  EXPECT_EQ(value, "40");
  auto nan = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(
      "nan nan nan nan nan:nan nan",
      fmt::format("{:%I %H %M %S %R %r}", std::chrono::duration<double>(nan)));
  EXPECT_EQ(fmt::format("{}", std::chrono::duration<float, std::exa>(1)),
            "1Es");
  EXPECT_EQ(fmt::format("{}", std::chrono::duration<float, std::atto>(1)),
            "1as");
  EXPECT_EQ(fmt::format("{:%R}", std::chrono::duration<char, std::mega>{2}),
            "03:33");
  EXPECT_EQ(fmt::format("{:%T}", std::chrono::duration<char, std::mega>{2}),
            "03:33:20");
  EXPECT_EQ("44.000000000000",
            fmt::format("{:%S}", std::chrono::duration<float, std::pico>(
                                     1.54213895E+26)));
}

TEST(chrono_test, unsigned_duration) {
  EXPECT_EQ("42s", fmt::format("{}", std::chrono::duration<unsigned>(42)));
}

TEST(chrono_test, weekday) {
  auto loc = get_locale("ru_RU.UTF-8");
  std::locale::global(loc);
  auto mon = fmt::weekday(1);

  auto tm = std::tm();
  tm.tm_wday = static_cast<int>(mon.c_encoding());

  EXPECT_EQ(fmt::format("{}", mon), "Mon");
  EXPECT_EQ(fmt::format("{:%a}", tm), "Mon");

  if (loc != std::locale::classic()) {
    EXPECT_THAT((std::vector<std::string>{"пн", "Пн", "пнд", "Пнд"}),
                Contains(fmt::format(loc, "{:L}", mon)));
    EXPECT_THAT((std::vector<std::string>{"пн", "Пн", "пнд", "Пнд"}),
                Contains(fmt::format(loc, "{:%a}", tm)));
  }
}

TEST(chrono_test, cpp20_duration_subsecond_support) {
  using attoseconds = std::chrono::duration<long long, std::atto>;
  // Check that 18 digits of subsecond precision are supported.
  EXPECT_EQ(fmt::format("{:%S}", attoseconds{999999999999999999}),
            "00.999999999999999999");
  EXPECT_EQ(fmt::format("{:%S}", attoseconds{673231113420148734}),
            "00.673231113420148734");
  EXPECT_EQ(fmt::format("{:%S}", attoseconds{-673231113420148734}),
            "-00.673231113420148734");
  EXPECT_EQ(fmt::format("{:%S}", std::chrono::nanoseconds{13420148734}),
            "13.420148734");
  EXPECT_EQ(fmt::format("{:%S}", std::chrono::nanoseconds{-13420148734}),
            "-13.420148734");
  EXPECT_EQ(fmt::format("{:%S}", std::chrono::milliseconds{1234}), "01.234");
  // Check subsecond presision modifier.
  EXPECT_EQ(fmt::format("{:.6%S}", std::chrono::nanoseconds{1234}),
            "00.000001");
  EXPECT_EQ(fmt::format("{:.18%S}", std::chrono::nanoseconds{1234}),
            "00.000001234000000000");
  EXPECT_EQ(fmt::format("{:.{}%S}", std::chrono::nanoseconds{1234}, 6),
            "00.000001");
  EXPECT_EQ(fmt::format("{:.6%S}", std::chrono::milliseconds{1234}),
            "01.234000");
  EXPECT_EQ(fmt::format("{:.6%S}", std::chrono::milliseconds{-1234}),
            "-01.234000");
  EXPECT_EQ(fmt::format("{:.3%S}", std::chrono::seconds{1234}), "34.000");
  EXPECT_EQ(fmt::format("{:.3%S}", std::chrono::hours{1234}), "00.000");
  EXPECT_EQ(fmt::format("{:.5%S}", dms(1.234)), "00.00123");
  EXPECT_EQ(fmt::format("{:.8%S}", dms(1.234)), "00.00123400");
  {
    // Check that {:%H:%M:%S} is equivalent to {:%T}.
    auto dur = std::chrono::milliseconds{3601234};
    auto formatted_dur = fmt::format("{:%T}", dur);
    EXPECT_EQ(formatted_dur, "01:00:01.234");
    EXPECT_EQ(fmt::format("{:%H:%M:%S}", dur), formatted_dur);
    EXPECT_EQ(fmt::format("{:.6%H:%M:%S}", dur), "01:00:01.234000");
  }
  using nanoseconds_dbl = std::chrono::duration<double, std::nano>;
  EXPECT_EQ(fmt::format("{:%S}", nanoseconds_dbl{-123456789}), "-00.123456789");
  EXPECT_EQ(fmt::format("{:%S}", nanoseconds_dbl{9123456789}), "09.123456789");
  // Verify that only the seconds part is extracted and printed.
  EXPECT_EQ(fmt::format("{:%S}", nanoseconds_dbl{99123456789}), "39.123456789");
  EXPECT_EQ(fmt::format("{:%S}", nanoseconds_dbl{99123000000}), "39.123000000");
  {
    // Now the hour is printed, and we also test if negative doubles work.
    auto dur = nanoseconds_dbl{-99123456789};
    auto formatted_dur = fmt::format("{:%T}", dur);
    EXPECT_EQ(formatted_dur, "-00:01:39.123456789");
    EXPECT_EQ(fmt::format("{:%H:%M:%S}", dur), formatted_dur);
    EXPECT_EQ(fmt::format("{:.3%H:%M:%S}", dur), "-00:01:39.123");
  }
  // Check that durations with precision greater than std::chrono::seconds have
  // fixed precision, and print zeros even if there is no fractional part.
  EXPECT_EQ(fmt::format("{:%S}", std::chrono::microseconds{7000000}),
            "07.000000");
  EXPECT_EQ(fmt::format("{:%S}",
                        std::chrono::duration<long long, std::ratio<1, 3>>(1)),
            "00.333333");
  EXPECT_EQ(fmt::format("{:%S}",
                        std::chrono::duration<long long, std::ratio<1, 7>>(1)),
            "00.142857");

  EXPECT_EQ(
      fmt::format("{:%S}",
                  std::chrono::duration<signed char, std::ratio<1, 100>>(0x80)),
      "-01.28");

  EXPECT_EQ(
      fmt::format("{:%M:%S}",
                  std::chrono::duration<short, std::ratio<1, 100>>(0x8000)),
      "-05:27.68");

  // Check that floating point seconds with ratio<1,1> are printed.
  EXPECT_EQ(fmt::format("{:%S}", std::chrono::duration<double>{1.5}),
            "01.500000");
  EXPECT_EQ(fmt::format("{:%M:%S}", std::chrono::duration<double>{-61.25}),
            "-01:01.250000");
}

#endif  // FMT_STATIC_THOUSANDS_SEPARATOR

// Disable the utc_clock test for windows, as the icu.dll used for tzdb
// (time zone database) is not shipped with many windows versions.
#if FMT_USE_UTC_TIME && !defined(_WIN32)
TEST(chrono_test, utc_clock) {
  auto t1 = std::chrono::system_clock::now();
  auto t1_utc = std::chrono::utc_clock::from_sys(t1);
  EXPECT_EQ(fmt::format("{:%Y-%m-%d %H:%M:%S}", t1),
            fmt::format("{:%Y-%m-%d %H:%M:%S}", t1_utc));
}
#endif

TEST(chrono_test, timestamps_sub_seconds) {
  std::chrono::time_point<std::chrono::system_clock,
                          std::chrono::duration<long long, std::ratio<1, 3>>>
      t1(std::chrono::duration<long long, std::ratio<1, 3>>(4));

  EXPECT_EQ(fmt::format("{:%S}", t1), "01.333333");

  std::chrono::time_point<std::chrono::system_clock,
                          std::chrono::duration<double, std::ratio<1, 3>>>
      t2(std::chrono::duration<double, std::ratio<1, 3>>(4));

  EXPECT_EQ(fmt::format("{:%S}", t2), "01.333333");

  const std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
      t3(std::chrono::seconds(2));

  EXPECT_EQ(fmt::format("{:%S}", t3), "02");

  const std::chrono::time_point<std::chrono::system_clock,
                                std::chrono::duration<double>>
      t4(std::chrono::duration<double, std::ratio<1, 1>>(9.5));

  EXPECT_EQ(fmt::format("{:%S}", t4), "09.500000");

  const std::chrono::time_point<std::chrono::system_clock,
                                std::chrono::duration<double>>
      t5(std::chrono::duration<double, std::ratio<1, 1>>(9));

  EXPECT_EQ(fmt::format("{:%S}", t5), "09");

  const std::chrono::time_point<std::chrono::system_clock,
                                std::chrono::milliseconds>
      t6(std::chrono::seconds(1) + std::chrono::milliseconds(120));

  EXPECT_EQ(fmt::format("{:%S}", t6), "01.120");

  const std::chrono::time_point<std::chrono::system_clock,
                                std::chrono::microseconds>
      t7(std::chrono::microseconds(1234567));

  EXPECT_EQ(fmt::format("{:%S}", t7), "01.234567");

  const std::chrono::time_point<std::chrono::system_clock,
                                std::chrono::nanoseconds>
      t8(std::chrono::nanoseconds(123456789));

  EXPECT_EQ(fmt::format("{:%S}", t8), "00.123456789");

  const auto t9 = std::chrono::time_point_cast<std::chrono::nanoseconds>(
      std::chrono::system_clock::now());
  const auto t9_sec = std::chrono::time_point_cast<std::chrono::seconds>(t9);
  auto t9_sub_sec_part = fmt::format("{0:09}", (t9 - t9_sec).count());

  EXPECT_EQ(fmt::format("{}.{}", strftime_full_utc(t9_sec), t9_sub_sec_part),
            fmt::format("{:%Y-%m-%d %H:%M:%S}", t9));
  EXPECT_EQ(fmt::format("{}.{}", strftime_full_utc(t9_sec), t9_sub_sec_part),
            fmt::format("{:%Y-%m-%d %T}", t9));

  const std::chrono::time_point<std::chrono::system_clock,
                                std::chrono::milliseconds>
      t10(std::chrono::milliseconds(2000));

  EXPECT_EQ(fmt::format("{:%S}", t10), "02.000");

  {
    const auto epoch = std::chrono::time_point<std::chrono::system_clock,
                                               std::chrono::milliseconds>();
    const auto d = std::chrono::milliseconds(250);

    EXPECT_EQ("59.750", fmt::format("{:%S}", epoch - d));
    EXPECT_EQ("00.000", fmt::format("{:%S}", epoch));
    EXPECT_EQ("00.250", fmt::format("{:%S}", epoch + d));
  }
}

TEST(chrono_test, glibc_extensions) {
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%0}"), std::chrono::seconds()),
                   fmt::format_error, "invalid format");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%_}"), std::chrono::seconds()),
                   fmt::format_error, "invalid format");
  EXPECT_THROW_MSG((void)fmt::format(runtime("{:%-}"), std::chrono::seconds()),
                   fmt::format_error, "invalid format");

  {
    const auto d = std::chrono::hours(1) + std::chrono::minutes(2) +
                   std::chrono::seconds(3);

    EXPECT_EQ(fmt::format("{:%I,%H,%M,%S}", d), "01,01,02,03");
    EXPECT_EQ(fmt::format("{:%0I,%0H,%0M,%0S}", d), "01,01,02,03");
    EXPECT_EQ(fmt::format("{:%_I,%_H,%_M,%_S}", d), " 1, 1, 2, 3");
    EXPECT_EQ(fmt::format("{:%-I,%-H,%-M,%-S}", d), "1,1,2,3");

    EXPECT_EQ(fmt::format("{:%OI,%OH,%OM,%OS}", d), "01,01,02,03");
    EXPECT_EQ(fmt::format("{:%0OI,%0OH,%0OM,%0OS}", d), "01,01,02,03");
    EXPECT_EQ(fmt::format("{:%_OI,%_OH,%_OM,%_OS}", d), " 1, 1, 2, 3");
    EXPECT_EQ(fmt::format("{:%-OI,%-OH,%-OM,%-OS}", d), "1,1,2,3");
  }

  {
    const auto tm = make_tm(1970, 1, 1, 1, 2, 3);
    EXPECT_EQ(fmt::format("{:%I,%H,%M,%S}", tm), "01,01,02,03");
    EXPECT_EQ(fmt::format("{:%0I,%0H,%0M,%0S}", tm), "01,01,02,03");
    EXPECT_EQ(fmt::format("{:%_I,%_H,%_M,%_S}", tm), " 1, 1, 2, 3");
    EXPECT_EQ(fmt::format("{:%-I,%-H,%-M,%-S}", tm), "1,1,2,3");

    EXPECT_EQ(fmt::format("{:%OI,%OH,%OM,%OS}", tm), "01,01,02,03");
    EXPECT_EQ(fmt::format("{:%0OI,%0OH,%0OM,%0OS}", tm), "01,01,02,03");
    EXPECT_EQ(fmt::format("{:%_OI,%_OH,%_OM,%_OS}", tm), " 1, 1, 2, 3");
    EXPECT_EQ(fmt::format("{:%-OI,%-OH,%-OM,%-OS}", tm), "1,1,2,3");
  }

  {
    const auto d = std::chrono::seconds(3) + std::chrono::milliseconds(140);
    EXPECT_EQ(fmt::format("{:%S}", d), "03.140");
    EXPECT_EQ(fmt::format("{:%0S}", d), "03.140");
    EXPECT_EQ(fmt::format("{:%_S}", d), " 3.140");
    EXPECT_EQ(fmt::format("{:%-S}", d), "3.140");
  }

  {
    const auto d = std::chrono::duration<double>(3.14);
    EXPECT_EQ(fmt::format("{:%S}", d), "03.140000");
    EXPECT_EQ(fmt::format("{:%0S}", d), "03.140000");
    EXPECT_EQ(fmt::format("{:%_S}", d), " 3.140000");
    EXPECT_EQ(fmt::format("{:%-S}", d), "3.140000");
  }
}
