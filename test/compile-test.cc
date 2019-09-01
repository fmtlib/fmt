// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include <stdint.h>
#include <cctype>
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstring>
#include <deque>
#include <list>
#include <memory>
#include <string>

// Check if fmt/compile.h compiles with windows.h included before it.
#ifdef _WIN32
#  include <windows.h>
#endif

#include "fmt/compile.h"
#include "gmock.h"
#include "gtest-extra.h"
#include "mock-allocator.h"
#include "util.h"

#undef ERROR
#undef min
#undef max

using testing::Return;
using testing::StrictMock;

// compiletime_prepared_parts_type_provider is useful only with relaxed
// constexpr.
#if FMT_USE_CONSTEXPR
template <unsigned EXPECTED_PARTS_COUNT, typename Format>
void check_prepared_parts_type(Format format) {
  typedef fmt::internal::compiletime_prepared_parts_type_provider<decltype(
      format)>
      provider;
  typedef typename provider::template format_parts_array<EXPECTED_PARTS_COUNT>
      expected_parts_type;
  static_assert(
      std::is_same<typename provider::type, expected_parts_type>::value,
      "CompileTimePreparedPartsTypeProvider test failed");
}

TEST(CompileTest, CompileTimePreparedPartsTypeProvider) {
  check_prepared_parts_type<1u>(FMT_STRING("text"));
  check_prepared_parts_type<1u>(FMT_STRING("{}"));
  check_prepared_parts_type<2u>(FMT_STRING("text{}"));
  check_prepared_parts_type<2u>(FMT_STRING("{}text"));
  check_prepared_parts_type<3u>(FMT_STRING("text{}text"));
  check_prepared_parts_type<3u>(FMT_STRING("{:{}.{}} {:{}}"));

  check_prepared_parts_type<3u>(FMT_STRING("{{{}}}"));   // '{', 'argument', '}'
  check_prepared_parts_type<2u>(FMT_STRING("text{{"));   // 'text', '{'
  check_prepared_parts_type<3u>(FMT_STRING("text{{ "));  // 'text', '{', ' '
  check_prepared_parts_type<2u>(FMT_STRING("}}text"));   // '}', text
  check_prepared_parts_type<2u>(FMT_STRING("text}}text"));  // 'text}', 'text'
  check_prepared_parts_type<4u>(
      FMT_STRING("text{{}}text"));  // 'text', '{', '}', 'text'
}
#endif

class custom_parts_container {
 public:
  typedef fmt::internal::format_part<char> format_part_type;

 private:
  typedef std::deque<format_part_type> parts;

 public:
  void add(format_part_type part) { parts_.push_back(std::move(part)); }

  void substitute_last(format_part_type part) {
    parts_.back() = std::move(part);
  }

  format_part_type last() { return parts_.back(); }

  auto begin() -> decltype(std::declval<parts>().begin()) {
    return parts_.begin();
  }

  auto begin() const -> decltype(std::declval<const parts>().begin()) {
    return parts_.begin();
  }

  auto end() -> decltype(std::declval<parts>().begin()) { return parts_.end(); }

  auto end() const -> decltype(std::declval<const parts>().begin()) {
    return parts_.end();
  }

 private:
  parts parts_;
};

TEST(CompileTest, PassStringLiteralFormat) {
  const auto prepared = fmt::compile<int>("test {}");
  EXPECT_EQ("test 42", fmt::format(prepared, 42));
  const auto wprepared = fmt::compile<int>(L"test {}");
  EXPECT_EQ(L"test 42", fmt::format(wprepared, 42));
}

#if FMT_USE_CONSTEXPR
TEST(CompileTest, PassCompileString) {
  const auto prepared = fmt::compile<int>(FMT_STRING("test {}"));
  EXPECT_EQ("test 42", fmt::format(prepared, 42));
  const auto wprepared = fmt::compile<int>(FMT_STRING(L"test {}"));
  EXPECT_EQ(L"test 42", fmt::format(wprepared, 42));
}
#endif

TEST(CompileTest, FormatToArrayOfChars) {
  char buffer[32] = {0};
  const auto prepared = fmt::compile<int>("4{}");
  fmt::format_to(fmt::internal::make_checked(buffer, 32), prepared, 2);
  EXPECT_EQ(std::string("42"), buffer);
  wchar_t wbuffer[32] = {0};
  const auto wprepared = fmt::compile<int>(L"4{}");
  fmt::format_to(fmt::internal::make_checked(wbuffer, 32), wprepared, 2);
  EXPECT_EQ(std::wstring(L"42"), wbuffer);
}

TEST(CompileTest, FormatToIterator) {
  std::string s(2, ' ');
  const auto prepared = fmt::compile<int>("4{}");
  fmt::format_to(s.begin(), prepared, 2);
  EXPECT_EQ("42", s);
  std::wstring ws(2, L' ');
  const auto wprepared = fmt::compile<int>(L"4{}");
  fmt::format_to(ws.begin(), wprepared, 2);
  EXPECT_EQ(L"42", ws);
}

TEST(CompileTest, FormatToN) {
  char buf[5];
  auto f = fmt::compile<int>("{:10}");
  auto result = fmt::format_to_n(buf, 5, f, 42);
  EXPECT_EQ(result.size, 10);
  EXPECT_EQ(result.out, buf + 5);
  EXPECT_EQ(fmt::string_view(buf, 5), "     ");
}

TEST(CompileTest, FormattedSize) {
  auto f = fmt::compile<int>("{:10}");
  EXPECT_EQ(fmt::formatted_size(f, 42), 10);
}

struct formattable {};

template <>
struct fmt::formatter<formattable> : formatter<const char*> {
  auto format(formattable, format_context& ctx) -> decltype(ctx.out()) {
    return formatter<const char*>::format("foo", ctx);
  }
};

TEST(CompileTest, FormatUserDefinedType) {
  auto f = fmt::compile<formattable>("{}");
  EXPECT_EQ(fmt::format(f, formattable()), "foo");
}
