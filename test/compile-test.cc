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
  typedef fmt::internal::compiled_format_base<decltype(format)> provider;
  typedef fmt::internal::format_part<char>
      expected_parts_type[EXPECTED_PARTS_COUNT];
  static_assert(std::is_same<typename provider::parts_container,
                             expected_parts_type>::value,
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

TEST(CompileTest, MultipleTypes) {
  auto f = fmt::compile<int, int>("{} {}");
  EXPECT_EQ(fmt::format(f, 42, 42), "42 42");
}

struct formattable {};

FMT_BEGIN_NAMESPACE
template <> struct formatter<formattable> : formatter<const char*> {
  auto format(formattable, format_context& ctx) -> decltype(ctx.out()) {
    return formatter<const char*>::format("foo", ctx);
  }
};
FMT_END_NAMESPACE

TEST(CompileTest, FormatUserDefinedType) {
  auto f = fmt::compile<formattable>("{}");
  EXPECT_EQ(fmt::format(f, formattable()), "foo");
}

TEST(CompileTest, EmptyFormatString) {
  auto f = fmt::compile<>("");
  EXPECT_EQ(fmt::format(f), "");
}
