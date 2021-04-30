// Formatting library for C++ - printf tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/printf.h"

#include <cctype>
#include <climits>
#include <cstring>

#include "fmt/core.h"
#include "fmt/ostream.h"
#include "gtest-extra.h"
#include "util.h"

using fmt::format;
using fmt::format_error;
using fmt::detail::max_value;

const unsigned BIG_NUM = INT_MAX + 1u;

// Makes format string argument positional.
static std::string make_positional(fmt::string_view format) {
  std::string s(format.data(), format.size());
  s.replace(s.find('%'), 1, "%1$");
  return s;
}

static std::wstring make_positional(fmt::wstring_view format) {
  std::wstring s(format.data(), format.size());
  s.replace(s.find(L'%'), 1, L"%1$");
  return s;
}

// A wrapper around fmt::sprintf to workaround bogus warnings about invalid
// format strings in MSVC.
template <typename... Args>
std::string test_sprintf(fmt::string_view format, const Args&... args) {
  return fmt::sprintf(format, args...);
}
template <typename... Args>
std::wstring test_sprintf(fmt::wstring_view format, const Args&... args) {
  return fmt::sprintf(format, args...);
}

#define EXPECT_PRINTF(expected_output, format, arg)     \
  EXPECT_EQ(expected_output, test_sprintf(format, arg)) \
      << "format: " << format;                          \
  EXPECT_EQ(expected_output, fmt::sprintf(make_positional(format), arg))

template <typename T> struct value_extractor {
  T operator()(T value) { return value; }

  template <typename U> FMT_NORETURN T operator()(U) {
    throw std::runtime_error(fmt::format("invalid type {}", typeid(U).name()));
  }

#if FMT_USE_INT128
  // Apple Clang does not define typeid for __int128_t and __uint128_t.
  FMT_NORETURN T operator()(fmt::detail::int128_t) {
    throw std::runtime_error("invalid type __int128_t");
  }

  FMT_NORETURN T operator()(fmt::detail::uint128_t) {
    throw std::runtime_error("invalid type __uint128_t");
  }
#endif
};

TEST(PrintfTest, ArgConverter) {
  long long value = max_value<long long>();
  auto arg = fmt::detail::make_arg<fmt::format_context>(value);
  fmt::visit_format_arg(
      fmt::detail::arg_converter<long long, fmt::format_context>(arg, 'd'),
      arg);
  EXPECT_EQ(value, fmt::visit_format_arg(value_extractor<long long>(), arg));
}

TEST(PrintfTest, NoArgs) {
  EXPECT_EQ("test", test_sprintf("test"));
  EXPECT_EQ(L"test", fmt::sprintf(L"test"));
}

TEST(PrintfTest, Escape) {
  EXPECT_EQ("%", test_sprintf("%%"));
  EXPECT_EQ("before %", test_sprintf("before %%"));
  EXPECT_EQ("% after", test_sprintf("%% after"));
  EXPECT_EQ("before % after", test_sprintf("before %% after"));
  EXPECT_EQ("%s", test_sprintf("%%s"));
  EXPECT_EQ(L"%", fmt::sprintf(L"%%"));
  EXPECT_EQ(L"before %", fmt::sprintf(L"before %%"));
  EXPECT_EQ(L"% after", fmt::sprintf(L"%% after"));
  EXPECT_EQ(L"before % after", fmt::sprintf(L"before %% after"));
  EXPECT_EQ(L"%s", fmt::sprintf(L"%%s"));
}

TEST(PrintfTest, PositionalArgs) {
  EXPECT_EQ("42", test_sprintf("%1$d", 42));
  EXPECT_EQ("before 42", test_sprintf("before %1$d", 42));
  EXPECT_EQ("42 after", test_sprintf("%1$d after", 42));
  EXPECT_EQ("before 42 after", test_sprintf("before %1$d after", 42));
  EXPECT_EQ("answer = 42", test_sprintf("%1$s = %2$d", "answer", 42));
  EXPECT_EQ("42 is the answer", test_sprintf("%2$d is the %1$s", "answer", 42));
  EXPECT_EQ("abracadabra", test_sprintf("%1$s%2$s%1$s", "abra", "cad"));
}

