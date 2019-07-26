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

class mock_parts_collector {
 public:
  MOCK_METHOD1(add, void(fmt::internal::format_part<char>));
  MOCK_METHOD1(substitute_last, void(fmt::internal::format_part<char>));
  MOCK_METHOD0(last, fmt::internal::format_part<char>());
};

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
  typedef format_part<char>::which_value which_value;

  if (lhs.which != rhs.which ||
      lhs.end_of_argument_id != rhs.end_of_argument_id) {
    return false;
  }

  switch (lhs.which) {
  case which_value::argument_id: {
    return lhs.val.arg_id == rhs.val.arg_id;
  }

  case which_value::named_argument_id: {
    return lhs.val.named_arg_id == rhs.val.named_arg_id;
  }

  case which_value::text: {
    return lhs.val.text == rhs.val.text;
  }

  case which_value::specification: {
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

TEST(PrepareTest, FormatPart_ComparisonOperators) {
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

TEST(PrepareTest, FormatPreparationHandler_OnText_AddsPartWithText) {
  typedef fmt::internal::format_part<char> format_part;
  typedef StrictMock<mock_parts_collector> parts_mock;

  parts_mock parts;
  const auto format = fmt::internal::to_string_view("text");
  fmt::internal::format_preparation_handler<char, parts_mock> handler(format,
                                                                      parts);

  const auto expected_text = fmt::internal::string_view_metadata(
      0u, static_cast<unsigned>(format.size()));
  EXPECT_CALL(parts, add(format_part(expected_text)));

  handler.on_text(format.begin(), format.end());
}

TEST(PrepareTest, FormatPreparationHandler_OnArgId_AddsPartWithIncrementedId) {
  typedef fmt::internal::format_part<char> format_part;
  typedef StrictMock<mock_parts_collector> parts_mock;

  parts_mock parts;
  const auto format = fmt::internal::to_string_view("");
  fmt::internal::format_preparation_handler<char, parts_mock> handler(format,
                                                                      parts);

  const auto expected_first_arg_id = 0u;
  const auto expected_second_arg_id = 1u;
  EXPECT_CALL(parts, add(format_part(expected_first_arg_id)));
  EXPECT_CALL(parts, add(format_part(expected_second_arg_id)));

  handler.on_arg_id();
  handler.on_arg_id();
}

TEST(PrepareTest, FormatPreparationHandler_OnArgId_AddsPartWithPassedId) {
  typedef fmt::internal::format_part<char> format_part;
  typedef StrictMock<mock_parts_collector> parts_mock;

  parts_mock parts;
  const auto format = fmt::internal::to_string_view("");
  fmt::internal::format_preparation_handler<char, parts_mock> handler(format,
                                                                      parts);

  const auto expected_first_arg_id = 2u;
  const auto expected_second_arg_id = 0u;
  const auto expected_third_arg_id = 1u;
  EXPECT_CALL(parts, add(format_part(expected_first_arg_id)));
  EXPECT_CALL(parts, add(format_part(expected_second_arg_id)));
  EXPECT_CALL(parts, add(format_part(expected_third_arg_id)));

  handler.on_arg_id(expected_first_arg_id);
  handler.on_arg_id(expected_second_arg_id);
  handler.on_arg_id(expected_third_arg_id);
}

TEST(PrepareTest, FormatPreparationHandler_OnArgId_AddsPartWithPassedNamedId) {
  typedef fmt::internal::format_part<char> format_part;
  typedef format_part::named_argument_id named_argument_id;
  typedef StrictMock<mock_parts_collector> parts_mock;

  parts_mock parts;
  const auto format = fmt::internal::to_string_view("0123456789");
  fmt::internal::format_preparation_handler<char, parts_mock> handler(format,
                                                                      parts);

  const auto expected_first_arg_id = fmt::string_view(format.data(), 1);
  const auto expected_first_arg_view_metadata =
      fmt::internal::string_view_metadata(0, 1);
  const auto expected_second_arg_id = fmt::string_view(format.data() + 3, 2);
  const auto expected_second_arg_view_metadata =
      fmt::internal::string_view_metadata(3, 2);
  const auto expected_third_arg_id = fmt::string_view(format.data() + 6, 3);
  const auto expected_third_arg_view_metadata =
      fmt::internal::string_view_metadata(6, 3);
  EXPECT_CALL(
      parts,
      add(format_part(named_argument_id(expected_first_arg_view_metadata))));
  EXPECT_CALL(
      parts,
      add(format_part(named_argument_id(expected_second_arg_view_metadata))));
  EXPECT_CALL(
      parts,
      add(format_part(named_argument_id(expected_third_arg_view_metadata))));

  handler.on_arg_id(expected_first_arg_id);
  handler.on_arg_id(expected_second_arg_id);
  handler.on_arg_id(expected_third_arg_id);
}

TEST(PrepareTest,
     FormatPreparationHandler_OnReplacementField_SetsEndOfArgumentId) {
  typedef fmt::internal::format_part<char> format_part;
  typedef StrictMock<mock_parts_collector> parts_mock;

  const auto format = fmt::internal::to_string_view("{:<}");
  parts_mock parts;

  const auto last_part = format_part(0u);
  EXPECT_CALL(parts, last()).WillOnce(Return(last_part));

  auto expected_substitution_part = last_part;
  expected_substitution_part.end_of_argument_id = 1;
  EXPECT_CALL(parts, substitute_last(expected_substitution_part));

  fmt::internal::format_preparation_handler<char, parts_mock> handler(format,
                                                                      parts);

  handler.on_replacement_field(format.data() + 1);
}

TEST(
    PrepareTest,
    FormatPreparationHandlerLastPartArgIndex_OnFormatSpecs_UpdatesLastAddedPart) {
  typedef fmt::internal::format_part<char> format_part;
  typedef StrictMock<mock_parts_collector> parts_mock;

  parts_mock parts;
  const auto specification_test_text = fmt::internal::to_string_view("{:<10}");
  const auto specification_offset = 2u;
  const auto specification_begin_it =
      specification_test_text.begin() + specification_offset;
  fmt::internal::format_preparation_handler<char, parts_mock> handler(
      specification_test_text, parts);

  const auto last_part = format_part(0u);
  format_part::specification expected_specification(0u);
  fmt::internal::dynamic_format_specs<char> specs{};
  specs.align = fmt::align::left;
  specs.width = 10;
  expected_specification.parsed_specs = specs;

  auto expected_substitution_part = format_part(expected_specification);
  expected_substitution_part.end_of_argument_id = specification_offset;

  EXPECT_CALL(parts, last()).WillOnce(Return(last_part));
  EXPECT_CALL(parts, substitute_last(expected_substitution_part));

  handler.on_format_specs(specification_begin_it,
                          specification_test_text.end());
}

TEST(
    PrepareTest,
    FormatPreparationHandlerLastPartNamedArgIndex_OnFormatSpecs_UpdatesLastAddedPart) {
  typedef fmt::internal::format_part<char> format_part;
  typedef StrictMock<mock_parts_collector> parts_mock;

  parts_mock parts;
  const auto specification_test_text = fmt::internal::to_string_view("{:<10}");
  const auto specification_offset = 2u;
  const auto specification_begin_it =
      specification_test_text.begin() + specification_offset;
  fmt::internal::format_preparation_handler<char, parts_mock> handler(
      specification_test_text, parts);

  const auto arg_id = fmt::internal::string_view_metadata(0, 42);
  const auto last_part = format_part(format_part::named_argument_id(arg_id));
  format_part::specification expected_specification(arg_id);
  fmt::internal::dynamic_format_specs<char> specs{};
  specs.align = fmt::align::left;
  specs.width = 10;
  expected_specification.parsed_specs = specs;

  auto expected_substitution_part = format_part(expected_specification);
  expected_substitution_part.end_of_argument_id = specification_offset;

  EXPECT_CALL(parts, last()).WillOnce(Return(last_part));
  EXPECT_CALL(parts, substitute_last(expected_substitution_part));

  handler.on_format_specs(specification_begin_it,
                          specification_test_text.end());
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

TEST(PrepareTest, CompileTimePreparedPartsTypeProvider) {
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

// Use the struct instead of a function to workaround GCC 4.4's 'sorry,
// unimplemented: mangling template_id_expr' issue.
template <typename... Args> struct copied_prepared_format_creator {
  static decltype(fmt::compile<Args...>(std::declval<std::string>())) make(
      std::string format_str) {
    auto prepared_format = fmt::compile<Args...>(std::move(format_str));
    auto copied_prepared_format = prepared_format;
    prepared_format = fmt::compile<Args...>("");

    return copied_prepared_format;
  }
};

TEST(PrepareTest, CopyPreparedFormat_InternalStringViewsAreNotInvalidated) {
  auto prepared = copied_prepared_format_creator<int, std::string>::make(
      "before {} middle {} after");
  EXPECT_EQ("before 42 middle text after", prepared.format(42, "text"));

  prepared = copied_prepared_format_creator<int, std::string>::make(
      "before {0} middle {1} after");
  EXPECT_EQ("before 42 middle text after", prepared.format(42, "text"));

  {
    typedef decltype(fmt::arg("first", 42)) argument0;
    typedef decltype(fmt::arg("second", "text")) argument1;
    auto named_prepared =
        copied_prepared_format_creator<argument0, argument1>::make(
            "before {first} middle {second} after");
    EXPECT_EQ("before 42 middle text after",
              named_prepared.format(fmt::arg("first", 42),
                                    fmt::arg("second", "text")));
  }
  {
    typedef decltype(fmt::arg("value", "12345")) argument0;
    typedef decltype(fmt::arg("width", 10)) argument1;
    auto named_prepared =
        copied_prepared_format_creator<argument0, argument1>::make(
            ">>>{value:>{width}}<<<");
    EXPECT_EQ(">>>     12345<<<",
              named_prepared.format(fmt::arg("value", "12345"),
                                    fmt::arg("width", 10)));
  }
}

TEST(PrepareTest, ReusedPreparedFormatType) {
  using prepared_format = fmt::internal::prepared_format_t<std::string, int>;

  prepared_format prepared = fmt::compile<prepared_format>("The {} is {}.");
  EXPECT_EQ("The answer is 42.", prepared.format("answer", 42));
  prepared = fmt::compile<prepared_format>("40 {} 2 = {}");
  EXPECT_EQ("40 + 2 = 42", prepared.format("+", 42));
}

TEST(PrepareTest, UserProvidedPartsContainerUnderlyingContainer) {
  typedef fmt::internal::format_part<char> format_part;
  typedef fmt::internal::parts_container<char, std::list<format_part>>
      parts_container;
  typedef fmt::internal::basic_prepared_format<std::string, parts_container, std::string,
                                     int>::type prepared_format;

  prepared_format prepared = fmt::compile<prepared_format>("The {} is {}.");
  EXPECT_EQ("The answer is 42.", prepared.format("answer", 42));
  prepared = fmt::compile<prepared_format>("40 {} 2 = {}");
  EXPECT_EQ("40 + 2 = 42", prepared.format("+", 42));
}

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

TEST(PrepareTest, UserProvidedPartsContainer) {
  typedef fmt::internal::basic_prepared_format<std::string, custom_parts_container,
                                     std::string, int>::type prepared_format;

  prepared_format prepared = fmt::compile<prepared_format>("The {} is {}.");
  EXPECT_EQ("The answer is 42.", prepared.format("answer", 42));
  prepared = fmt::compile<prepared_format>("40 {} 2 = {}");
  EXPECT_EQ("40 + 2 = 42", prepared.format("+", 42));
}

TEST(PrepareTest, PassConstCharPointerFormat) {
  const char* c_format = "test {}";
  const auto prepared = fmt::compile<int>(c_format);
  EXPECT_EQ("test 42", prepared.format(42));
  const wchar_t* wc_format = L"test {}";
  const auto wprepared = fmt::compile<int>(wc_format);
  EXPECT_EQ(L"test 42", wprepared.format(42));
}

TEST(PrepareTest, PassCharArrayFormat) {
  char c_format[] = "test {}";
  const auto prepared = fmt::compile<int>(c_format);
  EXPECT_EQ("test 42", prepared.format(42));
  wchar_t wc_format[] = L"test {}";
  const auto wprepared = fmt::compile<int>(wc_format);
  EXPECT_EQ(L"test 42", wprepared.format(42));
}

TEST(PrepareTest, PassConstCharArrayFormat) {
  const char c_format[] = "test {}";
  const auto prepared = fmt::compile<int>(c_format);
  EXPECT_EQ("test 42", prepared.format(42));
  const wchar_t wc_format[] = L"test {}";
  const auto wprepared = fmt::compile<int>(wc_format);
  EXPECT_EQ(L"test 42", wprepared.format(42));
}

TEST(PrepareTest, PassStringLiteralFormat) {
  const auto prepared = fmt::compile<int>("test {}");
  EXPECT_EQ("test 42", prepared.format(42));
  const auto wprepared = fmt::compile<int>(L"test {}");
  EXPECT_EQ(L"test 42", wprepared.format(42));
}

TEST(PrepareTest, PassStringViewFormat) {
  const auto prepared =
      fmt::compile<int>(fmt::basic_string_view<char>("test {}"));
  EXPECT_EQ("test 42", prepared.format(42));
  const auto wprepared =
      fmt::compile<int>(fmt::basic_string_view<wchar_t>(L"test {}"));
  EXPECT_EQ(L"test 42", wprepared.format(42));
}

TEST(PrepareTest, PassBasicStringFormat) {
  const auto prepared = fmt::compile<int>(std::string("test {}"));
  EXPECT_EQ("test 42", prepared.format(42));
  const auto wprepared = fmt::compile<int>(std::wstring(L"test {}"));
  EXPECT_EQ(L"test 42", wprepared.format(42));
}

#if FMT_USE_CONSTEXPR
TEST(PrepareTest, PassCompileString) {
  const auto prepared = fmt::compile<int>(FMT_STRING("test {}"));
  EXPECT_EQ("test 42", prepared.format(42));
  const auto wprepared = fmt::compile<int>(FMT_STRING(L"test {}"));
  EXPECT_EQ(L"test 42", wprepared.format(42));
}
#endif

template <typename T> struct user_allocator {
  typedef T value_type;
  typedef value_type* pointer;
  typedef const value_type* const_pointer;
  typedef value_type& reference;
  typedef const value_type& const_reference;
  typedef std::size_t size_type;
  typedef std::ptrdiff_t difference_type;

  template <typename U> struct rebind { typedef user_allocator<U> other; };

  user_allocator() = default;
  ~user_allocator() = default;
  template <typename U> user_allocator(const user_allocator<U>&) {}

  pointer allocate(
      size_type cnt,
      typename std::allocator<fmt::monostate>::const_pointer = nullptr) {
    return new value_type[cnt];
  }

  void deallocate(pointer p, size_type) { delete[] p; }

  void construct(pointer p, const value_type& val) { new (p) value_type(val); }

  void destroy(pointer p) { (*p).~value_type(); }

  bool operator==(const user_allocator&) const { return true; }
  bool operator!=(const user_allocator&) const { return false; }
};

TEST(PrepareTest, PassUserTypeFormat) {
  typedef std::basic_string<char, std::char_traits<char>, user_allocator<char>>
      user_format;
  const auto prepared = fmt::compile<int>(user_format("test {}"));
  EXPECT_EQ("test 42", prepared.format(42));
}

TEST(PrepareTest, FormatToArrayOfChars) {
  char buffer[32] = {0};
  const auto prepared = fmt::compile<int>("4{}");
  prepared.format_to(buffer, 2);
  EXPECT_EQ(std::string("42"), buffer);
  wchar_t wbuffer[32] = {0};
  const auto wprepared = fmt::compile<int>(L"4{}");
  wprepared.format_to(wbuffer, 2);
  EXPECT_EQ(std::wstring(L"42"), wbuffer);
}

TEST(PrepareTest, FormatToIterator) {
  std::string s(2, ' ');
  const auto prepared = fmt::compile<int>("4{}");
  prepared.format_to(s.begin(), 2);
  EXPECT_EQ("42", s);
  std::wstring ws(2, L' ');
  const auto wprepared = fmt::compile<int>(L"4{}");
  wprepared.format_to(ws.begin(), 2);
  EXPECT_EQ(L"42", ws);
}

TEST(PrepareTest, FormatToBackInserter) {
  std::string s;
  const auto prepared = fmt::compile<int>("4{}");
  prepared.format_to(std::back_inserter(s), 2);
  EXPECT_EQ("42", s);
  std::wstring ws;
  const auto wprepared = fmt::compile<int>(L"4{}");
  wprepared.format_to(std::back_inserter(ws), 2);
  EXPECT_EQ(L"42", ws);
}

TEST(PrepareTest, FormatToBasicMemoryBuffer) {
  fmt::basic_memory_buffer<char, 100> buffer;
  const auto prepared = fmt::compile<int>("4{}");
  prepared.format_to(buffer, 2);
  EXPECT_EQ("42", to_string(buffer));
  fmt::basic_memory_buffer<wchar_t, 100> wbuffer;
  const auto wprepared = fmt::compile<int>(L"4{}");
  wprepared.format_to(wbuffer, 2);
  EXPECT_EQ(L"42", to_string(wbuffer));
}

TEST(PrepareTest, FormatToMemoryBuffer) {
  fmt::memory_buffer buffer;
  const auto prepared = fmt::compile<int>("4{}");
  prepared.format_to(buffer, 2);
  EXPECT_EQ("42", to_string(buffer));
  fmt::wmemory_buffer wbuffer;
  const auto wprepared = fmt::compile<int>(L"4{}");
  wprepared.format_to(wbuffer, 2);
  EXPECT_EQ(L"42", to_string(wbuffer));
}
