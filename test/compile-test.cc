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

FMT_BEGIN_NAMESPACE
namespace internal {
bool operator==(const internal::string_view_metadata& lhs,
                const internal::string_view_metadata& rhs) {
  return std::tie(lhs.offset_, lhs.size_) == std::tie(rhs.offset_, rhs.size_);
}
bool operator!=(const internal::string_view_metadata& lhs,
                const internal::string_view_metadata& rhs) {
  return !(lhs == rhs);
}

bool operator==(const format_part<char>::specification& lhs,
                const format_part<char>::specification& rhs) {
  if (lhs.arg_id.which != rhs.arg_id.which) {
    return false;
  }

  typedef format_part<char>::argument_id::which_arg_id which_arg_id;
  switch (lhs.arg_id.which) {
  case which_arg_id::index: {
    if (lhs.arg_id.val.index != rhs.arg_id.val.index) {
      return false;
    }
  } break;
  case which_arg_id::named_index: {
    if (lhs.arg_id.val.named_index != rhs.arg_id.val.named_index) {
      return false;
    }
  } break;
  }

  return std::tie(lhs.parsed_specs.width, lhs.parsed_specs.fill[0],
                  lhs.parsed_specs.align, lhs.parsed_specs.precision,
                  lhs.parsed_specs.sign, lhs.parsed_specs.type) ==
         std::tie(rhs.parsed_specs.width, rhs.parsed_specs.fill[0],
                  rhs.parsed_specs.align, rhs.parsed_specs.precision,
                  rhs.parsed_specs.sign, rhs.parsed_specs.type);
}

bool operator!=(const format_part<char>::specification& lhs,
                const format_part<char>::specification& rhs) {
  return !(lhs == rhs);
}

bool operator==(const format_part<char>& lhs,
                const fmt::internal::format_part<char>& rhs) {
  typedef format_part<char>::kind kind;

  if (lhs.which != rhs.which ||
      lhs.end_of_argument_id != rhs.end_of_argument_id) {
    return false;
  }

  switch (lhs.which) {
  case kind::argument_id: {
    return lhs.val.arg_id == rhs.val.arg_id;
  }

  case kind::named_argument_id: {
    return lhs.val.named_arg_id == rhs.val.named_arg_id;
  }

  case kind::text: {
    return lhs.val.text == rhs.val.text;
  }

  case kind::specification: {
    return lhs.val.spec == rhs.val.spec;
  }
  }

  return false;
}

bool operator!=(const fmt::internal::format_part<char>& lhs,
                const fmt::internal::format_part<char>& rhs) {
  return !(lhs == rhs);
}
}
FMT_END_NAMESPACE

TEST(CompileTest, FormatPart_ComparisonOperators) {
  typedef fmt::internal::format_part<char> format_part;
  typedef fmt::internal::dynamic_format_specs<char> prepared_specs;

  {
    const auto part = format_part(0u);
    const auto other = format_part(0u);
    EXPECT_EQ(part, other);
  }
  {
    const auto lhs = format_part(0u);
    const auto rhs = format_part(1u);
    EXPECT_NE(lhs, rhs);
  }
  {
    const auto lhs = format_part(fmt::internal::string_view_metadata(0, 42));
    const auto rhs = format_part(fmt::internal::string_view_metadata(0, 42));
    EXPECT_EQ(lhs, rhs);
  }
  {
    const auto lhs = format_part(fmt::internal::string_view_metadata(0, 42));
    const auto rhs = format_part(fmt::internal::string_view_metadata(0, 4422));
    EXPECT_NE(lhs, rhs);
  }
  {
    auto lhs = format_part(0u);
    auto rhs = format_part(fmt::internal::string_view_metadata(0, 42));
    EXPECT_NE(lhs, rhs);
    rhs = format_part(fmt::internal::string_view_metadata(0, 0));
    EXPECT_NE(lhs, rhs);
  }
  {
    auto lhs = format_part(0u);
    lhs.end_of_argument_id = 42;
    auto rhs = format_part(0u);
    rhs.end_of_argument_id = 42;
    EXPECT_EQ(lhs, rhs);
    rhs.end_of_argument_id = 13;
    EXPECT_NE(lhs, rhs);
  }
  {
    const auto specs_argument_id = 0u;
    const auto specs_named_argument_id =
        fmt::internal::string_view_metadata(0, 42);
    auto specs = format_part::specification(0u);
    auto lhs = format_part(specs);
    auto rhs = format_part(specs);
    EXPECT_EQ(lhs, rhs);

    specs.parsed_specs = prepared_specs();
    lhs = format_part(specs);
    rhs = format_part(specs);
    EXPECT_EQ(lhs, rhs);

    specs = format_part::specification(specs_named_argument_id);
    lhs = format_part(specs);
    rhs = format_part(specs);
    EXPECT_EQ(lhs, rhs);

    specs.parsed_specs = prepared_specs();
    lhs = format_part(specs);
    rhs = format_part(specs);
    EXPECT_EQ(lhs, rhs);

    auto lhs_spec = format_part::specification(specs_argument_id);
    auto rhs_spec = format_part::specification(specs_named_argument_id);
    lhs = format_part(lhs_spec);
    rhs = format_part(rhs_spec);
    EXPECT_NE(lhs, rhs);

    lhs_spec = format_part::specification(specs_argument_id);
    rhs_spec = format_part::specification(specs_argument_id);
    lhs_spec.parsed_specs.precision = 1;
    rhs_spec.parsed_specs.precision = 2;
    lhs = format_part(lhs_spec);
    rhs = format_part(rhs_spec);
    EXPECT_NE(lhs, rhs);
  }
  {
    const auto specs_argument_id = 0u;
    const auto specs_named_argument_id =
        fmt::internal::string_view_metadata(0, 42);
    auto specs = format_part::specification(specs_argument_id);
    auto lhs = format_part(specs);
    auto rhs = format_part(0u);
    auto rhs2 = format_part(fmt::internal::string_view_metadata(0, 42));
    EXPECT_NE(lhs, rhs);
    EXPECT_NE(lhs, rhs2);

    specs.parsed_specs = prepared_specs();
    lhs = format_part{specs};
    EXPECT_NE(lhs, rhs);
    EXPECT_NE(lhs, rhs2);

    specs = format_part::specification(specs_named_argument_id);
    EXPECT_NE(lhs, rhs);
    EXPECT_NE(lhs, rhs2);

    specs.parsed_specs = prepared_specs();
    lhs = format_part(specs);
    EXPECT_NE(lhs, rhs);
    EXPECT_NE(lhs, rhs2);
  }
}

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