TEST(PrintfTest, AutomaticArgIndexing) {
  EXPECT_EQ("abc", test_sprintf("%c%c%c", 'a', 'b', 'c'));
}

TEST(PrintfTest, NumberIsTooBigInArgIndex) {
  EXPECT_THROW_MSG(test_sprintf(format("%{}$", BIG_NUM)), format_error,
                   "number is too big");
  EXPECT_THROW_MSG(test_sprintf(format("%{}$d", BIG_NUM)), format_error,
                   "number is too big");
}

TEST(PrintfTest, SwitchArgIndexing) {
  EXPECT_THROW_MSG(test_sprintf("%1$d%", 1, 2), format_error,
                   "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG(test_sprintf(format("%1$d%{}d", BIG_NUM), 1, 2),
                   format_error, "number is too big");
  EXPECT_THROW_MSG(test_sprintf("%1$d%d", 1, 2), format_error,
                   "cannot switch from manual to automatic argument indexing");

  EXPECT_THROW_MSG(test_sprintf("%d%1$", 1, 2), format_error,
                   "cannot switch from automatic to manual argument indexing");
  EXPECT_THROW_MSG(test_sprintf(format("%d%{}$d", BIG_NUM), 1, 2), format_error,
                   "number is too big");
  EXPECT_THROW_MSG(test_sprintf("%d%1$d", 1, 2), format_error,
                   "cannot switch from automatic to manual argument indexing");

  // Indexing errors override width errors.
  EXPECT_THROW_MSG(test_sprintf(format("%d%1${}d", BIG_NUM), 1, 2),
                   format_error, "number is too big");
  EXPECT_THROW_MSG(test_sprintf(format("%1$d%{}d", BIG_NUM), 1, 2),
                   format_error, "number is too big");
}

TEST(PrintfTest, InvalidArgIndex) {
  EXPECT_THROW_MSG(test_sprintf("%0$d", 42), format_error,
                   "argument not found");
  EXPECT_THROW_MSG(test_sprintf("%2$d", 42), format_error,
                   "argument not found");
  EXPECT_THROW_MSG(test_sprintf(format("%{}$d", INT_MAX), 42), format_error,
                   "argument not found");

  EXPECT_THROW_MSG(test_sprintf("%2$", 42), format_error, "argument not found");
  EXPECT_THROW_MSG(test_sprintf(format("%{}$d", BIG_NUM), 42), format_error,
                   "number is too big");
}

TEST(PrintfTest, DefaultAlignRight) {
  EXPECT_PRINTF("   42", "%5d", 42);
  EXPECT_PRINTF("  abc", "%5s", "abc");
}

TEST(PrintfTest, ZeroFlag) {
  EXPECT_PRINTF("00042", "%05d", 42);
  EXPECT_PRINTF("-0042", "%05d", -42);

  EXPECT_PRINTF("00042", "%05d", 42);
  EXPECT_PRINTF("-0042", "%05d", -42);
  EXPECT_PRINTF("-004.2", "%06g", -4.2);

  EXPECT_PRINTF("+00042", "%00+6d", 42);

  EXPECT_PRINTF("   42", "%05.d", 42);
  EXPECT_PRINTF(" 0042", "%05.4d", 42);

  // '0' flag is ignored for non-numeric types.
  EXPECT_PRINTF("    x", "%05c", 'x');
}

TEST(PrintfTest, PlusFlag) {
  EXPECT_PRINTF("+42", "%+d", 42);
  EXPECT_PRINTF("-42", "%+d", -42);
  EXPECT_PRINTF("+0042", "%+05d", 42);
  EXPECT_PRINTF("+0042", "%0++5d", 42);

  // '+' flag is ignored for non-numeric types.
  EXPECT_PRINTF("x", "%+c", 'x');

  // '+' flag wins over space flag
  EXPECT_PRINTF("+42", "%+ d", 42);
  EXPECT_PRINTF("-42", "%+ d", -42);
  EXPECT_PRINTF("+42", "% +d", 42);
  EXPECT_PRINTF("-42", "% +d", -42);
  EXPECT_PRINTF("+0042", "% +05d", 42);
  EXPECT_PRINTF("+0042", "%0+ 5d", 42);

  // '+' flag and space flag are both ignored for non-numeric types.
  EXPECT_PRINTF("x", "%+ c", 'x');
  EXPECT_PRINTF("x", "% +c", 'x');
}

