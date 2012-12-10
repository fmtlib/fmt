/*
 Formatting library tests.
 Author: Victor Zverovich
 */

#include <cfloat>
#include <climits>
#include <cstring>
#include <gtest/gtest.h>
#include "format.h"

using std::size_t;
using std::sprintf;

using fmt::Formatter;
using fmt::Format;
using fmt::FormatError;

#define FORMAT_TEST_THROW_(statement, expected_exception, message, fail) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_ \
  if (::testing::internal::ConstCharPtr gtest_msg = "") { \
    bool gtest_caught_expected = false; \
    std::string actual_message; \
    try { \
      GTEST_SUPPRESS_UNREACHABLE_CODE_WARNING_BELOW_(statement); \
    } \
    catch (expected_exception const& e) { \
      gtest_caught_expected = true; \
      actual_message = e.what(); \
    } \
    catch (...) { \
      gtest_msg.value = \
          "Expected: " #statement " throws an exception of type " \
          #expected_exception ".\n  Actual: it throws a different type."; \
      goto GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__); \
    } \
    if (!gtest_caught_expected) { \
      gtest_msg.value = \
          "Expected: " #statement " throws an exception of type " \
          #expected_exception ".\n  Actual: it throws nothing."; \
      goto GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__); \
    } else if (actual_message != message) {\
      gtest_msg.value = \
          "Expected: " #statement " throws an exception of type " \
          #expected_exception " with message \"" message "\"."; \
      goto GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__); \
    } \
  } else \
    GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__): \
      fail(gtest_msg.value)

#define EXPECT_THROW_MSG(statement, expected_exception, expected_message) \
  FORMAT_TEST_THROW_(statement, expected_exception, expected_message, \
      GTEST_NONFATAL_FAILURE_)

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

TEST(FormatterTest, NoArgs) {
  EXPECT_EQ("abracadabra", str(Format("{0}{1}{0}") << "abra" << "cad"));
  EXPECT_EQ("test", str(Format("test")));
}

TEST(FormatterTest, ArgsInDifferentPositions) {
  EXPECT_EQ("42", str(Format("{0}") << 42));
  EXPECT_EQ("before 42", str(Format("before {0}") << 42));
  EXPECT_EQ("42 after", str(Format("{0} after") << 42));
  EXPECT_EQ("before 42 after", str(Format("before {0} after") << 42));
  EXPECT_EQ("answer = 42", str(Format("{0} = {1}") << "answer" << 42));
  EXPECT_EQ("42 is the answer", str(Format("{1} is the {0}") << "answer" << 42));
  EXPECT_EQ("abracadabra", str(Format("{0}{1}{0}") << "abra" << "cad"));
}

