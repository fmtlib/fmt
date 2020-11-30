// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include <list>
#include <iterator>
#include <string>
#include <utility>

#ifdef WIN32
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include "fmt/format.h"
#include "fmt/locale.h"
#include "fmt/chrono.h"
#include "fmt/color.h"
#include "fmt/ostream.h"

#undef index

#include "gmock.h"
#include "gtest-extra.h"
#include "mock-allocator.h"
#include "util.h"

#undef ERROR

using fmt::basic_memory_buffer;
using fmt::format;
using fmt::format_error;
using fmt::memory_buffer;
using fmt::string_view;
using fmt::wmemory_buffer;
using fmt::wstring_view;
using fmt::detail::max_value;

using testing::Return;
using testing::StrictMock;

class Answer {};

FMT_BEGIN_NAMESPACE
template <> struct formatter<Answer> : formatter<int> {
  template <typename FormatContext>
  auto format(Answer, FormatContext& ctx) -> decltype(ctx.out()) {
    return formatter<int>::format(42, ctx);
  }
};
FMT_END_NAMESPACE

struct string_like {};
fmt::string_view to_string_view(string_like) { return "foo"; }

constexpr char with_null[3] = {'{', '}', '\0'};
constexpr char no_null[2] = {'{', '}'};
static FMT_CONSTEXPR_DECL const char static_with_null[3] = {'{', '}', '\0'};
static FMT_CONSTEXPR_DECL const wchar_t static_with_null_wide[3] = {'{', '}',
                                                                    '\0'};
static FMT_CONSTEXPR_DECL const char static_no_null[2] = {'{', '}'};
static FMT_CONSTEXPR_DECL const wchar_t static_no_null_wide[2] = {'{', '}'};

TEST(FormatTest, CompileTimeString) {
  EXPECT_EQ("42", fmt::format(FMT_STRING("{}"), 42));
  EXPECT_EQ(L"42", fmt::format(FMT_STRING(L"{}"), 42));
  EXPECT_EQ("foo", fmt::format(FMT_STRING("{}"), string_like()));

  (void)static_with_null;
  (void)static_with_null_wide;
  (void)static_no_null;
  (void)static_no_null_wide;
#if !defined(_MSC_VER)
  EXPECT_EQ("42", fmt::format(FMT_STRING(static_with_null), 42));
  EXPECT_EQ(L"42", fmt::format(FMT_STRING(static_with_null_wide), 42));
  EXPECT_EQ("42", fmt::format(FMT_STRING(static_no_null), 42));
  EXPECT_EQ(L"42", fmt::format(FMT_STRING(static_no_null_wide), 42));
#endif

  (void)with_null;
  (void)no_null;
#if __cplusplus >= 201703L
  EXPECT_EQ("42", fmt::format(FMT_STRING(with_null), 42));
  EXPECT_EQ("42", fmt::format(FMT_STRING(no_null), 42));
#endif
#if defined(FMT_USE_STRING_VIEW) && __cplusplus >= 201703L
  EXPECT_EQ("42", fmt::format(FMT_STRING(std::string_view("{}")), 42));
  EXPECT_EQ(L"42", fmt::format(FMT_STRING(std::wstring_view(L"{}")), 42));
#endif
}

TEST(FormatTest, CustomFormatCompileTimeString) {
  EXPECT_EQ("42", fmt::format(FMT_STRING("{}"), Answer()));
  Answer answer;
  EXPECT_EQ("42", fmt::format(FMT_STRING("{}"), answer));
  char buf[10] = {};
  fmt::format_to(buf, FMT_STRING("{}"), answer);
  const Answer const_answer = Answer();
  EXPECT_EQ("42", fmt::format(FMT_STRING("{}"), const_answer));
}

#if FMT_USE_USER_DEFINED_LITERALS
// Passing user-defined literals directly to EXPECT_EQ causes problems
// with macro argument stringification (#) on some versions of GCC.
// Workaround: Assing the UDL result to a variable before the macro.

using namespace fmt::literals;

TEST(LiteralsTest, Format) {
  auto udl_format = "{}c{}"_format("ab", 1);
  EXPECT_EQ("abc1", udl_format);
  auto udl_format_w = L"{}c{}"_format(L"ab", 1);
  EXPECT_EQ(L"abc1", udl_format_w);
}