TEST(PrintfTest, MinusFlag) {
  EXPECT_PRINTF("abc  ", "%-5s", "abc");
  EXPECT_PRINTF("abc  ", "%0--5s", "abc");

  EXPECT_PRINTF("7    ", "%-5d", 7);
  EXPECT_PRINTF("97   ", "%-5hhi", 'a');
  EXPECT_PRINTF("a    ", "%-5c", 'a');

  // '0' flag is ignored if '-' flag is given
  EXPECT_PRINTF("7    ", "%-05d", 7);
  EXPECT_PRINTF("7    ", "%0-5d", 7);
  EXPECT_PRINTF("a    ", "%-05c", 'a');
  EXPECT_PRINTF("a    ", "%0-5c", 'a');
  EXPECT_PRINTF("97   ", "%-05hhi", 'a');
  EXPECT_PRINTF("97   ", "%0-5hhi", 'a');

  // '-' and space flag don't interfere
  EXPECT_PRINTF(" 42", "%- d", 42);
}

TEST(PrintfTest, SpaceFlag) {
  EXPECT_PRINTF(" 42", "% d", 42);
  EXPECT_PRINTF("-42", "% d", -42);
  EXPECT_PRINTF(" 0042", "% 05d", 42);
  EXPECT_PRINTF(" 0042", "%0  5d", 42);

  // ' ' flag is ignored for non-numeric types.
  EXPECT_PRINTF("x", "% c", 'x');
}

TEST(PrintfTest, HashFlag) {
  EXPECT_PRINTF("042", "%#o", 042);
  EXPECT_PRINTF(fmt::format("0{:o}", static_cast<unsigned>(-042)), "%#o", -042);
  EXPECT_PRINTF("0", "%#o", 0);

  EXPECT_PRINTF("0x42", "%#x", 0x42);
  EXPECT_PRINTF("0X42", "%#X", 0x42);
  EXPECT_PRINTF(fmt::format("0x{:x}", static_cast<unsigned>(-0x42)), "%#x",
                -0x42);
  EXPECT_PRINTF("0", "%#x", 0);

  EXPECT_PRINTF("0x0042", "%#06x", 0x42);
  EXPECT_PRINTF("0x0042", "%0##6x", 0x42);

  EXPECT_PRINTF("-42.000000", "%#f", -42.0);
  EXPECT_PRINTF("-42.000000", "%#F", -42.0);

  char buffer[BUFFER_SIZE];
  safe_sprintf(buffer, "%#e", -42.0);
  EXPECT_PRINTF(buffer, "%#e", -42.0);
  safe_sprintf(buffer, "%#E", -42.0);
  EXPECT_PRINTF(buffer, "%#E", -42.0);

  EXPECT_PRINTF("-42.0000", "%#g", -42.0);
  EXPECT_PRINTF("-42.0000", "%#G", -42.0);

  safe_sprintf(buffer, "%#a", 16.0);
  EXPECT_PRINTF(buffer, "%#a", 16.0);
  safe_sprintf(buffer, "%#A", 16.0);
  EXPECT_PRINTF(buffer, "%#A", 16.0);

  // '#' flag is ignored for non-numeric types.
  EXPECT_PRINTF("x", "%#c", 'x');
}

TEST(PrintfTest, Width) {
  EXPECT_PRINTF("  abc", "%5s", "abc");

  // Width cannot be specified twice.
  EXPECT_THROW_MSG(test_sprintf("%5-5d", 42), format_error,
                   "invalid type specifier");

  EXPECT_THROW_MSG(test_sprintf(format("%{}d", BIG_NUM), 42), format_error,
                   "number is too big");
  EXPECT_THROW_MSG(test_sprintf(format("%1${}d", BIG_NUM), 42), format_error,
                   "number is too big");
}

TEST(PrintfTest, DynamicWidth) {
  EXPECT_EQ("   42", test_sprintf("%*d", 5, 42));
  EXPECT_EQ("42   ", test_sprintf("%*d", -5, 42));
  EXPECT_THROW_MSG(test_sprintf("%*d", 5.0, 42), format_error,
                   "width is not integer");
  EXPECT_THROW_MSG(test_sprintf("%*d"), format_error, "argument not found");
  EXPECT_THROW_MSG(test_sprintf("%*d", BIG_NUM, 42), format_error,
                   "number is too big");
}

