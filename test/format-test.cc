/*
 Formatting library tests.

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
#include <cfloat>
#include <climits>
#include <cmath>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <memory>
#include <sstream>

#if defined(_WIN32) && !defined(__MINGW32__)
// Fix MSVC warning about "unsafe" fopen.
FILE *FOpen(const char *filename, const char *mode) {
  FILE *f = 0;
  errno = fopen_s(&f, filename, mode);
  return f;
}
#define fopen FOpen
#endif

#include "format.h"
#include "util.h"
#include "gtest-extra.h"

#include <stdint.h>

#undef min
#undef max

using std::size_t;

using fmt::internal::Array;
using fmt::BasicWriter;
using fmt::format;
using fmt::FormatError;
using fmt::StringRef;
using fmt::Writer;
using fmt::WWriter;
using fmt::pad;

namespace {

// Checks if writing value to BasicWriter<Char> produces the same result
// as writing it to std::basic_ostringstream<Char>.
template <typename Char, typename T>
static ::testing::AssertionResult CheckWrite(const T &value, const char *type) {
  std::basic_ostringstream<Char> os;
  os << value;
  std::basic_string<Char> expected = os.str();
  std::basic_string<Char> actual = (BasicWriter<Char>() << value).str();
  if (expected == actual)
    return ::testing::AssertionSuccess();
  return ::testing::AssertionFailure()
      << "Value of: (Writer<" << type << ">() << value).str()\n"
      << "  Actual: " << actual << "\n"
      << "Expected: " << expected << "\n";
}

struct AnyWriteChecker {
  template <typename T>
  ::testing::AssertionResult operator()(const char *, const T &value) const {
    ::testing::AssertionResult result = CheckWrite<char>(value, "char");
    return result ? CheckWrite<wchar_t>(value, "wchar_t") : result;
  }
};

template <typename Char>
struct WriteChecker {
  template <typename T>
  ::testing::AssertionResult operator()(const char *, const T &value) const {
    return CheckWrite<Char>(value, "char");
  }
};

// Checks if writing value to BasicWriter produces the same result
// as writing it to std::ostringstream both for char and wchar_t.
#define CHECK_WRITE(value) EXPECT_PRED_FORMAT1(AnyWriteChecker(), value)

#define CHECK_WRITE_CHAR(value) \
  EXPECT_PRED_FORMAT1(WriteChecker<char>(), value)
#define CHECK_WRITE_WCHAR(value) \
  EXPECT_PRED_FORMAT1(WriteChecker<wchar_t>(), value)

std::string ReadFile(StringRef filename) {
  std::ifstream out(filename.c_str());
  std::stringstream content;
  content << out.rdbuf();
  return content.str();
}
}

class TestString {
 private:
  std::string value_;

 public:
  explicit TestString(const char *value = "") : value_(value) {}

  friend std::ostream &operator<<(std::ostream &os, const TestString &s) {
    os << s.value_;
    return os;
  }
};

TEST(ArrayTest, Ctor) {
  Array<char, 123> array;
  EXPECT_EQ(0u, array.size());
  EXPECT_EQ(123u, array.capacity());
}

#if FMT_USE_RVALUE_REFERENCES

void CheckMoveArray(const char *str, Array<char, 5> &array) {
  Array<char, 5> array2(std::move(array));
  // Move shouldn't destroy the inline content of the first array.
  EXPECT_EQ(str, std::string(&array[0], array.size()));
  EXPECT_EQ(str, std::string(&array2[0], array2.size()));
  EXPECT_EQ(5, array2.capacity());
}

TEST(ArrayTest, MoveCtor) {
  Array<char, 5> array;
  const char test[] = "test";
  array.append(test, test + 4);
  CheckMoveArray("test", array);
  // Adding one more character fills the inline buffer, but doesn't cause
  // dynamic allocation.
  array.push_back('a');
  CheckMoveArray("testa", array);
  const char *inline_buffer_ptr = &array[0];
  // Adding one more character causes the content to move from the inline to
  // a dynamically allocated buffer.
  array.push_back('b');
  Array<char, 5> array2(std::move(array));
  // Move should rip the guts of the first array.
  EXPECT_EQ(inline_buffer_ptr, &array[0]);
  EXPECT_EQ("testab", std::string(&array2[0], array2.size()));
  EXPECT_GT(array2.capacity(), 5u);
}

void CheckMoveAssignArray(const char *str, Array<char, 5> &array) {
  Array<char, 5> array2;
  array2 = std::move(array);
  // Move shouldn't destroy the inline content of the first array.
  EXPECT_EQ(str, std::string(&array[0], array.size()));
  EXPECT_EQ(str, std::string(&array2[0], array2.size()));
  EXPECT_EQ(5, array2.capacity());
}

TEST(ArrayTest, MoveAssignment) {
  Array<char, 5> array;
  const char test[] = "test";
  array.append(test, test + 4);
  CheckMoveAssignArray("test", array);
  // Adding one more character fills the inline buffer, but doesn't cause
  // dynamic allocation.
  array.push_back('a');
  CheckMoveAssignArray("testa", array);
  const char *inline_buffer_ptr = &array[0];
  // Adding one more character causes the content to move from the inline to
  // a dynamically allocated buffer.
  array.push_back('b');
  Array<char, 5> array2;
  array2 = std::move(array);
  // Move should rip the guts of the first array.
  EXPECT_EQ(inline_buffer_ptr, &array[0]);
  EXPECT_EQ("testab", std::string(&array2[0], array2.size()));
  EXPECT_GT(array2.capacity(), 5u);
}

#endif  // FMT_USE_RVALUE_REFERENCES

TEST(ArrayTest, Access) {
  Array<char, 10> array;
  array[0] = 11;
  EXPECT_EQ(11, array[0]);
  array[3] = 42;
  EXPECT_EQ(42, *(&array[0] + 3));
  const Array<char, 10> &carray = array;
  EXPECT_EQ(42, carray[3]);
}

TEST(ArrayTest, Resize) {
  Array<char, 123> array;
  array[10] = 42;
  EXPECT_EQ(42, array[10]);
  array.resize(20);
  EXPECT_EQ(20u, array.size());
  EXPECT_EQ(123u, array.capacity());
  EXPECT_EQ(42, array[10]);
  array.resize(5);
  EXPECT_EQ(5u, array.size());
  EXPECT_EQ(123u, array.capacity());
  EXPECT_EQ(42, array[10]);
}

TEST(ArrayTest, Grow) {
  Array<int, 10> array;
  array.resize(10);
  for (int i = 0; i < 10; ++i)
    array[i] = i * i;
  array.resize(20);
  EXPECT_EQ(20u, array.size());
  EXPECT_EQ(20u, array.capacity());
  for (int i = 0; i < 10; ++i)
    EXPECT_EQ(i * i, array[i]);
}

TEST(ArrayTest, Clear) {
  Array<char, 10> array;
  array.resize(20);
  array.clear();
  EXPECT_EQ(0u, array.size());
  EXPECT_EQ(20u, array.capacity());
}

TEST(ArrayTest, PushBack) {
  Array<int, 10> array;
  array.push_back(11);
  EXPECT_EQ(11, array[0]);
  EXPECT_EQ(1u, array.size());
  array.resize(10);
  array.push_back(22);
  EXPECT_EQ(22, array[10]);
  EXPECT_EQ(11u, array.size());
  EXPECT_EQ(15u, array.capacity());
}

TEST(ArrayTest, Append) {
  Array<char, 10> array;
  const char *test = "test";
  array.append(test, test + 5);
  EXPECT_STREQ(test, &array[0]);
  EXPECT_EQ(5u, array.size());
  array.resize(10);
  array.append(test, test + 2);
  EXPECT_EQ('t', array[10]);
  EXPECT_EQ('e', array[11]);
  EXPECT_EQ(12u, array.size());
  EXPECT_EQ(15u, array.capacity());
}

TEST(ArrayTest, AppendAllocatesEnoughStorage) {
  Array<char, 10> array;
  const char *test = "abcdefgh";
  array.resize(10);
  array.append(test, test + 9);
  EXPECT_STREQ(test, &array[10]);
  EXPECT_EQ(19u, array.size());
  EXPECT_EQ(19u, array.capacity());
}

TEST(WriterTest, Ctor) {
  Writer w;
  EXPECT_EQ(0u, w.size());
  EXPECT_STREQ("", w.c_str());
  EXPECT_EQ("", w.str());
}

#if FMT_USE_RVALUE_REFERENCES

void CheckMoveWriter(const std::string &str, Writer &w) {
  Writer w2(std::move(w));
  // Move shouldn't destroy the inline content of the first writer.
  EXPECT_EQ(str, w.str());
  EXPECT_EQ(str, w2.str());
}

TEST(WriterTest, MoveCtor) {
  Writer w;
  w << "test";
  CheckMoveWriter("test", w);
  // This fills the inline buffer, but doesn't cause dynamic allocation.
  std::string s;
  for (int i = 0; i < fmt::internal::INLINE_BUFFER_SIZE; ++i)
    s += '*';
  w.clear();
  w << s;
  CheckMoveWriter(s, w);
  const char *inline_buffer_ptr = w.data();
  // Adding one more character causes the content to move from the inline to
  // a dynamically allocated buffer.
  w << '*';
  Writer w2(std::move(w));
  // Move should rip the guts of the first writer.
  EXPECT_EQ(inline_buffer_ptr, w.data());
  EXPECT_EQ(s + '*', w2.str());
}

void CheckMoveAssignWriter(const std::string &str, Writer &w) {
  Writer w2;
  w2 = std::move(w);
  // Move shouldn't destroy the inline content of the first writer.
  EXPECT_EQ(str, w.str());
  EXPECT_EQ(str, w2.str());
}

TEST(WriterTest, MoveAssignment) {
  Writer w;
  w << "test";
  CheckMoveAssignWriter("test", w);
  // This fills the inline buffer, but doesn't cause dynamic allocation.
  std::string s;
  for (int i = 0; i < fmt::internal::INLINE_BUFFER_SIZE; ++i)
    s += '*';
  w.clear();
  w << s;
  CheckMoveAssignWriter(s, w);
  const char *inline_buffer_ptr = w.data();
  // Adding one more character causes the content to move from the inline to
  // a dynamically allocated buffer.
  w << '*';
  Writer w2;
  w2 = std::move(w);
  // Move should rip the guts of the first writer.
  EXPECT_EQ(inline_buffer_ptr, w.data());
  EXPECT_EQ(s + '*', w2.str());
}

#endif  // FMT_USE_RVALUE_REFERENCES

TEST(WriterTest, Data) {
  Writer w;
  w << 42;
  EXPECT_EQ("42", std::string(w.data(), w.size()));
}

TEST(WriterTest, WriteInt) {
  CHECK_WRITE(42);
  CHECK_WRITE(-42);
  CHECK_WRITE(static_cast<short>(12));
  CHECK_WRITE(34u);
  CHECK_WRITE(std::numeric_limits<int>::min());
  CHECK_WRITE(std::numeric_limits<int>::max());
  CHECK_WRITE(std::numeric_limits<unsigned>::max());
}

TEST(WriterTest, WriteLong) {
  CHECK_WRITE(56l);
  CHECK_WRITE(78ul);
  CHECK_WRITE(std::numeric_limits<long>::min());
  CHECK_WRITE(std::numeric_limits<long>::max());
  CHECK_WRITE(std::numeric_limits<unsigned long>::max());
}

TEST(WriterTest, WriteLongLong) {
  CHECK_WRITE(56ll);
  CHECK_WRITE(78ull);
  CHECK_WRITE(std::numeric_limits<long long>::min());
  CHECK_WRITE(std::numeric_limits<long long>::max());
  CHECK_WRITE(std::numeric_limits<unsigned long long>::max());
}

TEST(WriterTest, WriteDouble) {
  CHECK_WRITE(4.2);
  CHECK_WRITE(-4.2);
  CHECK_WRITE(4.2l);
}

TEST(WriterTest, WriteDoubleAtBufferBoundary) {
  fmt::Writer writer;
  for (int i = 0; i < 100; ++i)
    writer << 1.23456789;
}

TEST(WriterTest, WriteDoubleWithFilledBuffer) {
  fmt::Writer writer;
  // Fill the buffer.
  for (int i = 0; i < fmt::internal::INLINE_BUFFER_SIZE; ++i)
    writer << ' ';
  writer << 1.2;
  EXPECT_STREQ("1.2", writer.c_str() + fmt::internal::INLINE_BUFFER_SIZE);
}

TEST(WriterTest, WriteChar) {
  CHECK_WRITE('a');
}

TEST(WriterTest, WriteWideChar) {
  CHECK_WRITE_WCHAR(L'a');
}

TEST(WriterTest, WriteString) {
  CHECK_WRITE_CHAR("abc");
  // The following line shouldn't compile:
  //fmt::Writer() << L"abc";
}

TEST(WriterTest, WriteWideString) {
  CHECK_WRITE_WCHAR(L"abc");
  // The following line shouldn't compile:
  //fmt::WWriter() << "abc";
}

TEST(WriterTest, bin) {
  using fmt::bin;
  EXPECT_EQ("1100101011111110", (Writer() << bin(0xcafe)).str());
  EXPECT_EQ("1011101010111110", (Writer() << bin(0xbabeu)).str());
  EXPECT_EQ("1101111010101101", (Writer() << bin(0xdeadl)).str());
  EXPECT_EQ("1011111011101111", (Writer() << bin(0xbeeful)).str());
  EXPECT_EQ("11001010111111101011101010111110",
            (Writer() << bin(0xcafebabell)).str());
  EXPECT_EQ("11011110101011011011111011101111",
            (Writer() << bin(0xdeadbeefull)).str());
}

TEST(WriterTest, oct) {
  using fmt::oct;
  EXPECT_EQ("12", (Writer() << oct(static_cast<short>(012))).str());
  EXPECT_EQ("12", (Writer() << oct(012)).str());
  EXPECT_EQ("34", (Writer() << oct(034u)).str());
  EXPECT_EQ("56", (Writer() << oct(056l)).str());
  EXPECT_EQ("70", (Writer() << oct(070ul)).str());
  EXPECT_EQ("1234", (Writer() << oct(01234ll)).str());
  EXPECT_EQ("5670", (Writer() << oct(05670ull)).str());
}

TEST(WriterTest, hex) {
  using fmt::hex;
  fmt::IntFormatSpec<int, fmt::TypeSpec<'x'> > (*phex)(int value) = hex;
  phex(42);
  // This shouldn't compile:
  //fmt::IntFormatSpec<short, fmt::TypeSpec<'x'> > (*phex2)(short value) = hex;

  EXPECT_EQ("cafe", (Writer() << hex(0xcafe)).str());
  EXPECT_EQ("babe", (Writer() << hex(0xbabeu)).str());
  EXPECT_EQ("dead", (Writer() << hex(0xdeadl)).str());
  EXPECT_EQ("beef", (Writer() << hex(0xbeeful)).str());
  EXPECT_EQ("cafebabe", (Writer() << hex(0xcafebabell)).str());
  EXPECT_EQ("deadbeef", (Writer() << hex(0xdeadbeefull)).str());
}

TEST(WriterTest, hexu) {
  using fmt::hexu;
  EXPECT_EQ("CAFE", (Writer() << hexu(0xcafe)).str());
  EXPECT_EQ("BABE", (Writer() << hexu(0xbabeu)).str());
  EXPECT_EQ("DEAD", (Writer() << hexu(0xdeadl)).str());
  EXPECT_EQ("BEEF", (Writer() << hexu(0xbeeful)).str());
  EXPECT_EQ("CAFEBABE", (Writer() << hexu(0xcafebabell)).str());
  EXPECT_EQ("DEADBEEF", (Writer() << hexu(0xdeadbeefull)).str());
}

class Date {
  int year_, month_, day_;
 public:
  Date(int year, int month, int day) : year_(year), month_(month), day_(day) {}

  int year() const { return year_; }
  int month() const { return month_; }
  int day() const { return day_; }

  friend std::ostream &operator<<(std::ostream &os, const Date &d) {
    os << d.year_ << '-' << d.month_ << '-' << d.day_;
    return os;
  }

  template <typename Char>
  friend BasicWriter<Char> &operator<<(BasicWriter<Char> &f, const Date &d) {
    return f << d.year_ << '-' << d.month_ << '-' << d.day_;
  }
};

class ISO8601DateFormatter {
 const Date *date_;

public:
  ISO8601DateFormatter(const Date &d) : date_(&d) {}

  template <typename Char>
  friend BasicWriter<Char> &operator<<(
      BasicWriter<Char> &w, const ISO8601DateFormatter &d) {
    return w << pad(d.date_->year(), 4, '0') << '-'
        << pad(d.date_->month(), 2, '0') << '-' << pad(d.date_->day(), 2, '0');
  }
};

ISO8601DateFormatter iso8601(const Date &d) { return ISO8601DateFormatter(d); }

TEST(WriterTest, pad) {
  using fmt::hex;
  EXPECT_EQ("    cafe", (Writer() << pad(hex(0xcafe), 8)).str());
  EXPECT_EQ("    babe", (Writer() << pad(hex(0xbabeu), 8)).str());
  EXPECT_EQ("    dead", (Writer() << pad(hex(0xdeadl), 8)).str());
  EXPECT_EQ("    beef", (Writer() << pad(hex(0xbeeful), 8)).str());
  EXPECT_EQ("    dead", (Writer() << pad(hex(0xdeadll), 8)).str());
  EXPECT_EQ("    beef", (Writer() << pad(hex(0xbeefull), 8)).str());

  EXPECT_EQ("     11", (Writer() << pad(11, 7)).str());
  EXPECT_EQ("     22", (Writer() << pad(22u, 7)).str());
  EXPECT_EQ("     33", (Writer() << pad(33l, 7)).str());
  EXPECT_EQ("     44", (Writer() << pad(44ul, 7)).str());
  EXPECT_EQ("     33", (Writer() << pad(33ll, 7)).str());
  EXPECT_EQ("     44", (Writer() << pad(44ull, 7)).str());

  BasicWriter<char> w;
  w.clear();
  w << pad(42, 5, '0');
  EXPECT_EQ("00042", w.str());
  w.clear();
  w << Date(2012, 12, 9);
  EXPECT_EQ("2012-12-9", w.str());
  w.clear();
  w << iso8601(Date(2012, 1, 9));
  EXPECT_EQ("2012-01-09", w.str());
}

TEST(WriterTest, PadString) {
  EXPECT_EQ("test    ", (Writer() << pad("test", 8)).str());
  EXPECT_EQ("test******", (Writer() << pad("test", 10, '*')).str());
}

TEST(WriterTest, PadWString) {
  EXPECT_EQ(L"test    ", (WWriter() << pad(L"test", 8)).str());
  EXPECT_EQ(L"test******", (WWriter() << pad(L"test", 10, '*')).str());
  EXPECT_EQ(L"test******", (WWriter() << pad(L"test", 10, L'*')).str());
}

TEST(WriterTest, NoConflictWithIOManip) {
  using namespace std;
  using namespace fmt;
  EXPECT_EQ("cafe", (Writer() << hex(0xcafe)).str());
  EXPECT_EQ("12", (Writer() << oct(012)).str());
}

TEST(WriterTest, Format) {
  Writer w;
  w.write("part{0}", 1);
  EXPECT_EQ(strlen("part1"), w.size());
  EXPECT_STREQ("part1", w.c_str());
  EXPECT_STREQ("part1", w.data());
  EXPECT_EQ("part1", w.str());
  w.write("part{0}", 2);
  EXPECT_EQ(strlen("part1part2"), w.size());
  EXPECT_STREQ("part1part2", w.c_str());
  EXPECT_STREQ("part1part2", w.data());
  EXPECT_EQ("part1part2", w.str());
}

TEST(WriterTest, WWriter) {
  EXPECT_EQ(L"cafe", (fmt::WWriter() << fmt::hex(0xcafe)).str());
}

TEST(FormatterTest, Escape) {
  EXPECT_EQ("{", format("{{"));
  EXPECT_EQ("before {", format("before {{"));
  EXPECT_EQ("{ after", format("{{ after"));
  EXPECT_EQ("before { after", format("before {{ after"));

  EXPECT_EQ("}", format("}}"));
  EXPECT_EQ("before }", format("before }}"));
  EXPECT_EQ("} after", format("}} after"));
  EXPECT_EQ("before } after", format("before }} after"));

  EXPECT_EQ("{}", format("{{}}"));
  EXPECT_EQ("{42}", format("{{{0}}}", 42));
}

TEST(FormatterTest, UnmatchedBraces) {
  EXPECT_THROW_MSG(format("{"), FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(format("}"), FormatError, "unmatched '}' in format");
  EXPECT_THROW_MSG(format("{0{}"), FormatError, "unmatched '{' in format");
}

TEST(FormatterTest, NoArgs) {
  EXPECT_EQ("test", format("test"));
}

TEST(FormatterTest, ArgsInDifferentPositions) {
  EXPECT_EQ("42", format("{0}", 42));
  EXPECT_EQ("before 42", format("before {0}", 42));
  EXPECT_EQ("42 after", format("{0} after", 42));
  EXPECT_EQ("before 42 after", format("before {0} after", 42));
  EXPECT_EQ("answer = 42", format("{0} = {1}", "answer", 42));
  EXPECT_EQ("42 is the answer", format("{1} is the {0}", "answer", 42));
  EXPECT_EQ("abracadabra", format("{0}{1}{0}", "abra", "cad"));
}

TEST(FormatterTest, ArgErrors) {
  EXPECT_THROW_MSG(format("{"), FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(format("{x}"), FormatError,
      "invalid argument index in format string");
  EXPECT_THROW_MSG(format("{0"), FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(format("{0}"), FormatError,
      "argument index is out of range in format");

  char format_str[BUFFER_SIZE];
  SPrintf(format_str, "{%u", INT_MAX);
  EXPECT_THROW_MSG(format(format_str), FormatError, "unmatched '{' in format");
  SPrintf(format_str, "{%u}", INT_MAX);
  EXPECT_THROW_MSG(format(format_str), FormatError,
      "argument index is out of range in format");

  SPrintf(format_str, "{%u", INT_MAX + 1u);
  EXPECT_THROW_MSG(format(format_str), FormatError, "unmatched '{' in format");
  SPrintf(format_str, "{%u}", INT_MAX + 1u);
  EXPECT_THROW_MSG(format(format_str), FormatError, "number is too big in format");
}

TEST(FormatterTest, AutoArgIndex) {
  EXPECT_EQ("abc", format("{}{}{}", 'a', 'b', 'c'));
  EXPECT_THROW_MSG(format("{0}{}", 'a', 'b'),
      FormatError, "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG(format("{}{0}", 'a', 'b'),
      FormatError, "cannot switch from automatic to manual argument indexing");
  EXPECT_EQ("1.2", format("{:.{}}", 1.2345, 2));
  EXPECT_THROW_MSG(format("{0}:.{}", 1.2345, 2),
      FormatError, "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG(format("{:.{0}}", 1.2345, 2),
      FormatError, "cannot switch from automatic to manual argument indexing");
  EXPECT_THROW_MSG(format("{}"), FormatError,
      "argument index is out of range in format");
}

TEST(FormatterTest, EmptySpecs) {
  EXPECT_EQ("42", format("{0:}", 42));
}

TEST(FormatterTest, LeftAlign) {
  EXPECT_EQ("42  ", format("{0:<4}", 42));
  EXPECT_EQ("42  ", format("{0:<4o}", 042));
  EXPECT_EQ("42  ", format("{0:<4x}", 0x42));
  EXPECT_EQ("-42  ", format("{0:<5}", -42));
  EXPECT_EQ("42   ", format("{0:<5}", 42u));
  EXPECT_EQ("-42  ", format("{0:<5}", -42l));
  EXPECT_EQ("42   ", format("{0:<5}", 42ul));
  EXPECT_EQ("-42  ", format("{0:<5}", -42ll));
  EXPECT_EQ("42   ", format("{0:<5}", 42ull));
  EXPECT_EQ("-42  ", format("{0:<5}", -42.0));
  EXPECT_EQ("-42  ", format("{0:<5}", -42.0l));
  EXPECT_EQ("c    ", format("{0:<5}", 'c'));
  EXPECT_EQ("abc  ", format("{0:<5}", "abc"));
  EXPECT_EQ("0xface  ", format("{0:<8}", reinterpret_cast<void*>(0xface)));
  EXPECT_EQ("def  ", format("{0:<5}", TestString("def")));
}

TEST(FormatterTest, RightAlign) {
  EXPECT_EQ("  42", format("{0:>4}", 42));
  EXPECT_EQ("  42", format("{0:>4o}", 042));
  EXPECT_EQ("  42", format("{0:>4x}", 0x42));
  EXPECT_EQ("  -42", format("{0:>5}", -42));
  EXPECT_EQ("   42", format("{0:>5}", 42u));
  EXPECT_EQ("  -42", format("{0:>5}", -42l));
  EXPECT_EQ("   42", format("{0:>5}", 42ul));
  EXPECT_EQ("  -42", format("{0:>5}", -42ll));
  EXPECT_EQ("   42", format("{0:>5}", 42ull));
  EXPECT_EQ("  -42", format("{0:>5}", -42.0));
  EXPECT_EQ("  -42", format("{0:>5}", -42.0l));
  EXPECT_EQ("    c", format("{0:>5}", 'c'));
  EXPECT_EQ("  abc", format("{0:>5}", "abc"));
  EXPECT_EQ("  0xface", format("{0:>8}", reinterpret_cast<void*>(0xface)));
  EXPECT_EQ("  def", format("{0:>5}", TestString("def")));
}

TEST(FormatterTest, NumericAlign) {
  EXPECT_EQ("  42", format("{0:=4}", 42));
  EXPECT_EQ("+ 42", format("{0:=+4}", 42));
  EXPECT_EQ("  42", format("{0:=4o}", 042));
  EXPECT_EQ("+ 42", format("{0:=+4o}", 042));
  EXPECT_EQ("  42", format("{0:=4x}", 0x42));
  EXPECT_EQ("+ 42", format("{0:=+4x}", 0x42));
  EXPECT_EQ("-  42", format("{0:=5}", -42));
  EXPECT_EQ("   42", format("{0:=5}", 42u));
  EXPECT_EQ("-  42", format("{0:=5}", -42l));
  EXPECT_EQ("   42", format("{0:=5}", 42ul));
  EXPECT_EQ("-  42", format("{0:=5}", -42ll));
  EXPECT_EQ("   42", format("{0:=5}", 42ull));
  EXPECT_EQ("-  42", format("{0:=5}", -42.0));
  EXPECT_EQ("-  42", format("{0:=5}", -42.0l));
  EXPECT_THROW_MSG(format("{0:=5", 'c'),
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(format("{0:=5}", 'c'),
      FormatError, "format specifier '=' requires numeric argument");
  EXPECT_THROW_MSG(format("{0:=5}", "abc"),
      FormatError, "format specifier '=' requires numeric argument");
  EXPECT_THROW_MSG(format("{0:=8}", reinterpret_cast<void*>(0xface)),
      FormatError, "format specifier '=' requires numeric argument");
  EXPECT_THROW_MSG(format("{0:=5}", TestString("def")),
      FormatError, "format specifier '=' requires numeric argument");
}

TEST(FormatterTest, CenterAlign) {
  EXPECT_EQ(" 42  ", format("{0:^5}", 42));
  EXPECT_EQ(" 42  ", format("{0:^5o}", 042));
  EXPECT_EQ(" 42  ", format("{0:^5x}", 0x42));
  EXPECT_EQ(" -42 ", format("{0:^5}", -42));
  EXPECT_EQ(" 42  ", format("{0:^5}", 42u));
  EXPECT_EQ(" -42 ", format("{0:^5}", -42l));
  EXPECT_EQ(" 42  ", format("{0:^5}", 42ul));
  EXPECT_EQ(" -42 ", format("{0:^5}", -42ll));
  EXPECT_EQ(" 42  ", format("{0:^5}", 42ull));
  EXPECT_EQ(" -42  ", format("{0:^6}", -42.0));
  EXPECT_EQ(" -42 ", format("{0:^5}", -42.0l));
  EXPECT_EQ("  c  ", format("{0:^5}", 'c'));
  EXPECT_EQ(" abc  ", format("{0:^6}", "abc"));
  EXPECT_EQ(" 0xface ", format("{0:^8}", reinterpret_cast<void*>(0xface)));
  EXPECT_EQ(" def ", format("{0:^5}", TestString("def")));
}

TEST(FormatterTest, Fill) {
  EXPECT_THROW_MSG(format("{0:{<5}", 'c'),
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(format("{0:{<5}}", 'c'),
      FormatError, "invalid fill character '{'");
  EXPECT_EQ("**42", format("{0:*>4}", 42));
  EXPECT_EQ("**-42", format("{0:*>5}", -42));
  EXPECT_EQ("***42", format("{0:*>5}", 42u));
  EXPECT_EQ("**-42", format("{0:*>5}", -42l));
  EXPECT_EQ("***42", format("{0:*>5}", 42ul));
  EXPECT_EQ("**-42", format("{0:*>5}", -42ll));
  EXPECT_EQ("***42", format("{0:*>5}", 42ull));
  EXPECT_EQ("**-42", format("{0:*>5}", -42.0));
  EXPECT_EQ("**-42", format("{0:*>5}", -42.0l));
  EXPECT_EQ("c****", format("{0:*<5}", 'c'));
  EXPECT_EQ("abc**", format("{0:*<5}", "abc"));
  EXPECT_EQ("**0xface", format("{0:*>8}", reinterpret_cast<void*>(0xface)));
  EXPECT_EQ("def**", format("{0:*<5}", TestString("def")));
}

TEST(FormatterTest, PlusSign) {
  EXPECT_EQ("+42", format("{0:+}", 42));
  EXPECT_EQ("-42", format("{0:+}", -42));
  EXPECT_EQ("+42", format("{0:+}", 42));
  EXPECT_THROW_MSG(format("{0:+}", 42u),
      FormatError, "format specifier '+' requires signed argument");
  EXPECT_EQ("+42", format("{0:+}", 42l));
  EXPECT_THROW_MSG(format("{0:+}", 42ul),
      FormatError, "format specifier '+' requires signed argument");
  EXPECT_EQ("+42", format("{0:+}", 42ll));
  EXPECT_THROW_MSG(format("{0:+}", 42ull),
      FormatError, "format specifier '+' requires signed argument");
  EXPECT_EQ("+42", format("{0:+}", 42.0));
  EXPECT_EQ("+42", format("{0:+}", 42.0l));
  EXPECT_THROW_MSG(format("{0:+", 'c'),
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(format("{0:+}", 'c'),
      FormatError, "format specifier '+' requires numeric argument");
  EXPECT_THROW_MSG(format("{0:+}", "abc"),
      FormatError, "format specifier '+' requires numeric argument");
  EXPECT_THROW_MSG(format("{0:+}", reinterpret_cast<void*>(0x42)),
      FormatError, "format specifier '+' requires numeric argument");
  EXPECT_THROW_MSG(format("{0:+}", TestString()),
      FormatError, "format specifier '+' requires numeric argument");
}

TEST(FormatterTest, MinusSign) {
  EXPECT_EQ("42", format("{0:-}", 42));
  EXPECT_EQ("-42", format("{0:-}", -42));
  EXPECT_EQ("42", format("{0:-}", 42));
  EXPECT_THROW_MSG(format("{0:-}", 42u),
      FormatError, "format specifier '-' requires signed argument");
  EXPECT_EQ("42", format("{0:-}", 42l));
  EXPECT_THROW_MSG(format("{0:-}", 42ul),
      FormatError, "format specifier '-' requires signed argument");
  EXPECT_EQ("42", format("{0:-}", 42ll));
  EXPECT_THROW_MSG(format("{0:-}", 42ull),
      FormatError, "format specifier '-' requires signed argument");
  EXPECT_EQ("42", format("{0:-}", 42.0));
  EXPECT_EQ("42", format("{0:-}", 42.0l));
  EXPECT_THROW_MSG(format("{0:-", 'c'),
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(format("{0:-}", 'c'),
      FormatError, "format specifier '-' requires numeric argument");
  EXPECT_THROW_MSG(format("{0:-}", "abc"),
      FormatError, "format specifier '-' requires numeric argument");
  EXPECT_THROW_MSG(format("{0:-}", reinterpret_cast<void*>(0x42)),
      FormatError, "format specifier '-' requires numeric argument");
  EXPECT_THROW_MSG(format("{0:-}", TestString()),
      FormatError, "format specifier '-' requires numeric argument");
}

TEST(FormatterTest, SpaceSign) {
  EXPECT_EQ(" 42", format("{0: }", 42));
  EXPECT_EQ("-42", format("{0: }", -42));
  EXPECT_EQ(" 42", format("{0: }", 42));
  EXPECT_THROW_MSG(format("{0: }", 42u),
      FormatError, "format specifier ' ' requires signed argument");
  EXPECT_EQ(" 42", format("{0: }", 42l));
  EXPECT_THROW_MSG(format("{0: }", 42ul),
      FormatError, "format specifier ' ' requires signed argument");
  EXPECT_EQ(" 42", format("{0: }", 42ll));
  EXPECT_THROW_MSG(format("{0: }", 42ull),
      FormatError, "format specifier ' ' requires signed argument");
  EXPECT_EQ(" 42", format("{0: }", 42.0));
  EXPECT_EQ(" 42", format("{0: }", 42.0l));
  EXPECT_THROW_MSG(format("{0: ", 'c'),
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(format("{0: }", 'c'),
      FormatError, "format specifier ' ' requires numeric argument");
  EXPECT_THROW_MSG(format("{0: }", "abc"),
      FormatError, "format specifier ' ' requires numeric argument");
  EXPECT_THROW_MSG(format("{0: }", reinterpret_cast<void*>(0x42)),
      FormatError, "format specifier ' ' requires numeric argument");
  EXPECT_THROW_MSG(format("{0: }", TestString()),
      FormatError, "format specifier ' ' requires numeric argument");
}

TEST(FormatterTest, HashFlag) {
  EXPECT_EQ("42", format("{0:#}", 42));
  EXPECT_EQ("-42", format("{0:#}", -42));
  EXPECT_EQ("0b101010", format("{0:#b}", 42));
  EXPECT_EQ("0B101010", format("{0:#B}", 42));
  EXPECT_EQ("-0b101010", format("{0:#b}", -42));
  EXPECT_EQ("0x42", format("{0:#x}", 0x42));
  EXPECT_EQ("0X42", format("{0:#X}", 0x42));
  EXPECT_EQ("-0x42", format("{0:#x}", -0x42));
  EXPECT_EQ("042", format("{0:#o}", 042));
  EXPECT_EQ("-042", format("{0:#o}", -042));
  EXPECT_EQ("42", format("{0:#}", 42u));
  EXPECT_EQ("0x42", format("{0:#x}", 0x42u));
  EXPECT_EQ("042", format("{0:#o}", 042u));

  EXPECT_EQ("-42", format("{0:#}", -42l));
  EXPECT_EQ("0x42", format("{0:#x}", 0x42l));
  EXPECT_EQ("-0x42", format("{0:#x}", -0x42l));
  EXPECT_EQ("042", format("{0:#o}", 042l));
  EXPECT_EQ("-042", format("{0:#o}", -042l));
  EXPECT_EQ("42", format("{0:#}", 42ul));
  EXPECT_EQ("0x42", format("{0:#x}", 0x42ul));
  EXPECT_EQ("042", format("{0:#o}", 042ul));

  EXPECT_EQ("-42", format("{0:#}", -42ll));
  EXPECT_EQ("0x42", format("{0:#x}", 0x42ll));
  EXPECT_EQ("-0x42", format("{0:#x}", -0x42ll));
  EXPECT_EQ("042", format("{0:#o}", 042ll));
  EXPECT_EQ("-042", format("{0:#o}", -042ll));
  EXPECT_EQ("42", format("{0:#}", 42ull));
  EXPECT_EQ("0x42", format("{0:#x}", 0x42ull));
  EXPECT_EQ("042", format("{0:#o}", 042ull));

  EXPECT_EQ("-42.0000", format("{0:#}", -42.0));
  EXPECT_EQ("-42.0000", format("{0:#}", -42.0l));
  EXPECT_THROW_MSG(format("{0:#", 'c'),
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(format("{0:#}", 'c'),
      FormatError, "format specifier '#' requires numeric argument");
  EXPECT_THROW_MSG(format("{0:#}", "abc"),
      FormatError, "format specifier '#' requires numeric argument");
  EXPECT_THROW_MSG(format("{0:#}", reinterpret_cast<void*>(0x42)),
      FormatError, "format specifier '#' requires numeric argument");
  EXPECT_THROW_MSG(format("{0:#}", TestString()),
      FormatError, "format specifier '#' requires numeric argument");
}

TEST(FormatterTest, ZeroFlag) {
  EXPECT_EQ("42", format("{0:0}", 42));
  EXPECT_EQ("-0042", format("{0:05}", -42));
  EXPECT_EQ("00042", format("{0:05}", 42u));
  EXPECT_EQ("-0042", format("{0:05}", -42l));
  EXPECT_EQ("00042", format("{0:05}", 42ul));
  EXPECT_EQ("-0042", format("{0:05}", -42ll));
  EXPECT_EQ("00042", format("{0:05}", 42ull));
  EXPECT_EQ("-0042", format("{0:05}", -42.0));
  EXPECT_EQ("-0042", format("{0:05}", -42.0l));
  EXPECT_THROW_MSG(format("{0:0", 'c'),
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(format("{0:05}", 'c'),
      FormatError, "format specifier '0' requires numeric argument");
  EXPECT_THROW_MSG(format("{0:05}", "abc"),
      FormatError, "format specifier '0' requires numeric argument");
  EXPECT_THROW_MSG(format("{0:05}", reinterpret_cast<void*>(0x42)),
      FormatError, "format specifier '0' requires numeric argument");
  EXPECT_THROW_MSG(format("{0:05}", TestString()),
      FormatError, "format specifier '0' requires numeric argument");
}

TEST(FormatterTest, Width) {
  char format_str[BUFFER_SIZE];
  SPrintf(format_str, "{0:%u", UINT_MAX);
  Increment(format_str + 3);
  EXPECT_THROW_MSG(format(format_str), FormatError, "unmatched '{' in format");
  std::size_t size = std::strlen(format_str);
  format_str[size] = '}';
  format_str[size + 1] = 0;
  EXPECT_THROW_MSG(format(format_str, 0),
      FormatError, "number is too big in format");

  SPrintf(format_str, "{0:%u", INT_MAX + 1u);
  EXPECT_THROW_MSG(format(format_str), FormatError, "unmatched '{' in format");
  SPrintf(format_str, "{0:%u}", INT_MAX + 1u);
  EXPECT_THROW_MSG(format(format_str, 0),
      FormatError, "number is too big in format");
  EXPECT_EQ(" -42", format("{0:4}", -42));
  EXPECT_EQ("   42", format("{0:5}", 42u));
  EXPECT_EQ("   -42", format("{0:6}", -42l));
  EXPECT_EQ("     42", format("{0:7}", 42ul));
  EXPECT_EQ("   -42", format("{0:6}", -42ll));
  EXPECT_EQ("     42", format("{0:7}", 42ull));
  EXPECT_EQ("   -1.23", format("{0:8}", -1.23));
  EXPECT_EQ("    -1.23", format("{0:9}", -1.23l));
  EXPECT_EQ("    0xcafe", format("{0:10}", reinterpret_cast<void*>(0xcafe)));
  EXPECT_EQ("x          ", format("{0:11}", 'x'));
  EXPECT_EQ("str         ", format("{0:12}", "str"));
  EXPECT_EQ("test         ", format("{0:13}", TestString("test")));
}

TEST(FormatterTest, Precision) {
  char format_str[BUFFER_SIZE];
  SPrintf(format_str, "{0:.%u", UINT_MAX);
  Increment(format_str + 4);
  EXPECT_THROW_MSG(format(format_str), FormatError, "unmatched '{' in format");
  std::size_t size = std::strlen(format_str);
  format_str[size] = '}';
  format_str[size + 1] = 0;
  EXPECT_THROW_MSG(format(format_str, 0),
      FormatError, "number is too big in format");

  SPrintf(format_str, "{0:.%u", INT_MAX + 1u);
  EXPECT_THROW_MSG(format(format_str), FormatError, "unmatched '{' in format");
  SPrintf(format_str, "{0:.%u}", INT_MAX + 1u);
  EXPECT_THROW_MSG(format(format_str, 0),
      FormatError, "number is too big in format");

  EXPECT_THROW_MSG(format("{0:.", 0),
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(format("{0:.}", 0),
      FormatError, "missing precision in format");
  EXPECT_THROW_MSG(format("{0:.2", 0),
      FormatError, "unmatched '{' in format");

  EXPECT_THROW_MSG(format("{0:.2}", 42),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.2f}", 42),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.2}", 42u),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.2f}", 42u),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.2}", 42l),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.2f}", 42l),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.2}", 42ul),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.2f}", 42ul),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.2}", 42ll),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.2f}", 42ll),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.2}", 42ull),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.2f}", 42ull),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_EQ("1.2", format("{0:.2}", 1.2345));
  EXPECT_EQ("1.2", format("{0:.2}", 1.2345l));

  EXPECT_THROW_MSG(format("{0:.2}", reinterpret_cast<void*>(0xcafe)),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.2f}", reinterpret_cast<void*>(0xcafe)),
      FormatError, "precision specifier requires floating-point argument");

  EXPECT_THROW_MSG(format("{0:.2}", 'x'),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.2f}", 'x'),
      FormatError, "precision specifier requires floating-point argument");

  EXPECT_THROW_MSG(format("{0:.2}", "str"),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.2f}", "str"),
      FormatError, "precision specifier requires floating-point argument");

  EXPECT_THROW_MSG(format("{0:.2}", TestString()),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.2f}", TestString()),
      FormatError, "precision specifier requires floating-point argument");
}

TEST(FormatterTest, RuntimePrecision) {
  char format_str[BUFFER_SIZE];
  SPrintf(format_str, "{0:.{%u", UINT_MAX);
  Increment(format_str + 4);
  EXPECT_THROW_MSG(format(format_str), FormatError, "unmatched '{' in format");
  std::size_t size = std::strlen(format_str);
  format_str[size] = '}';
  format_str[size + 1] = 0;
  EXPECT_THROW_MSG(format(format_str, 0), FormatError, "unmatched '{' in format");
  format_str[size + 1] = '}';
  format_str[size + 2] = 0;
  EXPECT_THROW_MSG(format(format_str, 0),
      FormatError, "number is too big in format");

  EXPECT_THROW_MSG(format("{0:.{", 0),
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(format("{0:.{}", 0),
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(format("{0:.{x}}", 0),
      FormatError, "invalid argument index in format string");
  EXPECT_THROW_MSG(format("{0:.{1}", 0, 0),
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(format("{0:.{1}}", 0),
      FormatError, "argument index is out of range in format");

  EXPECT_THROW_MSG(format("{0:.{1}}", 0, -1),
      FormatError, "negative precision in format");
  EXPECT_THROW_MSG(format("{0:.{1}}", 0, (INT_MAX + 1u)),
      FormatError, "number is too big in format");
  EXPECT_THROW_MSG(format("{0:.{1}}", 0, -1l),
      FormatError, "negative precision in format");
  if (sizeof(long) > sizeof(int)) {
    long value = INT_MAX;
    EXPECT_THROW_MSG(format("{0:.{1}}", 0, (value + 1)),
        FormatError, "number is too big in format");
  }
  EXPECT_THROW_MSG(format("{0:.{1}}", 0, (INT_MAX + 1ul)),
      FormatError, "number is too big in format");

  EXPECT_THROW_MSG(format("{0:.{1}}", 0, '0'),
      FormatError, "precision is not integer");
  EXPECT_THROW_MSG(format("{0:.{1}}", 0, 0.0),
      FormatError, "precision is not integer");

  EXPECT_THROW_MSG(format("{0:.{1}}", 42, 2),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.{1}f}", 42, 2),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.{1}}", 42u, 2),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.{1}f}", 42u, 2),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.{1}}", 42l, 2),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.{1}f}", 42l, 2),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.{1}}", 42ul, 2),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.{1}f}", 42ul, 2),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.{1}}", 42ll, 2),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.{1}f}", 42ll, 2),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.{1}}", 42ull, 2),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.{1}f}", 42ull, 2),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_EQ("1.2", format("{0:.{1}}", 1.2345, 2));
  EXPECT_EQ("1.2", format("{1:.{0}}", 2, 1.2345l));

  EXPECT_THROW_MSG(format("{0:.{1}}", reinterpret_cast<void*>(0xcafe), 2),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.{1}f}", reinterpret_cast<void*>(0xcafe), 2),
      FormatError, "precision specifier requires floating-point argument");

  EXPECT_THROW_MSG(format("{0:.{1}}", 'x', 2),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.{1}f}", 'x', 2),
      FormatError, "precision specifier requires floating-point argument");

  EXPECT_THROW_MSG(format("{0:.{1}}", "str", 2),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.{1}f}", "str", 2),
      FormatError, "precision specifier requires floating-point argument");

  EXPECT_THROW_MSG(format("{0:.{1}}", TestString(), 2),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(format("{0:.{1}f}", TestString(), 2),
      FormatError, "precision specifier requires floating-point argument");
}

template <typename T>
void CheckUnknownTypes(
    const T &value, const char *types, const char *type_name) {
  char format_str[BUFFER_SIZE], message[BUFFER_SIZE];
  const char *special = ".0123456789}";
  for (int i = CHAR_MIN; i <= CHAR_MAX; ++i) {
    char c = i;
    if (std::strchr(types, c) || std::strchr(special, c) || !c) continue;
    SPrintf(format_str, "{0:10%c}", c);
    if (std::isprint(static_cast<unsigned char>(c)))
      SPrintf(message, "unknown format code '%c' for %s", c, type_name);
    else
      SPrintf(message, "unknown format code '\\x%02x' for %s", c, type_name);
    EXPECT_THROW_MSG(format(format_str, value), FormatError, message)
      << format_str << " " << message;
  }
}

TEST(FormatterTest, FormatBool) {
  EXPECT_EQ(L"1", format(L"{}", true));
}

TEST(FormatterTest, FormatShort) {
  short s = 42;
  EXPECT_EQ("42", format("{0:d}", s));
  unsigned short us = 42;
  EXPECT_EQ("42", format("{0:d}", us));
}

TEST(FormatterTest, FormatInt) {
  EXPECT_THROW_MSG(format("{0:v", 42),
      FormatError, "unmatched '{' in format");
  CheckUnknownTypes(42, "bBdoxX", "integer");
}

TEST(FormatterTest, FormatBin) {
  EXPECT_EQ("0", format("{0:b}", 0));
  EXPECT_EQ("101010", format("{0:b}", 42));
  EXPECT_EQ("101010", format("{0:b}", 42u));
  EXPECT_EQ("-101010", format("{0:b}", -42));
  EXPECT_EQ("11000000111001", format("{0:b}", 12345));
  EXPECT_EQ("10010001101000101011001111000", format("{0:b}", 0x12345678));
  EXPECT_EQ("10010000101010111100110111101111", format("{0:b}", 0x90ABCDEF));
  EXPECT_EQ("11111111111111111111111111111111",
            format("{0:b}", std::numeric_limits<uint32_t>::max()));
}

TEST(FormatterTest, FormatDec) {
  EXPECT_EQ("0", format("{0}", 0));
  EXPECT_EQ("42", format("{0}", 42));
  EXPECT_EQ("42", format("{0:d}", 42));
  EXPECT_EQ("42", format("{0}", 42u));
  EXPECT_EQ("-42", format("{0}", -42));
  EXPECT_EQ("12345", format("{0}", 12345));
  EXPECT_EQ("67890", format("{0}", 67890));
  char buffer[BUFFER_SIZE];
  SPrintf(buffer, "%d", INT_MIN);
  EXPECT_EQ(buffer, format("{0}", INT_MIN));
  SPrintf(buffer, "%d", INT_MAX);
  EXPECT_EQ(buffer, format("{0}", INT_MAX));
  SPrintf(buffer, "%u", UINT_MAX);
  EXPECT_EQ(buffer, format("{0}", UINT_MAX));
  SPrintf(buffer, "%ld", 0 - static_cast<unsigned long>(LONG_MIN));
  EXPECT_EQ(buffer, format("{0}", LONG_MIN));
  SPrintf(buffer, "%ld", LONG_MAX);
  EXPECT_EQ(buffer, format("{0}", LONG_MAX));
  SPrintf(buffer, "%lu", ULONG_MAX);
  EXPECT_EQ(buffer, format("{0}", ULONG_MAX));
}

TEST(FormatterTest, FormatHex) {
  EXPECT_EQ("0", format("{0:x}", 0));
  EXPECT_EQ("42", format("{0:x}", 0x42));
  EXPECT_EQ("42", format("{0:x}", 0x42u));
  EXPECT_EQ("-42", format("{0:x}", -0x42));
  EXPECT_EQ("12345678", format("{0:x}", 0x12345678));
  EXPECT_EQ("90abcdef", format("{0:x}", 0x90abcdef));
  EXPECT_EQ("12345678", format("{0:X}", 0x12345678));
  EXPECT_EQ("90ABCDEF", format("{0:X}", 0x90ABCDEF));

  char buffer[BUFFER_SIZE];
  SPrintf(buffer, "-%x", 0 - static_cast<unsigned>(INT_MIN));
  EXPECT_EQ(buffer, format("{0:x}", INT_MIN));
  SPrintf(buffer, "%x", INT_MAX);
  EXPECT_EQ(buffer, format("{0:x}", INT_MAX));
  SPrintf(buffer, "%x", UINT_MAX);
  EXPECT_EQ(buffer, format("{0:x}", UINT_MAX));
  SPrintf(buffer, "-%lx", 0 - static_cast<unsigned long>(LONG_MIN));
  EXPECT_EQ(buffer, format("{0:x}", LONG_MIN));
  SPrintf(buffer, "%lx", LONG_MAX);
  EXPECT_EQ(buffer, format("{0:x}", LONG_MAX));
  SPrintf(buffer, "%lx", ULONG_MAX);
  EXPECT_EQ(buffer, format("{0:x}", ULONG_MAX));
}

TEST(FormatterTest, FormatOct) {
  EXPECT_EQ("0", format("{0:o}", 0));
  EXPECT_EQ("42", format("{0:o}", 042));
  EXPECT_EQ("42", format("{0:o}", 042u));
  EXPECT_EQ("-42", format("{0:o}", -042));
  EXPECT_EQ("12345670", format("{0:o}", 012345670));
  char buffer[BUFFER_SIZE];
  SPrintf(buffer, "-%o", 0 - static_cast<unsigned>(INT_MIN));
  EXPECT_EQ(buffer, format("{0:o}", INT_MIN));
  SPrintf(buffer, "%o", INT_MAX);
  EXPECT_EQ(buffer, format("{0:o}", INT_MAX));
  SPrintf(buffer, "%o", UINT_MAX);
  EXPECT_EQ(buffer, format("{0:o}", UINT_MAX));
  SPrintf(buffer, "-%lo", 0 - static_cast<unsigned long>(LONG_MIN));
  EXPECT_EQ(buffer, format("{0:o}", LONG_MIN));
  SPrintf(buffer, "%lo", LONG_MAX);
  EXPECT_EQ(buffer, format("{0:o}", LONG_MAX));
  SPrintf(buffer, "%lo", ULONG_MAX);
  EXPECT_EQ(buffer, format("{0:o}", ULONG_MAX));
}

TEST(FormatterTest, FormatFloat) {
  EXPECT_EQ("392.500000", format("{0:f}", 392.5f));
}

TEST(FormatterTest, FormatDouble) {
  CheckUnknownTypes(1.2, "eEfFgGaA", "double");
  EXPECT_EQ("0", format("{0:}", 0.0));
  EXPECT_EQ("0.000000", format("{0:f}", 0.0));
  EXPECT_EQ("392.65", format("{0:}", 392.65));
  EXPECT_EQ("392.65", format("{0:g}", 392.65));
  EXPECT_EQ("392.65", format("{0:G}", 392.65));
  EXPECT_EQ("392.650000", format("{0:f}", 392.65));
  EXPECT_EQ("392.650000", format("{0:F}", 392.65));
  char buffer[BUFFER_SIZE];
  SPrintf(buffer, "%e", 392.65);
  EXPECT_EQ(buffer, format("{0:e}", 392.65));
  SPrintf(buffer, "%E", 392.65);
  EXPECT_EQ(buffer, format("{0:E}", 392.65));
  EXPECT_EQ("+0000392.6", format("{0:+010.4g}", 392.65));
  SPrintf(buffer, "%a", -42.0);
  EXPECT_EQ(buffer, format("{:a}", -42.0));
  SPrintf(buffer, "%A", -42.0);
  EXPECT_EQ(buffer, format("{:A}", -42.0));
}

TEST(FormatterTest, FormatNaN) {
  double nan = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ("nan", format("{}", nan));
  EXPECT_EQ("+nan", format("{:+}", nan));
  if (fmt::internal::SignBitNoInline(-nan))
    EXPECT_EQ("-nan", format("{}", -nan));
  else
    fmt::Print("Warning: compiler doesn't handle negative NaN correctly");
  EXPECT_EQ(" nan", format("{: }", nan));
  EXPECT_EQ("NAN", format("{:F}", nan));
  EXPECT_EQ("nan    ", format("{:<7}", nan));
  EXPECT_EQ("  nan  ", format("{:^7}", nan));
  EXPECT_EQ("    nan", format("{:>7}", nan));
}

TEST(FormatterTest, FormatInfinity) {
  double inf = std::numeric_limits<double>::infinity();
  EXPECT_EQ("inf", format("{}", inf));
  EXPECT_EQ("+inf", format("{:+}", inf));
  EXPECT_EQ("-inf", format("{}", -inf));
  EXPECT_EQ(" inf", format("{: }", inf));
  EXPECT_EQ("INF", format("{:F}", inf));
  EXPECT_EQ("inf    ", format("{:<7}", inf));
  EXPECT_EQ("  inf  ", format("{:^7}", inf));
  EXPECT_EQ("    inf", format("{:>7}", inf));
}

TEST(FormatterTest, FormatLongDouble) {
  EXPECT_EQ("0", format("{0:}", 0.0l));
  EXPECT_EQ("0.000000", format("{0:f}", 0.0l));
  EXPECT_EQ("392.65", format("{0:}", 392.65l));
  EXPECT_EQ("392.65", format("{0:g}", 392.65l));
  EXPECT_EQ("392.65", format("{0:G}", 392.65l));
  EXPECT_EQ("392.650000", format("{0:f}", 392.65l));
  EXPECT_EQ("392.650000", format("{0:F}", 392.65l));
  char buffer[BUFFER_SIZE];
  SPrintf(buffer, "%Le", 392.65l);
  EXPECT_EQ(buffer, format("{0:e}", 392.65l));
  SPrintf(buffer, "%LE", 392.65l);
  EXPECT_EQ("+0000392.6", format("{0:+010.4g}", 392.65l));
}

TEST(FormatterTest, FormatChar) {
  CheckUnknownTypes('a', "c", "char");
  EXPECT_EQ("a", format("{0}", 'a'));
  EXPECT_EQ("z", format("{0:c}", 'z'));
  EXPECT_EQ(L"a", format(L"{0}", 'a'));
}

TEST(FormatterTest, FormatWChar) {
  EXPECT_EQ(L"a", format(L"{0}", L'a'));
  // This shouldn't compile:
  //format("{}", L'a');
}

TEST(FormatterTest, FormatCString) {
  CheckUnknownTypes("test", "s", "string");
  EXPECT_EQ("test", format("{0}", "test"));
  EXPECT_EQ("test", format("{0:s}", "test"));
  char nonconst[] = "nonconst";
  EXPECT_EQ("nonconst", format("{0}", nonconst));
  EXPECT_THROW_MSG(format("{0}", reinterpret_cast<const char*>(0)),
      FormatError, "string pointer is null");
}

TEST(FormatterTest, FormatPointer) {
  CheckUnknownTypes(reinterpret_cast<void*>(0x1234), "p", "pointer");
  EXPECT_EQ("0x0", format("{0}", reinterpret_cast<void*>(0)));
  EXPECT_EQ("0x1234", format("{0}", reinterpret_cast<void*>(0x1234)));
  EXPECT_EQ("0x1234", format("{0:p}", reinterpret_cast<void*>(0x1234)));
  EXPECT_EQ("0x" + std::string(sizeof(void*) * CHAR_BIT / 4, 'f'),
      format("{0}", reinterpret_cast<void*>(~uintptr_t())));
}

TEST(FormatterTest, FormatString) {
  EXPECT_EQ("test", format("{0}", std::string("test")));
}

TEST(FormatterTest, FormatStringRef) {
  EXPECT_EQ("test", format("{0}", StringRef("test")));
}

TEST(FormatterTest, FormatUsingIOStreams) {
  EXPECT_EQ("a string", format("{0}", TestString("a string")));
  std::string s = format("The date is {0}", Date(2012, 12, 9));
  EXPECT_EQ("The date is 2012-12-9", s);
  Date date(2012, 12, 9);
  CheckUnknownTypes(date, "", "object");
}

class Answer {};

template <typename Char>
void format(BasicWriter<Char> &w, const fmt::FormatSpec &spec, Answer) {
  w.write_str("42", spec);
}

TEST(FormatterTest, CustomFormat) {
  EXPECT_EQ("42", format("{0}", Answer()));
}

TEST(FormatterTest, WideFormatString) {
  EXPECT_EQ(L"42", format(L"{}", 42));
  EXPECT_EQ(L"4.2", format(L"{}", 4.2));
  EXPECT_EQ(L"abc", format(L"{}", L"abc"));
  EXPECT_EQ(L"z", format(L"{}", L'z'));
}

TEST(FormatterTest, FormatStringFromSpeedTest) {
  EXPECT_EQ("1.2340000000:0042:+3.13:str:0x3e8:X:%",
      format("{0:0.10f}:{1:04}:{2:+g}:{3}:{4}:{5}:%",
          1.234, 42, 3.13, "str", reinterpret_cast<void*>(1000), 'X'));
}

TEST(FormatterTest, FormatExamples) {
  using fmt::hex;
  EXPECT_EQ("0000cafe", (Writer() << pad(hex(0xcafe), 8, '0')).str());

  std::string message = format("The answer is {}", 42);
  EXPECT_EQ("The answer is 42", message);

  EXPECT_EQ("42", format("{}", 42));
  EXPECT_EQ("42", format(std::string("{}"), 42));

  Writer out;
  out << "The answer is " << 42 << "\n";
  out.write("({:+f}, {:+f})", -3.14, 3.14);
  EXPECT_EQ("The answer is 42\n(-3.140000, +3.140000)", out.str());

  {
    fmt::Writer writer;
    for (int i = 0; i < 10; i++)
      writer.write("{}", i);
    std::string s = writer.str(); // s == 0123456789
    EXPECT_EQ("0123456789", s);
  }

  const char *filename = "nonexistent";
  FILE *ftest = fopen(filename, "r");
  int error_code = errno;
  EXPECT_TRUE(ftest == 0);
  EXPECT_SYSTEM_ERROR({
    FILE *f = fopen(filename, "r");
    if (!f)
      throw fmt::SystemError(errno, "Cannot open file '{}'", filename);
  }, error_code, "Cannot open file 'nonexistent'");
}

TEST(StringRefTest, Ctor) {
  EXPECT_STREQ("abc", StringRef("abc").c_str());
  EXPECT_EQ(3u, StringRef("abc").size());

  EXPECT_STREQ("defg", StringRef(std::string("defg")).c_str());
  EXPECT_EQ(4u, StringRef(std::string("defg")).size());
}

TEST(StringRefTest, ConvertToString) {
  std::string s = StringRef("abc");
  EXPECT_EQ("abc", s);
}

TEST(FormatterTest, Ctor) {
  fmt::Formatter<> f1("test");
  fmt::Formatter<> f1copy(f1);
  fmt::Formatter<> f2("test", fmt::NullSink());
  fmt::Formatter<fmt::NullSink> f3("test");
  fmt::Formatter<fmt::NullSink, wchar_t> f4(L"test");
  fmt::Formatter<fmt::NullSink, wchar_t> f4copy(f4);
  fmt::Formatter<fmt::NullSink, wchar_t> f5(L"test", fmt::NullSink());
}

// A sink that counts the number of times the output is written to it.
struct CountingSink {
  int &num_writes;

  explicit CountingSink(int &num_writes) : num_writes(num_writes) {}

  void operator()(const Writer &) const {
    ++num_writes;
  }
};

TEST(FormatterTest, Sink) {
  int num_writes = 0;
  {
    fmt::Formatter<CountingSink> f("test", CountingSink(num_writes));
    EXPECT_EQ(0, num_writes);
  }
  EXPECT_EQ(1, num_writes);
}

TEST(FormatterTest, Move) {
  // Test if formatting is performed once if we "move" a formatter.
  int num_writes = 0;
  {
    typedef fmt::Formatter<CountingSink> TestFormatter;
    TestFormatter *f = new TestFormatter("test", CountingSink(num_writes));
    TestFormatter f2(*f);
    delete f;
    EXPECT_EQ(0, num_writes);
  }
  EXPECT_EQ(1, num_writes);
}

TEST(FormatterTest, OutputNotWrittenOnError) {
  int num_writes = 0;
  {
    typedef fmt::Formatter<CountingSink> TestFormatter;
    EXPECT_THROW(TestFormatter f("{0", CountingSink(num_writes)), FormatError);
  }
  EXPECT_EQ(0, num_writes);
}

#if FMT_USE_FILE_DESCRIPTORS

using fmt::BufferedFile;
using fmt::File;

TEST(FormatterTest, FileSink) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  BufferedFile f = write_end.fdopen("w");
  EXPECT_WRITE(f.get(), {
    fmt::FileSink fs(f.get());
    fs(Writer() << "test");
  }, "test");
}

TEST(FormatterTest, FileSinkWriteError) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  BufferedFile f = read_end.fdopen("r");
  fmt::FileSink fs(f.get());
  std::size_t result = std::fwrite(" ", 1, 1, f.get());
  int error_code = errno;
  EXPECT_EQ(0u, result);
  EXPECT_SYSTEM_ERROR(
      fs(Writer() << "test"), error_code, "cannot write to file");
}

#else
# pragma message "warning: some tests are disabled"
#endif

struct PrintError {
  void operator()(const fmt::Writer &w) const {
    std::cerr << "Error: " << w.str() << std::endl;
  }
};

fmt::Formatter<PrintError> ReportError(const char *format) {
  fmt::Formatter<PrintError> f(format);
  return f;
}

TEST(FormatterTest, Examples) {
  EXPECT_EQ("First, thou shalt count to three",
      format("First, thou shalt count to {0}", "three"));
  EXPECT_EQ("Bring me a shrubbery",
      format("Bring me a {}", "shrubbery"));
  EXPECT_EQ("From 1 to 3", format("From {} to {}", 1, 3));

  char buffer[BUFFER_SIZE];
  SPrintf(buffer, "%03.2f", -1.2);
  EXPECT_EQ(buffer, format("{:03.2f}", -1.2));

  EXPECT_EQ("a, b, c", format("{0}, {1}, {2}", 'a', 'b', 'c'));
  EXPECT_EQ("a, b, c", format("{}, {}, {}", 'a', 'b', 'c'));
  EXPECT_EQ("c, b, a", format("{2}, {1}, {0}", 'a', 'b', 'c'));
  EXPECT_EQ("abracadabra", format("{0}{1}{0}", "abra", "cad"));

  EXPECT_EQ("left aligned                  ",
      format("{:<30}", "left aligned"));
  EXPECT_EQ("                 right aligned",
      format("{:>30}", "right aligned"));
  EXPECT_EQ("           centered           ",
      format("{:^30}", "centered"));
  EXPECT_EQ("***********centered***********",
      format("{:*^30}", "centered"));

  EXPECT_EQ("+3.140000; -3.140000",
      format("{:+f}; {:+f}", 3.14, -3.14));
  EXPECT_EQ(" 3.140000; -3.140000",
      format("{: f}; {: f}", 3.14, -3.14));
  EXPECT_EQ("3.140000; -3.140000",
      format("{:-f}; {:-f}", 3.14, -3.14));

  EXPECT_EQ("int: 42;  hex: 2a;  oct: 52",
      format("int: {0:d};  hex: {0:x};  oct: {0:o}", 42));
  EXPECT_EQ("int: 42;  hex: 0x2a;  oct: 052",
      format("int: {0:d};  hex: {0:#x};  oct: {0:#o}", 42));

  std::string path = "somefile";
  ReportError("File not found: {0}") << path;

  EXPECT_EQ("The answer is 42", format("The answer is {}", 42));
  EXPECT_THROW_MSG(
    format("The answer is {:d}", "forty-two"), FormatError,
    "unknown format code 'd' for string");

  EXPECT_EQ(L"Cyrillic letter \x42e",
    format(L"Cyrillic letter {}", L'\x42e'));

  EXPECT_WRITE(stdout,
      fmt::print("{}", std::numeric_limits<double>::infinity()), "inf");
}

TEST(FormatIntTest, Data) {
  fmt::FormatInt format_int(42);
  EXPECT_EQ("42", std::string(format_int.data(), format_int.size()));
}

TEST(FormatIntTest, FormatInt) {
  EXPECT_EQ("42", fmt::FormatInt(42).str());
  EXPECT_EQ(2u, fmt::FormatInt(42).size());
  EXPECT_EQ("-42", fmt::FormatInt(-42).str());
  EXPECT_EQ(3u, fmt::FormatInt(-42).size());
  EXPECT_EQ("42", fmt::FormatInt(42ul).str());
  EXPECT_EQ("-42", fmt::FormatInt(-42l).str());
  EXPECT_EQ("42", fmt::FormatInt(42ull).str());
  EXPECT_EQ("-42", fmt::FormatInt(-42ll).str());
  std::ostringstream os;
  os << std::numeric_limits<int64_t>::max();
  EXPECT_EQ(os.str(), fmt::FormatInt(std::numeric_limits<int64_t>::max()).str());
}

template <typename T>
std::string FormatDec(T value) {
  char buffer[10];
  char *ptr = buffer;
  fmt::FormatDec(ptr, value);
  return std::string(buffer, ptr);
}

TEST(FormatIntTest, FormatDec) {
  EXPECT_EQ("-42", FormatDec(static_cast<char>(-42)));
  EXPECT_EQ("-42", FormatDec(static_cast<short>(-42)));
  std::ostringstream os;
  os << std::numeric_limits<unsigned short>::max();
  EXPECT_EQ(os.str(), FormatDec(std::numeric_limits<unsigned short>::max()));
  EXPECT_EQ("1", FormatDec(1));
  EXPECT_EQ("-1", FormatDec(-1));
  EXPECT_EQ("42", FormatDec(42));
  EXPECT_EQ("-42", FormatDec(-42));
  EXPECT_EQ("42", FormatDec(42l));
  EXPECT_EQ("42", FormatDec(42ul));
  EXPECT_EQ("42", FormatDec(42ll));
  EXPECT_EQ("42", FormatDec(42ull));
}

#if FMT_USE_FILE_DESCRIPTORS

TEST(FormatTest, Print) {
  EXPECT_WRITE(stdout, fmt::print("Don't {}!", "panic"), "Don't panic!");
  EXPECT_WRITE(stderr,
      fmt::print(stderr, "Don't {}!", "panic"), "Don't panic!");
}

TEST(FormatTest, PrintColored) {
  EXPECT_WRITE(stdout, fmt::PrintColored(fmt::RED, "Hello, {}!\n") << "world",
      "\x1b[31mHello, world!\n\x1b[0m");
}

#endif

TEST(FormatTest, Variadic) {
  EXPECT_EQ("abc1", format("{}c{}", "ab", 1));
  EXPECT_EQ(L"abc1", format(L"{}c{}", L"ab", 1));
}

template <typename T>
std::string str(const T &value) {
  return fmt::format("{}", value);
}

TEST(StrTest, Convert) {
  EXPECT_EQ("42", str(42));
  std::string s = str(Date(2012, 12, 9));
  EXPECT_EQ("2012-12-9", s);
}

std::string FormatMessage(int id, const char *format,
    const fmt::ArgList &args) {
  fmt::Writer w;
  w.write("[{}] ", id);
  w.write(format, args);
  return w.str();
}

FMT_VARIADIC(std::string, FormatMessage, int, const char *)

TEST(FormatTest, FormatMessageExample) {
  EXPECT_EQ("[42] something happened",
      FormatMessage(42, "{} happened", "something"));
}
