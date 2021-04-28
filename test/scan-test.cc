// Formatting library for C++ - scanning API test
//
// Copyright (c) 2019 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "scan.h"

#include <time.h>

#include <climits>

#include "gmock/gmock.h"
#include "gtest-extra.h"

TEST(ScanTest, ReadText) {
  fmt::string_view s = "foo";
  auto end = fmt::scan(s, "foo");
  EXPECT_EQ(end, s.end());
  EXPECT_THROW_MSG(fmt::scan("fob", "foo"), fmt::format_error, "invalid input");
}

TEST(ScanTest, ReadInt) {
  int n = 0;
  fmt::scan("42", "{}", n);
  EXPECT_EQ(n, 42);
  fmt::scan("-42", "{}", n);
  EXPECT_EQ(n, -42);
}

TEST(ScanTest, ReadLongLong) {
  long long n = 0;
  fmt::scan("42", "{}", n);
  EXPECT_EQ(n, 42);
  fmt::scan("-42", "{}", n);
  EXPECT_EQ(n, -42);
}

TEST(ScanTest, ReadUInt) {
  unsigned n = 0;
  fmt::scan("42", "{}", n);
  EXPECT_EQ(n, 42);
  EXPECT_THROW_MSG(fmt::scan("-42", "{}", n), fmt::format_error,
                   "invalid input");
}

TEST(ScanTest, ReadULongLong) {
  unsigned long long n = 0;
  fmt::scan("42", "{}", n);
  EXPECT_EQ(n, 42);
  EXPECT_THROW_MSG(fmt::scan("-42", "{}", n), fmt::format_error,
                   "invalid input");
}

TEST(ScanTest, ReadString) {
  std::string s;
  fmt::scan("foo", "{}", s);
  EXPECT_EQ(s, "foo");
}

TEST(ScanTest, ReadStringView) {
  fmt::string_view s;
  fmt::scan("foo", "{}", s);
  EXPECT_EQ(s, "foo");
}

#ifndef _WIN32
namespace fmt {
template <> struct scanner<tm> {
  std::string format;

  scan_parse_context::iterator parse(scan_parse_context& ctx) {
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
  typename ScanContext::iterator scan(tm& t, ScanContext& ctx) {
    auto result = strptime(ctx.begin(), format.c_str(), &t);
    if (!result) throw format_error("failed to parse time");
    return result;
  }
};
}  // namespace fmt

TEST(ScanTest, ReadCustom) {
  const char* input = "Date: 1985-10-25";
  auto t = tm();
  fmt::scan(input, "Date: {0:%Y-%m-%d}", t);
  EXPECT_EQ(t.tm_year, 85);
  EXPECT_EQ(t.tm_mon, 9);
  EXPECT_EQ(t.tm_mday, 25);
}
#endif

TEST(ScanTest, InvalidFormat) {
  EXPECT_THROW_MSG(fmt::scan("", "{}"), fmt::format_error,
                   "argument index out of range");
  EXPECT_THROW_MSG(fmt::scan("", "{"), fmt::format_error,
                   "invalid format string");
}

TEST(ScanTest, Example) {
  std::string key;
  int value;
  fmt::scan("answer = 42", "{} = {}", key, value);
  EXPECT_EQ(key, "answer");
  EXPECT_EQ(value, 42);
}