TEST(PrintfTest, IntPrecision) {
  EXPECT_PRINTF("00042", "%.5d", 42);
  EXPECT_PRINTF("-00042", "%.5d", -42);
  EXPECT_PRINTF("00042", "%.5x", 0x42);
  EXPECT_PRINTF("0x00042", "%#.5x", 0x42);
  EXPECT_PRINTF("00042", "%.5o", 042);
  EXPECT_PRINTF("00042", "%#.5o", 042);

  EXPECT_PRINTF("  00042", "%7.5d", 42);
  EXPECT_PRINTF("  00042", "%7.5x", 0x42);
  EXPECT_PRINTF("   0x00042", "%#10.5x", 0x42);
  EXPECT_PRINTF("  00042", "%7.5o", 042);
  EXPECT_PRINTF("     00042", "%#10.5o", 042);

  EXPECT_PRINTF("00042  ", "%-7.5d", 42);
  EXPECT_PRINTF("00042  ", "%-7.5x", 0x42);
  EXPECT_PRINTF("0x00042   ", "%-#10.5x", 0x42);
  EXPECT_PRINTF("00042  ", "%-7.5o", 042);
  EXPECT_PRINTF("00042     ", "%-#10.5o", 042);
}

TEST(PrintfTest, FloatPrecision) {
  char buffer[BUFFER_SIZE];
  safe_sprintf(buffer, "%.3e", 1234.5678);
  EXPECT_PRINTF(buffer, "%.3e", 1234.5678);
  EXPECT_PRINTF("1234.568", "%.3f", 1234.5678);
  EXPECT_PRINTF("1.23e+03", "%.3g", 1234.5678);
  safe_sprintf(buffer, "%.3a", 1234.5678);
  EXPECT_PRINTF(buffer, "%.3a", 1234.5678);
}

TEST(PrintfTest, StringPrecision) {
  char test[] = {'H', 'e', 'l', 'l', 'o'};
  EXPECT_EQ(fmt::sprintf("%.4s", test), "Hell");
}

TEST(PrintfTest, IgnorePrecisionForNonNumericArg) {
  EXPECT_PRINTF("abc", "%.5s", "abc");
}

TEST(PrintfTest, DynamicPrecision) {
  EXPECT_EQ("00042", test_sprintf("%.*d", 5, 42));
  EXPECT_EQ("42", test_sprintf("%.*d", -5, 42));
  EXPECT_THROW_MSG(test_sprintf("%.*d", 5.0, 42), format_error,
                   "precision is not integer");
  EXPECT_THROW_MSG(test_sprintf("%.*d"), format_error, "argument not found");
  EXPECT_THROW_MSG(test_sprintf("%.*d", BIG_NUM, 42), format_error,
                   "number is too big");
  if (sizeof(long long) != sizeof(int)) {
    long long prec = static_cast<long long>(INT_MIN) - 1;
    EXPECT_THROW_MSG(test_sprintf("%.*d", prec, 42), format_error,
                     "number is too big");
  }
}

template <typename T> struct make_signed { typedef T type; };

#define SPECIALIZE_MAKE_SIGNED(T, S) \
  template <> struct make_signed<T> { typedef S type; }

SPECIALIZE_MAKE_SIGNED(char, signed char);
SPECIALIZE_MAKE_SIGNED(unsigned char, signed char);
SPECIALIZE_MAKE_SIGNED(unsigned short, short);
SPECIALIZE_MAKE_SIGNED(unsigned, int);
SPECIALIZE_MAKE_SIGNED(unsigned long, long);
SPECIALIZE_MAKE_SIGNED(unsigned long long, long long);

