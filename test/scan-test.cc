// Formatting library for C++ - scanning API test
//
// Copyright (c) 2019 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "scan.h"

#include <time.h>

#include <climits>

#include "fmt/os.h"
#include "gmock/gmock.h"
#include "gtest-extra.h"

TEST(scan_test, read_text) {
  fmt::string_view s = "foo";
  auto end = fmt::scan(s, "foo");
  EXPECT_EQ(end, s.end());
  EXPECT_THROW_MSG(fmt::scan("fob", "foo"), fmt::format_error, "invalid input");
}

TEST(scan_test, read_int) {
  int n = 0;
  fmt::scan("42", "{}", n);
  EXPECT_EQ(n, 42);
  fmt::scan("-42", "{}", n);
  EXPECT_EQ(n, -42);
}

TEST(scan_test, read_longlong) {
  long long n = 0;
  fmt::scan("42", "{}", n);
  EXPECT_EQ(n, 42);
  fmt::scan("-42", "{}", n);
  EXPECT_EQ(n, -42);
}

TEST(scan_test, read_uint) {
  unsigned n = 0;
  fmt::scan("42", "{}", n);
  EXPECT_EQ(n, 42);
  EXPECT_THROW_MSG(fmt::scan("-42", "{}", n), fmt::format_error,
                   "invalid input");
}

TEST(scan_test, read_ulonglong) {
  unsigned long long n = 0;
  fmt::scan("42", "{}", n);
  EXPECT_EQ(n, 42);
  EXPECT_THROW_MSG(fmt::scan("-42", "{}", n), fmt::format_error,
                   "invalid input");
}

TEST(scan_test, read_string) {
  std::string s;
  fmt::scan("foo", "{}", s);
  EXPECT_EQ(s, "foo");
}

TEST(scan_test, read_string_view) {
  fmt::string_view s;
  fmt::scan("foo", "{}", s);
  EXPECT_EQ(s, "foo");
}

#ifdef FMT_HAVE_STRPTIME
namespace fmt {
template <> struct scanner<tm> {
  std::string format;

  auto parse(scan_parse_context& ctx) -> scan_parse_context::iterator {
    auto it = ctx.begin();
    if (it != ctx.end() && *it == ':') ++it;
    auto end = it;
    while (end != ctx.end() && *end != '}') ++end;
    format.reserve(detail::to_unsigned(end - it + 1));
    format.append(it, end);
    format.push_back('\0');
    return end;
  }

  template <class ScanContext>
  auto scan(tm&, ScanContext& ctx) const -> typename ScanContext::iterator {
    // TODO: replace strptime with get_time
    // auto result = strptime(ctx.begin(), format.c_str(), &t);
    // if (!result) throw format_error("failed to parse time");
    // return result;
    return ctx.begin();
  }
};
}  // namespace fmt

TEST(scan_test, read_custom) {
  /*auto input = "Date: 1985-10-25";
  auto t = tm();
  fmt::scan(input, "Date: {0:%Y-%m-%d}", t);
  EXPECT_EQ(t.tm_year, 85);
  EXPECT_EQ(t.tm_mon, 9);
  EXPECT_EQ(t.tm_mday, 25);*/
}
#endif

TEST(scan_test, invalid_format) {
  EXPECT_THROW_MSG(fmt::scan("", "{}"), fmt::format_error,
                   "argument index out of range");
  EXPECT_THROW_MSG(fmt::scan("", "{"), fmt::format_error,
                   "invalid format string");
}

TEST(scan_test, example) {
  auto key = std::string();
  auto value = int();
  fmt::scan("answer = 42", "{} = {}", key, value);
  EXPECT_EQ(key, "answer");
  EXPECT_EQ(value, 42);
}

#if FMT_USE_FCNTL
TEST(scan_test, file) {
  fmt::file read_end, write_end;
  fmt::file::pipe(read_end, write_end);
  fmt::string_view input = "4";
  write_end.write(input.data(), input.size());
  write_end.close();
  int value = 0;
  fmt::scan(read_end.fdopen("r").get(), "{}", value);
  EXPECT_EQ(value, 4);
}
#endif  // FMT_USE_FCNTL