#endif  // FMT_USE_USER_DEFINED_LITERALS

enum TestEnum { A };

namespace adl_test {
namespace fmt {
namespace detail {
struct foo {};
template <typename, typename OutputIt> void write(OutputIt, foo) = delete;
}  // namespace detail
}  // namespace fmt
}  // namespace adl_test

FMT_BEGIN_NAMESPACE
template <>
struct formatter<adl_test::fmt::detail::foo> : formatter<std::string> {
  template <typename FormatContext>
  auto format(adl_test::fmt::detail::foo, FormatContext& ctx)
      -> decltype(ctx.out()) {
    return formatter<std::string>::format("foo", ctx);
  }
};
FMT_END_NAMESPACE

TEST(FormatTest, ToString) {
  EXPECT_EQ("42", fmt::to_string(42));
  EXPECT_EQ("0x1234", fmt::to_string(reinterpret_cast<void*>(0x1234)));
  EXPECT_EQ("foo", fmt::to_string(adl_test::fmt::detail::foo()));
}

TEST(FormatTest, ToWString) { EXPECT_EQ(L"42", fmt::to_wstring(42)); }

TEST(FormatTest, OutputIterators) {
  std::list<char> out;
  fmt::format_to(std::back_inserter(out), FMT_STRING("{}"), 42);
  EXPECT_EQ("42", std::string(out.begin(), out.end()));
  std::stringstream s;
  fmt::format_to(std::ostream_iterator<char>(s), FMT_STRING("{}"), 42);
  EXPECT_EQ("42", s.str());
}

TEST(FormatTest, FormattedSize) {
  //EXPECT_EQ(2u, fmt::formatted_size(FMT_STRING("{}"), 42));
}

TEST(FormatTest, FormatTo) {
  std::vector<char> v;
  fmt::format_to(std::back_inserter(v), FMT_STRING("{}"), "foo");
  EXPECT_EQ(fmt::string_view(v.data(), v.size()), "foo");
}