// Test length format specifier ``length_spec``.
template <typename T, typename U>
void TestLength(const char* length_spec, U value) {
  long long signed_value = 0;
  unsigned long long unsigned_value = 0;
  // Apply integer promotion to the argument.
  unsigned long long max = max_value<U>();
  using fmt::detail::const_check;
  if (const_check(max <= static_cast<unsigned>(max_value<int>()))) {
    signed_value = static_cast<int>(value);
    unsigned_value = static_cast<unsigned long long>(value);
  } else if (const_check(max <= max_value<unsigned>())) {
    signed_value = static_cast<unsigned>(value);
    unsigned_value = static_cast<unsigned long long>(value);
  }
  if (sizeof(U) <= sizeof(int) && sizeof(int) < sizeof(T)) {
    signed_value = static_cast<long long>(value);
    unsigned_value = static_cast<unsigned long long>(
        static_cast<typename std::make_unsigned<unsigned>::type>(value));
  } else {
    signed_value = static_cast<typename make_signed<T>::type>(value);
    unsigned_value = static_cast<typename std::make_unsigned<T>::type>(value);
  }
  std::ostringstream os;
  os << signed_value;
  EXPECT_PRINTF(os.str(), fmt::format("%{}d", length_spec), value);
  EXPECT_PRINTF(os.str(), fmt::format("%{}i", length_spec), value);
  os.str("");
  os << unsigned_value;
  EXPECT_PRINTF(os.str(), fmt::format("%{}u", length_spec), value);
  os.str("");
  os << std::oct << unsigned_value;
  EXPECT_PRINTF(os.str(), fmt::format("%{}o", length_spec), value);
  os.str("");
  os << std::hex << unsigned_value;
  EXPECT_PRINTF(os.str(), fmt::format("%{}x", length_spec), value);
  os.str("");
  os << std::hex << std::uppercase << unsigned_value;
  EXPECT_PRINTF(os.str(), fmt::format("%{}X", length_spec), value);
}

template <typename T> void TestLength(const char* length_spec) {
  T min = std::numeric_limits<T>::min(), max = max_value<T>();
  TestLength<T>(length_spec, 42);
  TestLength<T>(length_spec, -42);
  TestLength<T>(length_spec, min);
  TestLength<T>(length_spec, max);
  long long long_long_min = std::numeric_limits<long long>::min();
  if (static_cast<long long>(min) > long_long_min)
    TestLength<T>(length_spec, static_cast<long long>(min) - 1);
  unsigned long long long_long_max = max_value<long long>();
  if (static_cast<unsigned long long>(max) < long_long_max)
    TestLength<T>(length_spec, static_cast<long long>(max) + 1);
  TestLength<T>(length_spec, std::numeric_limits<short>::min());
  TestLength<T>(length_spec, max_value<unsigned short>());
  TestLength<T>(length_spec, std::numeric_limits<int>::min());
  TestLength<T>(length_spec, max_value<int>());
  TestLength<T>(length_spec, std::numeric_limits<unsigned>::min());
  TestLength<T>(length_spec, max_value<unsigned>());
  TestLength<T>(length_spec, std::numeric_limits<long long>::min());
  TestLength<T>(length_spec, max_value<long long>());
  TestLength<T>(length_spec, std::numeric_limits<unsigned long long>::min());
  TestLength<T>(length_spec, max_value<unsigned long long>());
}

TEST(PrintfTest, Length) {
  TestLength<char>("hh");
  TestLength<signed char>("hh");
  TestLength<unsigned char>("hh");
  TestLength<short>("h");
  TestLength<unsigned short>("h");
  TestLength<long>("l");
  TestLength<unsigned long>("l");
  TestLength<long long>("ll");
  TestLength<unsigned long long>("ll");
  TestLength<intmax_t>("j");
  TestLength<size_t>("z");
  TestLength<std::ptrdiff_t>("t");
  long double max = max_value<long double>();
  EXPECT_PRINTF(fmt::format("{:.6}", max), "%g", max);
  EXPECT_PRINTF(fmt::format("{:.6}", max), "%Lg", max);
}

TEST(PrintfTest, Bool) {
  EXPECT_PRINTF("1", "%d", true);
  EXPECT_PRINTF("true", "%s", true);
}

TEST(PrintfTest, Int) {
  EXPECT_PRINTF("-42", "%d", -42);
  EXPECT_PRINTF("-42", "%i", -42);
  unsigned u = 0 - 42u;
  EXPECT_PRINTF(fmt::format("{}", u), "%u", -42);
  EXPECT_PRINTF(fmt::format("{:o}", u), "%o", -42);
  EXPECT_PRINTF(fmt::format("{:x}", u), "%x", -42);
  EXPECT_PRINTF(fmt::format("{:X}", u), "%X", -42);
}