TEST(FormatterTest, ArgErrors) {
  EXPECT_THROW_MSG(Format("{"), FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(Format("{}"), FormatError,
      "missing argument index in format string");
  EXPECT_THROW_MSG(Format("{0"), FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(Format("{0}"), FormatError,
      "argument index is out of range in format");
  char format[256];
  std::sprintf(format, "{%u", UINT_MAX);
  EXPECT_THROW_MSG(Format(format), FormatError, "unmatched '{' in format");
  std::sprintf(format, "{%u}", UINT_MAX);
  EXPECT_THROW_MSG(Format(format), FormatError,
      "argument index is out of range in format");
  if (ULONG_MAX > UINT_MAX) {
    std::sprintf(format, "{%lu", UINT_MAX + 1l);
    EXPECT_THROW_MSG(Format(format), FormatError, "unmatched '{' in format");
    std::sprintf(format, "{%lu}", UINT_MAX + 1l);
    EXPECT_THROW_MSG(Format(format),
        FormatError, "number is too big in format");
  } else {
    std::sprintf(format, "{%u0", UINT_MAX);
    EXPECT_THROW_MSG(Format(format), FormatError, "unmatched '{' in format");
    std::sprintf(format, "{%u0}", UINT_MAX);
    EXPECT_THROW_MSG(Format(format),
        FormatError, "number is too big in format");
  }
}

TEST(FormatterTest, EmptySpecs) {
  EXPECT_EQ("42", str(Format("{0:}") << 42));
}

TEST(FormatterTest, PlusFlag) {
  EXPECT_EQ("+42", str(Format("{0:+}") << 42));
  EXPECT_EQ("-42", str(Format("{0:+}") << -42));
  EXPECT_EQ("+42", str(Format("{0:+}") << 42));
  EXPECT_THROW_MSG(Format("{0:+}") << 42u,
      FormatError, "format specifier '+' requires signed argument");
  EXPECT_EQ("+42", str(Format("{0:+}") << 42l));
  EXPECT_THROW_MSG(Format("{0:+}") << 42ul,
      FormatError, "format specifier '+' requires signed argument");
  EXPECT_EQ("+42", str(Format("{0:+}") << 42.0));
  EXPECT_EQ("+42", str(Format("{0:+}") << 42.0l));
  EXPECT_THROW_MSG(Format("{0:+") << 'c',
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(Format("{0:+}") << 'c',
      FormatError, "format specifier '+' requires numeric argument");
  EXPECT_THROW_MSG(Format("{0:+}") << "abc",
      FormatError, "format specifier '+' requires numeric argument");
  EXPECT_THROW_MSG(Format("{0:+}") << reinterpret_cast<void*>(0x42),
      FormatError, "format specifier '+' requires numeric argument");
  EXPECT_THROW_MSG(Format("{0:+}") << TestString(),
      FormatError, "format specifier '+' requires numeric argument");
}

TEST(FormatterTest, ZeroFlag) {
  EXPECT_EQ("42", str(Format("{0:0}") << 42));
  EXPECT_EQ("-0042", str(Format("{0:05}") << -42));
  EXPECT_EQ("00042", str(Format("{0:05}") << 42u));
  EXPECT_EQ("-0042", str(Format("{0:05}") << -42l));
  EXPECT_EQ("00042", str(Format("{0:05}") << 42ul));
  EXPECT_EQ("-0042", str(Format("{0:05}") << -42.0));
  EXPECT_EQ("-0042", str(Format("{0:05}") << -42.0l));
  EXPECT_THROW_MSG(Format("{0:0") << 'c',
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(Format("{0:05}") << 'c',
      FormatError, "format specifier '0' requires numeric argument");
  EXPECT_THROW_MSG(Format("{0:05}") << "abc",
      FormatError, "format specifier '0' requires numeric argument");
  EXPECT_THROW_MSG(Format("{0:05}") << reinterpret_cast<void*>(0x42),
      FormatError, "format specifier '0' requires numeric argument");
  EXPECT_THROW_MSG(Format("{0:05}") << TestString(),
      FormatError, "format specifier '0' requires numeric argument");
}

TEST(FormatterTest, Width) {
  char format[256];
  if (ULONG_MAX > UINT_MAX) {
    std::sprintf(format, "{0:%lu", INT_MAX + 1l);
    EXPECT_THROW_MSG(Format(format), FormatError, "unmatched '{' in format");
    std::sprintf(format, "{0:%lu}", UINT_MAX + 1l);
    EXPECT_THROW_MSG(Format(format) << 0,
        FormatError, "number is too big in format");
  } else {
    std::sprintf(format, "{0:%u0", UINT_MAX);
    EXPECT_THROW_MSG(Format(format), FormatError, "unmatched '{' in format");
    std::sprintf(format, "{0:%u0}", UINT_MAX);
    EXPECT_THROW_MSG(Format(format) << 0,
        FormatError, "number is too big in format");
  }
  std::sprintf(format, "{0:%u", INT_MAX + 1u);
  EXPECT_THROW_MSG(Format(format), FormatError, "unmatched '{' in format");
  std::sprintf(format, "{0:%u}", INT_MAX + 1u);
  EXPECT_THROW_MSG(Format(format) << 0,
      FormatError, "number is too big in format");
  EXPECT_EQ(" -42", str(Format("{0:4}") << -42));
  EXPECT_EQ("   42", str(Format("{0:5}") << 42u));
  EXPECT_EQ("   -42", str(Format("{0:6}") << -42l));
  EXPECT_EQ("     42", str(Format("{0:7}") << 42ul));
  EXPECT_EQ("   -1.23", str(Format("{0:8}") << -1.23));
  EXPECT_EQ("    -1.23", str(Format("{0:9}") << -1.23l));
  EXPECT_EQ("    0xcafe",
      str(Format("{0:10}") << reinterpret_cast<void*>(0xcafe)));
  EXPECT_EQ("x          ", str(Format("{0:11}") << 'x'));
  EXPECT_EQ("str         ", str(Format("{0:12}") << "str"));
  EXPECT_EQ("test         ", str(Format("{0:13}") << TestString("test")));
}

TEST(FormatterTest, Precision) {
  char format[256];
  if (ULONG_MAX > UINT_MAX) {
    std::sprintf(format, "{0:.%lu", INT_MAX + 1l);
    EXPECT_THROW_MSG(Format(format), FormatError, "unmatched '{' in format");
    std::sprintf(format, "{0:.%lu}", UINT_MAX + 1l);
    EXPECT_THROW_MSG(Format(format) << 0,
        FormatError, "number is too big in format");
  } else {
    std::sprintf(format, "{0:.%u0", UINT_MAX);
    EXPECT_THROW_MSG(Format(format), FormatError, "unmatched '{' in format");
    std::sprintf(format, "{0:.%u0}", UINT_MAX);
    EXPECT_THROW_MSG(Format(format) << 0,
        FormatError, "number is too big in format");
  }

  std::sprintf(format, "{0:.%u", INT_MAX + 1u);
  EXPECT_THROW_MSG(Format(format), FormatError, "unmatched '{' in format");
  std::sprintf(format, "{0:.%u}", INT_MAX + 1u);
  EXPECT_THROW_MSG(Format(format) << 0,
      FormatError, "number is too big in format");

  EXPECT_THROW_MSG(Format("{0:.") << 0,
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(Format("{0:.}") << 0,
      FormatError, "missing precision in format");
  EXPECT_THROW_MSG(Format("{0:.2") << 0,
      FormatError, "unmatched '{' in format");

  EXPECT_THROW_MSG(Format("{0:.2}") << 42,
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2f}") << 42,
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2}") << 42u,
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2f}") << 42u,
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2}") << 42l,
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2f}") << 42l,
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2}") << 42ul,
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2f}") << 42ul,
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_EQ("1.2", str(Format("{0:.2}") << 1.2345));
  EXPECT_EQ("1.2", str(Format("{0:.2}") << 1.2345l));

  EXPECT_THROW_MSG(Format("{0:.2}") << reinterpret_cast<void*>(0xcafe),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2f}") << reinterpret_cast<void*>(0xcafe),
      FormatError, "precision specifier requires floating-point argument");

  EXPECT_THROW_MSG(Format("{0:.2}") << 'x',
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2f}") << 'x',
      FormatError, "precision specifier requires floating-point argument");

  EXPECT_THROW_MSG(Format("{0:.2}") << "str",
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2f}") << "str",
      FormatError, "precision specifier requires floating-point argument");

  EXPECT_THROW_MSG(Format("{0:.2}") << TestString(),
      FormatError, "precision specifier requires floating-point argument");
  EXPECT_THROW_MSG(Format("{0:.2f}") << TestString(),
      FormatError, "precision specifier requires floating-point argument");
}