TEST(FormatTest, FormatToN) {
  char buffer[4];
  buffer[3] = 'x';
  auto result = fmt::format_to_n(buffer, 3, FMT_STRING("{}"), 12345);
  EXPECT_EQ(5u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("123x", fmt::string_view(buffer, 4));

  result = fmt::format_to_n(buffer, 3, FMT_STRING("{:s}"), "foobar");
  EXPECT_EQ(6u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("foox", fmt::string_view(buffer, 4));

  buffer[0] = 'x';
  buffer[1] = 'x';
  buffer[2] = 'x';
  result = fmt::format_to_n(buffer, 3, FMT_STRING("{}"), 'A');
  EXPECT_EQ(1u, result.size);
  EXPECT_EQ(buffer + 1, result.out);
  EXPECT_EQ("Axxx", fmt::string_view(buffer, 4));

  result = fmt::format_to_n(buffer, 3, FMT_STRING("{}{} "), 'B', 'C');
  EXPECT_EQ(3u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("BC x", fmt::string_view(buffer, 4));

  result = fmt::format_to_n(buffer, 4, FMT_STRING("{}"), "ABCDE");
  EXPECT_EQ(5u, result.size);
  EXPECT_EQ("ABCD", fmt::string_view(buffer, 4));

  buffer[3] = 'x';
  result = fmt::format_to_n(buffer, 3, FMT_STRING("{}"), std::string(1000, '*'));
  EXPECT_EQ(1000u, result.size);
  EXPECT_EQ("***x", fmt::string_view(buffer, 4));
}

TEST(FormatTest, WideFormatToN) {
  wchar_t buffer[4];
  buffer[3] = L'x';
  auto result = fmt::format_to_n(buffer, 3, FMT_STRING(L"{}"), 12345);
  EXPECT_EQ(5u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ(L"123x", fmt::wstring_view(buffer, 4));
  buffer[0] = L'x';
  buffer[1] = L'x';
  buffer[2] = L'x';
  result = fmt::format_to_n(buffer, 3, FMT_STRING(L"{}"), L'A');
  EXPECT_EQ(1u, result.size);
  EXPECT_EQ(buffer + 1, result.out);
  EXPECT_EQ(L"Axxx", fmt::wstring_view(buffer, 4));
  result = fmt::format_to_n(buffer, 3, FMT_STRING(L"{}{} "), L'B', L'C');
  EXPECT_EQ(3u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ(L"BC x", fmt::wstring_view(buffer, 4));
}

struct test_output_iterator {
  char* data;

  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = void;
  using pointer = void;
  using reference = void;

  test_output_iterator& operator++() {
    ++data;
    return *this;
  }
  test_output_iterator operator++(int) {
    auto tmp = *this;
    ++data;
    return tmp;
  }
  char& operator*() { return *data; }
};

TEST(FormatTest, FormatToNOutputIterator) {
  char buf[10] = {};
  fmt::format_to_n(test_output_iterator{buf}, 10, FMT_STRING("{}"), 42);
  EXPECT_STREQ(buf, "42");
}

TEST(FormatTest, VFormatTo) {
  typedef fmt::format_context context;
  fmt::basic_format_arg<context> arg = fmt::detail::make_arg<context>(42);
  fmt::basic_format_args<context> args(&arg, 1);
  std::string s;
  fmt::vformat_to(std::back_inserter(s), FMT_STRING("{}"), args);
  EXPECT_EQ("42", s);

  typedef fmt::wformat_context wcontext;
  fmt::basic_format_arg<wcontext> warg = fmt::detail::make_arg<wcontext>(42);
  fmt::basic_format_args<wcontext> wargs(&warg, 1);
  std::wstring w;
  fmt::vformat_to(std::back_inserter(w), FMT_STRING(L"{}"), wargs);
  EXPECT_EQ(L"42", w);
}

template <typename T> static std::string FmtToString(const T& t) {
  return fmt::format(FMT_STRING("{}"), t);
}

TEST(FormatTest, FmtStringInTemplate) {
  EXPECT_EQ(FmtToString(1), "1");
  EXPECT_EQ(FmtToString(0), "0");
}



#include "fmt/chrono.h"

#ifndef FMT_STATIC_THOUSANDS_SEPARATOR
/*
TEST(ChronoTest, FormatDefault) {
  EXPECT_EQ("42s", fmt::format(FMT_STRING("{}"), std::chrono::seconds(42)));
}

TEST(ChronoTest, FormatWide) {
  EXPECT_EQ(L"42s", fmt::format(L"{}", std::chrono::seconds(42)));
}

typedef std::chrono::duration<double, std::milli> dms;

TEST(ChronoTest, FormatDefaultFP) {
  EXPECT_EQ("1.234ms", fmt::format(FMT_STRING("{}"), dms(1.234)));
}

TEST(ChronoTest, FormatPrecision) {
  EXPECT_EQ("1.2ms", fmt::format(FMT_STRING("{:.1}"), dms(1.234)));
  EXPECT_EQ("1.23ms", fmt::format(FMT_STRING("{:.{}}"), dms(1.234), 2));
}

TEST(ChronoTest, FormatFullSpecs) {
  EXPECT_EQ("1.2ms ", fmt::format(FMT_STRING("{:6.1}"), dms(1.234)));
  EXPECT_EQ(" 1.2ms ", fmt::format(FMT_STRING("{:^{}.{}}"), dms(1.234), 7, 1));
}

TEST(ChronoTest, FormatSimpleQq) {
  typedef std::chrono::duration<float> fs;
  //EXPECT_EQ("1.234 s", fmt::format(FMT_STRING("{:%Q %q}"), fs(1.234)));
  typedef std::chrono::duration<float, std::milli> fms;
  EXPECT_EQ("1.234 ms", fmt::format(FMT_STRING("{:%Q %q}"), fms(1.234)));
  typedef std::chrono::duration<double> ds;
  EXPECT_EQ("1.234 s", fmt::format(FMT_STRING("{:%Q %q}"), ds(1.234)));
  EXPECT_EQ("1.234 ms", fmt::format(FMT_STRING("{:%Q %q}"), dms(1.234)));
}

TEST(ChronoTest, FormatPrecisionQq) {
  EXPECT_EQ("1.2 ms", fmt::format(FMT_STRING("{:.1%Q %q}"), dms(1.234)));
  EXPECT_EQ("1.23 ms", fmt::format(FMT_STRING("{:.{}%Q %q}"), dms(1.234), 2));
}

TEST(ChronoTest, FormatFullSpecsQq) {
  EXPECT_EQ("1.2 ms ", fmt::format(FMT_STRING("{:7.1%Q %q}"), dms(1.234)));
  EXPECT_EQ(" 1.2 ms ", fmt::format(FMT_STRING("{:^{}.{}%Q %q}"), dms(1.234), 8, 1));
}

TEST(ChronoTest, UnsignedDuration) {
  EXPECT_EQ("42s", fmt::format(FMT_STRING("{}"), std::chrono::duration<unsigned>(42)));
}
*/
#endif  // FMT_STATIC_THOUSANDS_SEPARATOR


TEST(ColorsTest, ColorsPrint) {
  EXPECT_WRITE(stdout, fmt::print(fg(fmt::rgb(255, 20, 30)), FMT_STRING("rgb(255,20,30)")),
               "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m");
  EXPECT_WRITE(
      stdout,
      fmt::print(fg(fmt::color::blue) | bg(fmt::color::red), FMT_STRING("two color")),
      "\x1b[38;2;000;000;255m\x1b[48;2;255;000;000mtwo color\x1b[0m");
  EXPECT_WRITE(stdout, fmt::print(fmt::emphasis::bold, FMT_STRING("bold")),
               "\x1b[1mbold\x1b[0m");
  EXPECT_WRITE(
      stdout,
      fmt::print(fg(fmt::color::blue) | fmt::emphasis::bold, FMT_STRING("blue/bold")),
      "\x1b[1m\x1b[38;2;000;000;255mblue/bold\x1b[0m");
  EXPECT_WRITE(stdout, fmt::print(fmt::text_style(), FMT_STRING("hi")), "hi");
  EXPECT_WRITE(stdout, fmt::print(fg(fmt::terminal_color::red), FMT_STRING("tred")),
               "\x1b[31mtred\x1b[0m");
  EXPECT_WRITE(stdout, fmt::print(bg(fmt::terminal_color::cyan), FMT_STRING("tcyan")),
               "\x1b[46mtcyan\x1b[0m");
  EXPECT_WRITE(stdout,
               fmt::print(bg(fmt::terminal_color::bright_magenta), FMT_STRING("tbmagenta")),
               "\x1b[105mtbmagenta\x1b[0m");
}

TEST(ColorsTest, Format) {
  EXPECT_EQ("\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m", fmt::format(fg(fmt::rgb(255, 20, 30)), FMT_STRING("rgb(255,20,30)")));
  EXPECT_EQ("\x1b[38;2;000;000;255mblue\x1b[0m", fmt::format(fg(fmt::color::blue), FMT_STRING("blue")));
  EXPECT_EQ(
      "\x1b[38;2;000;000;255m\x1b[48;2;255;000;000mtwo color\x1b[0m",
      fmt::format(fg(fmt::color::blue) | bg(fmt::color::red), FMT_STRING("two color")));
  EXPECT_EQ("\x1b[1mbold\x1b[0m", fmt::format(fmt::emphasis::bold, FMT_STRING("bold")));
  EXPECT_EQ(
    "\x1b[1m\x1b[38;2;000;000;255mblue/bold\x1b[0m",
      fmt::format(fg(fmt::color::blue) | fmt::emphasis::bold, FMT_STRING("blue/bold")));
  EXPECT_EQ("hi", fmt::format(fmt::text_style(), FMT_STRING("hi")));
  EXPECT_EQ("\x1b[31mtred\x1b[0m", 
        fmt::format(fg(fmt::terminal_color::red), FMT_STRING("tred")));
  EXPECT_EQ("\x1b[46mtcyan\x1b[0m", 
          fmt::format(bg(fmt::terminal_color::cyan), FMT_STRING("tcyan")));
  EXPECT_EQ("\x1b[105mtbmagenta\x1b[0m", 
          fmt::format(bg(fmt::terminal_color::bright_magenta), FMT_STRING("tbmagenta")));
}

TEST(ColorsTest, FormatToOutAcceptsTextStyle) {
  fmt::text_style ts = fg(fmt::rgb(255, 20, 30));
  std::string out;
  fmt::format_to(std::back_inserter(out), ts, FMT_STRING("rgb(255,20,30){}{}{}"), 1, 2, 3);

  EXPECT_EQ(fmt::to_string(out),
            "\x1b[38;2;255;020;030mrgb(255,20,30)123\x1b[0m");
}

// Formatting library for C++ - std::ostream support tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#define FMT_STRING_ALIAS 1
#include "fmt/format.h"

struct test {};

// Test that there is no issues with specializations when fmt/ostream.h is
// included after fmt/format.h.
namespace fmt {
template <> struct formatter<test> : formatter<int> {
  template <typename FormatContext>
  typename FormatContext::iterator format(const test&, FormatContext& ctx) {
    return formatter<int>::format(42, ctx);
  }
};
}  // namespace fmt

#include <sstream>

#include "fmt/ostream.h"

struct EmptyTest {};
static std::ostream& operator<<(std::ostream& os, EmptyTest) {
  return os << "";
}

TEST(OStreamTest, EmptyCustomOutput) {
  EXPECT_EQ("", fmt::format(FMT_STRING("{}"), EmptyTest()));
}

TEST(OStreamTest, Print) {
  std::ostringstream os;
  fmt::print(os, FMT_STRING("Don't {}!"), "panic");
  EXPECT_EQ("Don't panic!", os.str());
  std::wostringstream wos;
  fmt::print(wos, FMT_STRING(L"Don't {}!"), L"panic");
  EXPECT_EQ(L"Don't panic!", wos.str());
}

TEST(OStreamTest, Join) {
  int v[3] = {1, 2, 3};
  EXPECT_EQ("1, 2, 3", fmt::format(FMT_STRING("{}"), fmt::join(v, v + 3, ", ")));
}

namespace fmt_test {
struct ABC {};

template <typename Output> Output& operator<<(Output& out, ABC) {
  out << "ABC";
  return out;
}
}  // namespace fmt_test

template <typename T> struct TestTemplate {};

template <typename T>
std::ostream& operator<<(std::ostream& os, TestTemplate<T>) {
  return os << 1;
}

namespace fmt {
template <typename T> struct formatter<TestTemplate<T>> : formatter<int> {
  template <typename FormatContext>
  typename FormatContext::iterator format(TestTemplate<T>, FormatContext& ctx) {
    return formatter<int>::format(2, ctx);
  }
};
}  // namespace fmt

#if !FMT_GCC_VERSION || FMT_GCC_VERSION >= 407
TEST(OStreamTest, Template) {
  EXPECT_EQ("2", fmt::format(FMT_STRING("{}"), TestTemplate<int>()));
}

TEST(OStreamTest, FormatToN) {
  char buffer[4];
  buffer[3] = 'x';
  auto result = fmt::format_to_n(buffer, 3, FMT_STRING("{}"), fmt_test::ABC());
  EXPECT_EQ(3u, result.size);
  EXPECT_EQ(buffer + 3, result.out);
  EXPECT_EQ("ABCx", fmt::string_view(buffer, 4));
}
#endif

TEST(OStreamTest, CompileTimeString) {
  EXPECT_EQ("42", fmt::format(FMT_STRING("{}"), 42));
}

TEST(OStreamTest, ToString) {
  EXPECT_EQ("ABC", fmt::to_string(fmt_test::ABC()));
}


#include "fmt/ranges.h"

// Check if  'if constexpr' is supported.
#if (__cplusplus > 201402L) || \
    (defined(_MSVC_LANG) && _MSVC_LANG > 201402L && _MSC_VER >= 1910)

#  include <array>
#  include <map>
#  include <string>
#  include <vector>

TEST(RangesTest, FormatArray) {
  int32_t ia[] = {1, 2, 3, 5, 7, 11};
  auto iaf = fmt::format(FMT_STRING("{}"), ia);
  EXPECT_EQ("{1, 2, 3, 5, 7, 11}", iaf);
}

TEST(RangesTest, Format2dArray) {
  int32_t ia[][2] = {{1, 2}, {3, 5}, {7, 11}};
  auto iaf = fmt::format(FMT_STRING("{}"), ia);
  EXPECT_EQ("{{1, 2}, {3, 5}, {7, 11}}", iaf);
}

TEST(RangesTest, FormatVector) {
  std::vector<int32_t> iv{1, 2, 3, 5, 7, 11};
  auto ivf = fmt::format(FMT_STRING("{}"), iv);
  EXPECT_EQ("{1, 2, 3, 5, 7, 11}", ivf);
}

TEST(RangesTest, FormatVector2) {
  std::vector<std::vector<int32_t>> ivv{{1, 2}, {3, 5}, {7, 11}};
  auto ivf = fmt::format(FMT_STRING("{}"), ivv);
  EXPECT_EQ("{{1, 2}, {3, 5}, {7, 11}}", ivf);
}

TEST(RangesTest, FormatMap) {
  std::map<std::string, int32_t> simap{{"one", 1}, {"two", 2}};
  EXPECT_EQ("{(\"one\", 1), (\"two\", 2)}", fmt::format(FMT_STRING("{}"), simap));
}

TEST(RangesTest, FormatPair) {
  std::pair<int64_t, float> pa1{42, 1.5f};
  EXPECT_EQ("(42, 1.5)", fmt::format(FMT_STRING("{}"), pa1));
}

TEST(RangesTest, FormatTuple) {
  std::tuple<int64_t, float, std::string, char> t{42, 1.5f, "this is tuple",
                                                  'i'};
  EXPECT_EQ("(42, 1.5, \"this is tuple\", 'i')", fmt::format(FMT_STRING("{}"), t));
  EXPECT_EQ("()", fmt::format(FMT_STRING("{}"), std::tuple<>()));
}

TEST(RangesTest, JoinTuple) {
  // Value tuple args
  std::tuple<char, int, float> t1 = std::make_tuple('a', 1, 2.0f);
  EXPECT_EQ("(a, 1, 2)", fmt::format(FMT_STRING("({})"), fmt::join(t1, ", ")));

  // Testing lvalue tuple args
  int x = 4;
  std::tuple<char, int&> t2{'b', x};
  EXPECT_EQ("b + 4", fmt::format(FMT_STRING("{}"), fmt::join(t2, " + ")));

  // Empty tuple
  std::tuple<> t3;
  EXPECT_EQ("", fmt::format(FMT_STRING("{}"), fmt::join(t3, "|")));

  // Single element tuple
  std::tuple<float> t4{4.0f};
  EXPECT_EQ("4", fmt::format(FMT_STRING("{}"), fmt::join(t4, "/")));
}

TEST(RangesTest, JoinInitializerList) {
  EXPECT_EQ("1, 2, 3", fmt::format(FMT_STRING("{}"), fmt::join({1, 2, 3}, ", ")));
  EXPECT_EQ("fmt rocks !",
            fmt::format(FMT_STRING("{}"), fmt::join({"fmt", "rocks", "!"}, " ")));
}

struct my_struct {
  int32_t i;
  std::string str;  // can throw
  template <size_t N> decltype(auto) get() const noexcept {
    if constexpr (N == 0)
      return i;
    else if constexpr (N == 1)
      return fmt::string_view{str};
  }
};

template <size_t N> decltype(auto) get(const my_struct& s) noexcept {
  return s.get<N>();
}

namespace std {

template <> struct tuple_size<my_struct> : std::integral_constant<size_t, 2> {};

template <size_t N> struct tuple_element<N, my_struct> {
  using type = decltype(std::declval<my_struct>().get<N>());
};

}  // namespace std

TEST(RangesTest, FormatStruct) {
  my_struct mst{13, "my struct"};
  EXPECT_EQ("(13, \"my struct\")", fmt::format(FMT_STRING("{}"), mst));
}

TEST(RangesTest, FormatTo) {
  char buf[10];
  auto end = fmt::format_to(buf, FMT_STRING("{}"), std::vector{1, 2, 3});
  *end = '\0';
  EXPECT_STREQ(buf, "{1, 2, 3}");
}

struct path_like {
  const path_like* begin() const;
  const path_like* end() const;

  operator std::string() const;
};

TEST(RangesTest, PathLike) {
  EXPECT_FALSE((fmt::is_range<path_like, char>::value));
}

#endif  // (__cplusplus > 201402L) || (defined(_MSVC_LANG) && _MSVC_LANG >
        // 201402L && _MSC_VER >= 1910)

#ifdef FMT_USE_STRING_VIEW
struct string_like {
  const char* begin();
  const char* end();
  explicit operator fmt::string_view() const { return "foo"; }
  explicit operator std::string_view() const { return "foo"; }
};

TEST(RangesTest, FormatStringLike) {
  EXPECT_EQ("foo", fmt::format(FMT_STRING("{}"), string_like()));
}
#endif  // FMT_USE_STRING_VIEW

struct zstring_sentinel {};

bool operator==(const char* p, zstring_sentinel) { return *p == '\0'; }
bool operator!=(const char* p, zstring_sentinel) { return *p != '\0'; }

struct zstring {
  const char* p;
  const char* begin() const { return p; }
  zstring_sentinel end() const { return {}; }
};

// TODO: Fix using zstrings with FMT_STRING
TEST(RangesTest, JoinSentinel) {
  zstring hello{"hello"};
  //EXPECT_EQ("{'h', 'e', 'l', 'l', 'o'}", fmt::format(FMT_STRING("{}"), hello));
  //EXPECT_EQ("h_e_l_l_o", fmt::format(FMT_STRING("{}"), fmt::join(hello, "_")));
}

// A range that provides non-const only begin()/end() to test fmt::join handles
// that
//
// Some ranges (eg those produced by range-v3's views::filter()) can cache
// information during iteration so they only provide non-const begin()/end().
template <typename T> class non_const_only_range {
 private:
  std::vector<T> vec;

 public:
  using const_iterator = typename ::std::vector<T>::const_iterator;

  template <typename... Args>
  explicit non_const_only_range(Args&&... args)
      : vec(::std::forward<Args>(args)...) {}

  const_iterator begin() { return vec.begin(); }
  const_iterator end() { return vec.end(); }
};

template <typename T> class noncopyable_range {
 private:
  std::vector<T> vec;

 public:
  using const_iterator = typename ::std::vector<T>::const_iterator;

  template <typename... Args>
  explicit noncopyable_range(Args&&... args)
      : vec(::std::forward<Args>(args)...) {}

  noncopyable_range(noncopyable_range const&) = delete;
  noncopyable_range(noncopyable_range&) = delete;

  const_iterator begin() const { return vec.begin(); }
  const_iterator end() const { return vec.end(); }
};

//TODO: Fixme
TEST(RangesTest, Range) {
  noncopyable_range<int> w(3u, 0);
  /*EXPECT_EQ("{0, 0, 0}", fmt::format(FMT_STRING("{}"), w));
  EXPECT_EQ("{0, 0, 0}", fmt::format(FMT_STRING("{}"), noncopyable_range<int>(3u, 0)));

  non_const_only_range<int> x(3u, 0);
  EXPECT_EQ("{0, 0, 0}", fmt::format(FMT_STRING("{}"), x));
  EXPECT_EQ("{0, 0, 0}", fmt::format(FMT_STRING("{}"), non_const_only_range<int>(3u, 0)));

  std::vector<int> y(3u, 0);
  EXPECT_EQ("{0, 0, 0}", fmt::format(FMT_STRING("{}"), y));
  EXPECT_EQ("{0, 0, 0}", fmt::format(FMT_STRING("{}"), std::vector<int>(3u, 0)));

  const std::vector<int> z(3u, 0);
  EXPECT_EQ("{0, 0, 0}", fmt::format(FMT_STRING("{}"), z));*/
}

#if !FMT_MSC_VER || FMT_MSC_VER >= 1927
struct unformattable {};

TEST(RangesTest, UnformattableRange) {
  EXPECT_FALSE((fmt::has_formatter<std::vector<unformattable>,
                                   fmt::format_context>::value));
}
#endif