TEST(PrintfTest, long_long) {
  // fmt::printf allows passing long long arguments to %d without length
  // specifiers.
  long long max = max_value<long long>();
  EXPECT_PRINTF(fmt::format("{}", max), "%d", max);
}

TEST(PrintfTest, Float) {
  EXPECT_PRINTF("392.650000", "%f", 392.65);
  EXPECT_PRINTF("392.65", "%.2f", 392.65);
  EXPECT_PRINTF("392.6", "%.1f", 392.65);
  EXPECT_PRINTF("393", "%.f", 392.65);
  EXPECT_PRINTF("392.650000", "%F", 392.65);
  char buffer[BUFFER_SIZE];
  safe_sprintf(buffer, "%e", 392.65);
  EXPECT_PRINTF(buffer, "%e", 392.65);
  safe_sprintf(buffer, "%E", 392.65);
  EXPECT_PRINTF(buffer, "%E", 392.65);
  EXPECT_PRINTF("392.65", "%g", 392.65);
  EXPECT_PRINTF("392.65", "%G", 392.65);
  EXPECT_PRINTF("392", "%g", 392.0);
  EXPECT_PRINTF("392", "%G", 392.0);
  EXPECT_PRINTF("4.56e-07", "%g", 0.000000456);
  safe_sprintf(buffer, "%a", -392.65);
  EXPECT_EQ(buffer, format("{:a}", -392.65));
  safe_sprintf(buffer, "%A", -392.65);
  EXPECT_EQ(buffer, format("{:A}", -392.65));
}

TEST(PrintfTest, Inf) {
  double inf = std::numeric_limits<double>::infinity();
  for (const char* type = "fega"; *type; ++type) {
    EXPECT_PRINTF("inf", fmt::format("%{}", *type), inf);
    char upper = static_cast<char>(std::toupper(*type));
    EXPECT_PRINTF("INF", fmt::format("%{}", upper), inf);
  }
}

TEST(PrintfTest, Char) {
  EXPECT_PRINTF("x", "%c", 'x');
  int max = max_value<int>();
  EXPECT_PRINTF(fmt::format("{}", static_cast<char>(max)), "%c", max);
  // EXPECT_PRINTF("x", "%lc", L'x');
  EXPECT_PRINTF(L"x", L"%c", L'x');
  EXPECT_PRINTF(fmt::format(L"{}", static_cast<wchar_t>(max)), L"%c", max);
}

TEST(PrintfTest, String) {
  EXPECT_PRINTF("abc", "%s", "abc");
  const char* null_str = nullptr;
  EXPECT_PRINTF("(null)", "%s", null_str);
  EXPECT_PRINTF("    (null)", "%10s", null_str);
  EXPECT_PRINTF(L"abc", L"%s", L"abc");
  const wchar_t* null_wstr = nullptr;
  EXPECT_PRINTF(L"(null)", L"%s", null_wstr);
  EXPECT_PRINTF(L"    (null)", L"%10s", null_wstr);
}

TEST(PrintfTest, UCharString) {
  unsigned char str[] = "test";
  unsigned char* pstr = str;
  EXPECT_EQ("test", fmt::sprintf("%s", pstr));
}

TEST(PrintfTest, Pointer) {
  int n;
  void* p = &n;
  EXPECT_PRINTF(fmt::format("{}", p), "%p", p);
  p = nullptr;
  EXPECT_PRINTF("(nil)", "%p", p);
  EXPECT_PRINTF("     (nil)", "%10p", p);
  const char* s = "test";
  EXPECT_PRINTF(fmt::format("{:p}", s), "%p", s);
  const char* null_str = nullptr;
  EXPECT_PRINTF("(nil)", "%p", null_str);

  p = &n;
  EXPECT_PRINTF(fmt::format(L"{}", p), L"%p", p);
  p = nullptr;
  EXPECT_PRINTF(L"(nil)", L"%p", p);
  EXPECT_PRINTF(L"     (nil)", L"%10p", p);
  const wchar_t* w = L"test";
  EXPECT_PRINTF(fmt::format(L"{:p}", w), L"%p", w);
  const wchar_t* null_wstr = nullptr;
  EXPECT_PRINTF(L"(nil)", L"%p", null_wstr);
}