TEST(FormatterTest, FormatInt) {
  EXPECT_THROW_MSG(Format("{0:v") << 42,
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(Format("{0:v}") << 42,
      FormatError, "unknown format code 'v' for integer");
  EXPECT_THROW_MSG(Format("{0:c}") << 42,
      FormatError, "unknown format code 'c' for integer");
  EXPECT_THROW_MSG(Format("{0:e}") << 42,
      FormatError, "unknown format code 'e' for integer");
  EXPECT_THROW_MSG(Format("{0:f}") << 42,
      FormatError, "unknown format code 'f' for integer");
  EXPECT_THROW_MSG(Format("{0:g}") << 42,
      FormatError, "unknown format code 'g' for integer");
}

TEST(FormatterTest, FormatDec) {
  EXPECT_EQ("0", str(Format("{0}") << 0));
  EXPECT_EQ("42", str(Format("{0}") << 42));
  EXPECT_EQ("42", str(Format("{0:d}") << 42));
  EXPECT_EQ("42", str(Format("{0}") << 42u));
  EXPECT_EQ("-42", str(Format("{0}") << -42));
  EXPECT_EQ("12345", str(Format("{0}") << 12345));
  EXPECT_EQ("67890", str(Format("{0}") << 67890));
  char buffer[256];
  sprintf(buffer, "%d", INT_MIN);
  EXPECT_EQ(buffer, str(Format("{0}") << INT_MIN));
  sprintf(buffer, "%d", INT_MAX);
  EXPECT_EQ(buffer, str(Format("{0}") << INT_MAX));
  sprintf(buffer, "%u", UINT_MAX);
  EXPECT_EQ(buffer, str(Format("{0}") << UINT_MAX));
  sprintf(buffer, "%ld", 0ul - LONG_MIN);
  EXPECT_EQ(buffer, str(Format("{0}") << LONG_MIN));
  sprintf(buffer, "%ld", LONG_MAX);
  EXPECT_EQ(buffer, str(Format("{0}") << LONG_MAX));
  sprintf(buffer, "%lu", ULONG_MAX);
  EXPECT_EQ(buffer, str(Format("{0}") << ULONG_MAX));
}

TEST(FormatterTest, FormatHex) {
  EXPECT_EQ("0", str(Format("{0:x}") << 0));
  EXPECT_EQ("42", str(Format("{0:x}") << 0x42));
  EXPECT_EQ("42", str(Format("{0:x}") << 0x42u));
  EXPECT_EQ("-42", str(Format("{0:x}") << -0x42));
  EXPECT_EQ("12345678", str(Format("{0:x}") << 0x12345678));
  EXPECT_EQ("90abcdef", str(Format("{0:x}") << 0x90abcdef));
  EXPECT_EQ("12345678", str(Format("{0:X}") << 0x12345678));
  EXPECT_EQ("90ABCDEF", str(Format("{0:X}") << 0x90ABCDEF));
  char buffer[256];
  sprintf(buffer, "-%x", 0u - INT_MIN);
  EXPECT_EQ(buffer, str(Format("{0:x}") << INT_MIN));
  sprintf(buffer, "%x", INT_MAX);
  EXPECT_EQ(buffer, str(Format("{0:x}") << INT_MAX));
  sprintf(buffer, "%x", UINT_MAX);
  EXPECT_EQ(buffer, str(Format("{0:x}") << UINT_MAX));
  sprintf(buffer, "-%lx", 0ul - LONG_MIN);
  EXPECT_EQ(buffer, str(Format("{0:x}") << LONG_MIN));
  sprintf(buffer, "%lx", LONG_MAX);
  EXPECT_EQ(buffer, str(Format("{0:x}") << LONG_MAX));
  sprintf(buffer, "%lx", ULONG_MAX);
  EXPECT_EQ(buffer, str(Format("{0:x}") << ULONG_MAX));
}

TEST(FormatterTest, FormatOct) {
  EXPECT_EQ("0", str(Format("{0:o}") << 0));
  EXPECT_EQ("42", str(Format("{0:o}") << 042));
  EXPECT_EQ("42", str(Format("{0:o}") << 042u));
  EXPECT_EQ("-42", str(Format("{0:o}") << -042));
  EXPECT_EQ("12345670", str(Format("{0:o}") << 012345670));
  char buffer[256];
  sprintf(buffer, "-%o", 0u - INT_MIN);
  EXPECT_EQ(buffer, str(Format("{0:o}") << INT_MIN));
  sprintf(buffer, "%o", INT_MAX);
  EXPECT_EQ(buffer, str(Format("{0:o}") << INT_MAX));
  sprintf(buffer, "%o", UINT_MAX);
  EXPECT_EQ(buffer, str(Format("{0:o}") << UINT_MAX));
  sprintf(buffer, "-%lo", 0ul - LONG_MIN);
  EXPECT_EQ(buffer, str(Format("{0:o}") << LONG_MIN));
  sprintf(buffer, "%lo", LONG_MAX);
  EXPECT_EQ(buffer, str(Format("{0:o}") << LONG_MAX));
  sprintf(buffer, "%lo", ULONG_MAX);
  EXPECT_EQ(buffer, str(Format("{0:o}") << ULONG_MAX));
}

TEST(FormatterTest, FormatDouble) {
  EXPECT_THROW_MSG(Format("{0:c}") << 1.2,
      FormatError, "unknown format code 'c' for double");
  EXPECT_THROW_MSG(Format("{0:d}") << 1.2,
      FormatError, "unknown format code 'd' for double");
  EXPECT_THROW_MSG(Format("{0:o}") << 1.2,
      FormatError, "unknown format code 'o' for double");
  EXPECT_THROW_MSG(Format("{0:x}") << 1.2,
      FormatError, "unknown format code 'x' for double");
  EXPECT_EQ("0", str(Format("{0:}") << 0.0));
  EXPECT_EQ("0.000000", str(Format("{0:f}") << 0.0));
  EXPECT_EQ("392.65", str(Format("{0:}") << 392.65));
  EXPECT_EQ("392.65", str(Format("{0:g}") << 392.65));
  EXPECT_EQ("392.65", str(Format("{0:G}") << 392.65));
  EXPECT_EQ("392.650000", str(Format("{0:f}") << 392.65));
  EXPECT_EQ("392.650000", str(Format("{0:F}") << 392.65));
  EXPECT_EQ("3.926500e+02", str(Format("{0:e}") << 392.65));
  EXPECT_EQ("3.926500E+02", str(Format("{0:E}") << 392.65));
  EXPECT_EQ("+0000392.6", str(Format("{0:+010.4g}") << 392.65));
}

TEST(FormatterTest, FormatLongDouble) {
  EXPECT_EQ("0", str(Format("{0:}") << 0.0l));
  EXPECT_EQ("0.000000", str(Format("{0:f}") << 0.0l));
  EXPECT_EQ("392.65", str(Format("{0:}") << 392.65l));
  EXPECT_EQ("392.65", str(Format("{0:g}") << 392.65l));
  EXPECT_EQ("392.65", str(Format("{0:G}") << 392.65l));
  EXPECT_EQ("392.650000", str(Format("{0:f}") << 392.65l));
  EXPECT_EQ("392.650000", str(Format("{0:F}") << 392.65l));
  EXPECT_EQ("3.926500e+02", str(Format("{0:e}") << 392.65l));
  EXPECT_EQ("3.926500E+02", str(Format("{0:E}") << 392.65l));
  EXPECT_EQ("+0000392.6", str(Format("{0:+010.4g}") << 392.65l));
}

TEST(FormatterTest, FormatChar) {
  EXPECT_EQ("a*b", str(Format("{0}{1}{2}") << 'a' << '*' << 'b'));
}

TEST(FormatterTest, FormatString) {
  EXPECT_EQ("test", str(Format("{0}") << std::string("test")));
}

TEST(FormatterTest, FormatPointer) {
  EXPECT_EQ("0x0", str(Format("{0}") << reinterpret_cast<void*>(0)));
}

class Date {
  int year_, month_, day_;
 public:
  Date(int year, int month, int day) : year_(year), month_(month), day_(day) {}

  friend std::ostream &operator<<(std::ostream &os, const Date &d) {
    os << d.year_ << '-' << d.month_ << '-' << d.day_;
    return os;
  }
};

TEST(FormatterTest, FormatCustomArg) {
  EXPECT_EQ("a string", str(Format("{0}") << TestString("a string")));
  std::string s = str(fmt::Format("The date is {0}") << Date(2012, 12, 9));
  EXPECT_EQ("The date is 2012-12-9", s);
}

TEST(FormatterTest, FormatStringFromSpeedTest) {
  EXPECT_EQ("1.2340000000:0042:+3.13:str:0x3e8:X:%",
      str(Format("{0:0.10f}:{1:04}:{2:+g}:{3}:{4}:{5}:%")
          << 1.234 << 42 << 3.13 << "str"
          << reinterpret_cast<void*>(1000) << 'X'));
}

// TODO: more tests
