/*
 printf tests.

 Copyright (c) 2012-2014, Victor Zverovich
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <cctype>
#include <climits>
#include <cstring>

#include "fmt/format.h"
#include "gtest-extra.h"
#include "util.h"

using fmt::format;
using fmt::FormatError;

const unsigned BIG_NUM = INT_MAX + 1u;

// Makes format string argument positional.
std::string make_positional(fmt::StringRef format) {
  std::string s(format.to_string());
  s.replace(s.find('%'), 1, "%1$");
  return s;
}

#define EXPECT_PRINTF(expected_output, format, arg) \
  EXPECT_EQ(expected_output, fmt::sprintf(format, arg)) \
    << "format: " << format; \
  EXPECT_EQ(expected_output, fmt::sprintf(make_positional(format), arg))

TEST(PrintfTest, NoArgs) {
  EXPECT_EQ("test", fmt::sprintf("test"));
}

TEST(PrintfTest, Escape) {
  EXPECT_EQ("%", fmt::sprintf("%%"));
  EXPECT_EQ("before %", fmt::sprintf("before %%"));
  EXPECT_EQ("% after", fmt::sprintf("%% after"));
  EXPECT_EQ("before % after", fmt::sprintf("before %% after"));
  EXPECT_EQ("%s", fmt::sprintf("%%s"));
}

TEST(PrintfTest, PositionalArgs) {
  EXPECT_EQ("42", fmt::sprintf("%1$d", 42));
  EXPECT_EQ("before 42", fmt::sprintf("before %1$d", 42));
  EXPECT_EQ("42 after", fmt::sprintf("%1$d after",42));
  EXPECT_EQ("before 42 after", fmt::sprintf("before %1$d after", 42));
  EXPECT_EQ("answer = 42", fmt::sprintf("%1$s = %2$d", "answer", 42));
  EXPECT_EQ("42 is the answer",
      fmt::sprintf("%2$d is the %1$s", "answer", 42));
  EXPECT_EQ("abracadabra", fmt::sprintf("%1$s%2$s%1$s", "abra", "cad"));
}

TEST(PrintfTest, AutomaticArgIndexing) {
  EXPECT_EQ("abc", fmt::sprintf("%c%c%c", 'a', 'b', 'c'));
}

TEST(PrintfTest, NumberIsTooBigInArgIndex) {
  EXPECT_THROW_MSG(fmt::sprintf(format("%{}$", BIG_NUM)),
      FormatError, "number is too big");
  EXPECT_THROW_MSG(fmt::sprintf(format("%{}$d", BIG_NUM)),
      FormatError, "number is too big");
}

TEST(PrintfTest, SwitchArgIndexing) {
  EXPECT_THROW_MSG(fmt::sprintf("%1$d%", 1, 2),
      FormatError, "invalid format string");
  EXPECT_THROW_MSG(fmt::sprintf(format("%1$d%{}d", BIG_NUM), 1, 2),
      FormatError, "number is too big");
  EXPECT_THROW_MSG(fmt::sprintf("%1$d%d", 1, 2),
      FormatError, "cannot switch from manual to automatic argument indexing");

  EXPECT_THROW_MSG(fmt::sprintf("%d%1$", 1, 2),
      FormatError, "invalid format string");
  EXPECT_THROW_MSG(fmt::sprintf(format("%d%{}$d", BIG_NUM), 1, 2),
      FormatError, "number is too big");
  EXPECT_THROW_MSG(fmt::sprintf("%d%1$d", 1, 2),
      FormatError, "cannot switch from automatic to manual argument indexing");

  // Indexing errors override width errors.
  EXPECT_THROW_MSG(fmt::sprintf(format("%d%1${}d", BIG_NUM), 1, 2),
      FormatError, "number is too big");
  EXPECT_THROW_MSG(fmt::sprintf(format("%1$d%{}d", BIG_NUM), 1, 2),
      FormatError, "number is too big");
}

TEST(PrintfTest, InvalidArgIndex) {
  EXPECT_THROW_MSG(fmt::sprintf("%0$d", 42), FormatError,
      "argument index out of range");
  EXPECT_THROW_MSG(fmt::sprintf("%2$d", 42), FormatError,
      "argument index out of range");
  EXPECT_THROW_MSG(fmt::sprintf(format("%{}$d", INT_MAX), 42),
      FormatError, "argument index out of range");

  EXPECT_THROW_MSG(fmt::sprintf("%2$", 42),
      FormatError, "invalid format string");
  EXPECT_THROW_MSG(fmt::sprintf(format("%{}$d", BIG_NUM), 42),
      FormatError, "number is too big");
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
}

TEST(PrintfTest, MinusFlag) {
  EXPECT_PRINTF("abc  ", "%-5s", "abc");
  EXPECT_PRINTF("abc  ", "%0--5s", "abc");
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
  EXPECT_PRINTF(
        fmt::format("0x{:x}", static_cast<unsigned>(-0x42)), "%#x", -0x42);
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
  EXPECT_THROW_MSG(fmt::sprintf("%5-5d", 42), FormatError,
      "unknown format code '-' for integer");

  EXPECT_THROW_MSG(fmt::sprintf(format("%{}d", BIG_NUM), 42),
      FormatError, "number is too big");
  EXPECT_THROW_MSG(fmt::sprintf(format("%1${}d", BIG_NUM), 42),
      FormatError, "number is too big");
}

TEST(PrintfTest, DynamicWidth) {
  EXPECT_EQ("   42", fmt::sprintf("%*d", 5, 42));
  EXPECT_EQ("42   ", fmt::sprintf("%*d", -5, 42));
  EXPECT_THROW_MSG(fmt::sprintf("%*d", 5.0, 42), FormatError,
      "width is not integer");
  EXPECT_THROW_MSG(fmt::sprintf("%*d"), FormatError,
      "argument index out of range");
  EXPECT_THROW_MSG(fmt::sprintf("%*d", BIG_NUM, 42), FormatError,
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
  safe_sprintf(buffer, "%.3g", 1234.5678);
  EXPECT_PRINTF(buffer, "%.3g", 1234.5678);
  safe_sprintf(buffer, "%.3a", 1234.5678);
  EXPECT_PRINTF(buffer, "%.3a", 1234.5678);
}

TEST(PrintfTest, IgnorePrecisionForNonNumericArg) {
  EXPECT_PRINTF("abc", "%.5s", "abc");
}

TEST(PrintfTest, DynamicPrecision) {
  EXPECT_EQ("00042", fmt::sprintf("%.*d", 5, 42));
  EXPECT_EQ("42", fmt::sprintf("%.*d", -5, 42));
  EXPECT_THROW_MSG(fmt::sprintf("%.*d", 5.0, 42), FormatError,
      "precision is not integer");
  EXPECT_THROW_MSG(fmt::sprintf("%.*d"), FormatError,
      "argument index out of range");
  EXPECT_THROW_MSG(fmt::sprintf("%.*d", BIG_NUM, 42), FormatError,
      "number is too big");
  if (sizeof(fmt::LongLong) != sizeof(int)) {
    fmt::LongLong prec = static_cast<fmt::LongLong>(INT_MIN) - 1;
    EXPECT_THROW_MSG(fmt::sprintf("%.*d", prec, 42), FormatError,
        "number is too big");
 }
}

template <typename T>
struct MakeSigned { typedef T Type; };

#define SPECIALIZE_MAKE_SIGNED(T, S) \
  template <> \
  struct MakeSigned<T> { typedef S Type; }

SPECIALIZE_MAKE_SIGNED(char, signed char);
SPECIALIZE_MAKE_SIGNED(unsigned char, signed char);
SPECIALIZE_MAKE_SIGNED(unsigned short, short);
SPECIALIZE_MAKE_SIGNED(unsigned, int);
SPECIALIZE_MAKE_SIGNED(unsigned long, long);
SPECIALIZE_MAKE_SIGNED(fmt::ULongLong, fmt::LongLong);

// Test length format specifier ``length_spec``.
template <typename T, typename U>
void TestLength(const char *length_spec, U value) {
  fmt::LongLong signed_value = 0;
  fmt::ULongLong unsigned_value = 0;
  // Apply integer promotion to the argument.
  fmt::ULongLong max = std::numeric_limits<U>::max();
  using fmt::internal::check;
  if (check(max <= static_cast<unsigned>(std::numeric_limits<int>::max()))) {
    signed_value = static_cast<int>(value);
    unsigned_value = static_cast<unsigned>(value);
  } else if (check(max <= std::numeric_limits<unsigned>::max())) {
    signed_value = static_cast<unsigned>(value);
    unsigned_value = static_cast<unsigned>(value);
  }
  using fmt::internal::MakeUnsigned;
  if (sizeof(U) <= sizeof(int) && sizeof(int) < sizeof(T)) {
    signed_value = static_cast<fmt::LongLong>(value);
    unsigned_value = static_cast<typename MakeUnsigned<unsigned>::Type>(value);
  } else {
    signed_value = static_cast<typename MakeSigned<T>::Type>(value);
    unsigned_value = static_cast<typename MakeUnsigned<T>::Type>(value);
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

template <typename T>
void TestLength(const char *length_spec) {
  T min = std::numeric_limits<T>::min(), max = std::numeric_limits<T>::max();
  TestLength<T>(length_spec, 42);
  TestLength<T>(length_spec, -42);
  TestLength<T>(length_spec, min);
  TestLength<T>(length_spec, max);
  TestLength<T>(length_spec, fmt::LongLong(min) - 1);
  fmt::ULongLong long_long_max = std::numeric_limits<fmt::LongLong>::max();
  if (static_cast<fmt::ULongLong>(max) < long_long_max)
    TestLength<T>(length_spec, fmt::LongLong(max) + 1);
  TestLength<T>(length_spec, std::numeric_limits<short>::min());
  TestLength<T>(length_spec, std::numeric_limits<unsigned short>::max());
  TestLength<T>(length_spec, std::numeric_limits<int>::min());
  TestLength<T>(length_spec, std::numeric_limits<int>::max());
  TestLength<T>(length_spec, std::numeric_limits<unsigned>::min());
  TestLength<T>(length_spec, std::numeric_limits<unsigned>::max());
  TestLength<T>(length_spec, std::numeric_limits<fmt::LongLong>::min());
  TestLength<T>(length_spec, std::numeric_limits<fmt::LongLong>::max());
  TestLength<T>(length_spec, std::numeric_limits<fmt::ULongLong>::min());
  TestLength<T>(length_spec, std::numeric_limits<fmt::ULongLong>::max());
}

TEST(PrintfTest, Length) {
  TestLength<char>("hh");
  TestLength<signed char>("hh");
  TestLength<unsigned char>("hh");
  TestLength<short>("h");
  TestLength<unsigned short>("h");
  TestLength<long>("l");
  TestLength<unsigned long>("l");
  TestLength<fmt::LongLong>("ll");
  TestLength<fmt::ULongLong>("ll");
  TestLength<intmax_t>("j");
  TestLength<std::size_t>("z");
  TestLength<std::ptrdiff_t>("t");
  long double max = std::numeric_limits<long double>::max();
  EXPECT_PRINTF(fmt::format("{}", max), "%g", max);
  EXPECT_PRINTF(fmt::format("{}", max), "%Lg", max);
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

TEST(PrintfTest, LongLong) {
  // fmt::printf allows passing long long arguments to %d without length
  // specifiers.
  fmt::LongLong max = std::numeric_limits<fmt::LongLong>::max();
  EXPECT_PRINTF(fmt::format("{}", max), "%d", max);
}

TEST(PrintfTest, Float) {
  EXPECT_PRINTF("392.650000", "%f", 392.65);
  EXPECT_PRINTF("392.650000", "%F", 392.65);
  char buffer[BUFFER_SIZE];
  safe_sprintf(buffer, "%e", 392.65);
  EXPECT_PRINTF(buffer, "%e", 392.65);
  safe_sprintf(buffer, "%E", 392.65);
  EXPECT_PRINTF(buffer, "%E", 392.65);
  EXPECT_PRINTF("392.65", "%g", 392.65);
  EXPECT_PRINTF("392.65", "%G", 392.65);
  safe_sprintf(buffer, "%a", -392.65);
  EXPECT_EQ(buffer, format("{:a}", -392.65));
  safe_sprintf(buffer, "%A", -392.65);
  EXPECT_EQ(buffer, format("{:A}", -392.65));
}

TEST(PrintfTest, Inf) {
  double inf = std::numeric_limits<double>::infinity();
  for (const char* type = "fega"; *type; ++type) {
    EXPECT_PRINTF("inf", fmt::format("%{}", *type), inf);
    char upper = std::toupper(*type);
    EXPECT_PRINTF("INF", fmt::format("%{}", upper), inf);
  }
}

TEST(PrintfTest, Char) {
  EXPECT_PRINTF("x", "%c", 'x');
  int max = std::numeric_limits<int>::max();
  EXPECT_PRINTF(fmt::format("{}", static_cast<char>(max)), "%c", max);
  //EXPECT_PRINTF("x", "%lc", L'x');
  // TODO: test wchar_t
}

TEST(PrintfTest, String) {
  EXPECT_PRINTF("abc", "%s", "abc");
  const char *null_str = 0;
  EXPECT_PRINTF("(null)", "%s", null_str);
  EXPECT_PRINTF("    (null)", "%10s", null_str);
  // TODO: wide string
}

TEST(PrintfTest, Pointer) {
  int n;
  void *p = &n;
  EXPECT_PRINTF(fmt::format("{}", p), "%p", p);
  p = 0;
  EXPECT_PRINTF("(nil)", "%p", p);
  EXPECT_PRINTF("     (nil)", "%10p", p);
  const char *s = "test";
  EXPECT_PRINTF(fmt::format("{:p}", s), "%p", s);
  const char *null_str = 0;
  EXPECT_PRINTF("(nil)", "%p", null_str);
}

TEST(PrintfTest, Location) {
  // TODO: test %n
}

enum E { A = 42 };

TEST(PrintfTest, Enum) {
  EXPECT_PRINTF("42", "%d", A);
}

#if FMT_USE_FILE_DESCRIPTORS
TEST(PrintfTest, Examples) {
  const char *weekday = "Thursday";
  const char *month = "August";
  int day = 21;
  EXPECT_WRITE(stdout, fmt::printf("%1$s, %3$d %2$s", weekday, month, day),
               "Thursday, 21 August");
}

TEST(PrintfTest, PrintfError) {
  fmt::File read_end, write_end;
  fmt::File::pipe(read_end, write_end);
  int result = fmt::fprintf(read_end.fdopen("r").get(), "test");
  EXPECT_LT(result, 0);
}
#endif

TEST(PrintfTest, WideString) {
  EXPECT_EQ(L"abc", fmt::sprintf(L"%s", L"abc"));
}