TEST(PrintfTest, Location) {
  // TODO: test %n
}

enum test_enum { answer = 42 };

TEST(PrintfTest, Enum) {
  EXPECT_PRINTF("42", "%d", answer);
  volatile test_enum volatile_enum = answer;
  EXPECT_PRINTF("42", "%d", volatile_enum);
}

#if FMT_USE_FCNTL
TEST(PrintfTest, Examples) {
  const char* weekday = "Thursday";
  const char* month = "August";
  int day = 21;
  EXPECT_WRITE(stdout, fmt::printf("%1$s, %3$d %2$s", weekday, month, day),
               "Thursday, 21 August");
}

TEST(PrintfTest, PrintfError) {
  fmt::file read_end, write_end;
  fmt::file::pipe(read_end, write_end);
  int result = fmt::fprintf(read_end.fdopen("r").get(), "test");
  EXPECT_LT(result, 0);
}
#endif

TEST(PrintfTest, WideString) { EXPECT_EQ(L"abc", fmt::sprintf(L"%s", L"abc")); }

TEST(PrintfTest, PrintfCustom) {
  EXPECT_EQ("abc", test_sprintf("%s", TestString("abc")));
}

TEST(PrintfTest, OStream) {
  std::ostringstream os;
  int ret = fmt::fprintf(os, "Don't %s!", "panic");
  EXPECT_EQ("Don't panic!", os.str());
  EXPECT_EQ(12, ret);
}

TEST(PrintfTest, VPrintf) {
  fmt::format_arg_store<fmt::printf_context, int> as{42};
  fmt::basic_format_args<fmt::printf_context> args(as);
  EXPECT_EQ(fmt::vsprintf("%d", args), "42");
  EXPECT_WRITE(stdout, fmt::vprintf("%d", args), "42");
  EXPECT_WRITE(stdout, fmt::vfprintf(stdout, "%d", args), "42");
  EXPECT_WRITE(stdout, fmt::vfprintf(std::cout, "%d", args), "42");
}

template <typename... Args>
void check_format_string_regression(fmt::string_view s, const Args&... args) {
  fmt::sprintf(s, args...);
}

TEST(PrintfTest, CheckFormatStringRegression) {
  check_format_string_regression("%c%s", 'x', "");
}

TEST(PrintfTest, FixedLargeExponent) {
  EXPECT_EQ("1000000000000000000000", fmt::sprintf("%.*f", -13, 1e21));
}

TEST(PrintfTest, VSPrintfMakeArgsExample) {
  fmt::format_arg_store<fmt::printf_context, int, const char*> as{42,
                                                                  "something"};
  fmt::basic_format_args<fmt::printf_context> args(as);
  EXPECT_EQ("[42] something happened", fmt::vsprintf("[%d] %s happened", args));
  auto as2 = fmt::make_printf_args(42, "something");
  fmt::basic_format_args<fmt::printf_context> args2(as2);
  EXPECT_EQ("[42] something happened",
            fmt::vsprintf("[%d] %s happened", args2));
  // The older gcc versions can't cast the return value.
#if !defined(__GNUC__) || (__GNUC__ > 4)
  EXPECT_EQ("[42] something happened",
            fmt::vsprintf("[%d] %s happened",
                          {fmt::make_printf_args(42, "something")}));
#endif
}

TEST(PrintfTest, VSPrintfMakeWArgsExample) {
  fmt::format_arg_store<fmt::wprintf_context, int, const wchar_t*> as{
      42, L"something"};
  fmt::basic_format_args<fmt::wprintf_context> args(as);
  EXPECT_EQ(L"[42] something happened",
            fmt::vsprintf(L"[%d] %s happened", args));
  auto as2 = fmt::make_wprintf_args(42, L"something");
  fmt::basic_format_args<fmt::wprintf_context> args2(as2);
  EXPECT_EQ(L"[42] something happened",
            fmt::vsprintf(L"[%d] %s happened", args2));
  // the older gcc versions can't cast the return value
#if !defined(__GNUC__) || (__GNUC__ > 4)
  EXPECT_EQ(L"[42] something happened",
            fmt::vsprintf(L"[%d] %s happened",
                          {fmt::make_wprintf_args(42, L"something")}));
#endif
}
