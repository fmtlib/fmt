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

TEST(FormatterTest, FormatNoArgs) {
  EXPECT_EQ("abracadabra", str(Format("{0}{1}{0}") << "abra" << "cad"));
  EXPECT_EQ("test", str(Format("test")));
}

TEST(FormatterTest, FormatArgs) {
  EXPECT_EQ("42", str(Format("{0}") << 42));
  EXPECT_EQ("before 42", str(Format("before {0}") << 42));
  EXPECT_EQ("42 after", str(Format("{0} after") << 42));
  EXPECT_EQ("before 42 after", str(Format("before {0} after") << 42));
  EXPECT_EQ("answer = 42", str(Format("{0} = {1}") << "answer" << 42));
  EXPECT_EQ("42 is the answer", str(Format("{1} is the {0}") << "answer" << 42));
  EXPECT_EQ("abracadabra", str(Format("{0}{1}{0}") << "abra" << "cad"));
}

TEST(FormatterTest, FormatArgErrors) {
  EXPECT_THROW_MSG(Format("{"), FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(Format("{}"), FormatError,
      "missing argument index in format string");
  EXPECT_THROW_MSG(Format("{0"), FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(Format("{0}"), std::out_of_range,
      "argument index is out of range in format");
  char format[256];
  std::sprintf(format, "{%u", UINT_MAX);
  EXPECT_THROW_MSG(Format(format), FormatError, "unmatched '{' in format");
  std::sprintf(format, "{%u}", UINT_MAX);
  EXPECT_THROW_MSG(Format(format), std::out_of_range,
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

TEST(FormatterTest, FormatPlusFlag) {
  EXPECT_EQ("+42", str(Format("{0:+}") << 42));
  EXPECT_EQ("-42", str(Format("{0:+}") << -42));
  EXPECT_THROW_MSG(Format("{0:+") << 'c',
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(Format("{0:+}") << 'c',
      FormatError, "format specifier '+' used with non-numeric type");
  EXPECT_EQ("+42", str(Format("{0:+}") << 42));
  EXPECT_EQ("+42", str(Format("{0:+}") << 42u));
  EXPECT_EQ("+42", str(Format("{0:+}") << 42l));
  EXPECT_EQ("+42", str(Format("{0:+}") << 42ul));
  EXPECT_EQ("+42", str(Format("{0:+}") << 42.0));
  EXPECT_EQ("+42", str(Format("{0:+}") << 42.0l));
  EXPECT_THROW_MSG(Format("{0:+}") << "abc",
      FormatError, "format specifier '+' used with non-numeric type");
  EXPECT_THROW_MSG(Format("{0:+}") << L"abc",
      FormatError, "format specifier '+' used with non-numeric type");
  EXPECT_THROW_MSG(Format("{0:+}") << static_cast<const void*>("abc"),
      FormatError, "format specifier '+' used with non-numeric type");
  EXPECT_THROW_MSG(Format("{0:+}") << TestString(),
      FormatError, "format specifier '+' used with non-numeric type");
}

TEST(FormatterTest, FormatZeroFlag) {
  EXPECT_EQ("42", str(Format("{0:0}") << 42));
  EXPECT_THROW_MSG(Format("{0:0") << 'c',
      FormatError, "unmatched '{' in format");
  EXPECT_THROW_MSG(Format("{0:05}") << 'c',
      FormatError, "format specifier '0' used with non-numeric type");
  EXPECT_EQ("-0042", str(Format("{0:05}") << -42));
  EXPECT_EQ("00042", str(Format("{0:05}") << 42u));
  EXPECT_EQ("-0042", str(Format("{0:05}") << -42l));
  EXPECT_EQ("00042", str(Format("{0:05}") << 42ul));
  EXPECT_EQ("-0042", str(Format("{0:05}") << -42.0));
  EXPECT_EQ("-0042", str(Format("{0:05}") << -42.0l));
  EXPECT_THROW_MSG(Format("{0:05}") << "abc",
      FormatError, "format specifier '0' used with non-numeric type");
  EXPECT_THROW_MSG(Format("{0:05}") << L"abc",
      FormatError, "format specifier '0' used with non-numeric type");
  EXPECT_THROW_MSG(Format("{0:05}") << static_cast<const void*>("abc"),
      FormatError, "format specifier '0' used with non-numeric type");
  EXPECT_THROW_MSG(Format("{0:05}") << TestString(),
      FormatError, "format specifier '0' used with non-numeric type");
}

TEST(FormatterTest, FormatWidth) {
  // TODO
}

TEST(FormatterTest, FormatChar) {
  EXPECT_EQ("a*b", str(Format("{0}{1}{2}") << 'a' << '*' << 'b'));
}

TEST(FormatterTest, FormatInt) {
  EXPECT_EQ("42", str(Format("{0}") << 42));
  EXPECT_EQ("-1234", str(Format("{0}") << -1234));
  std::ostringstream os;
  os << INT_MIN;
  EXPECT_EQ(os.str(), str(Format("{0}") << INT_MIN));
  os.str(std::string());
  os << INT_MAX;
  EXPECT_EQ(os.str(), str(Format("{0}") << INT_MAX));
}

TEST(FormatterTest, FormatString) {
  EXPECT_EQ("test", str(Format("{0}") << std::string("test")));
}

TEST(FormatterTest, FormatCustomArg) {
  EXPECT_EQ("a string", str(Format("{0}") << TestString("a string")));
}

TEST(FormatterTest, FormatStringFromSpeedTest) {
  EXPECT_EQ("1.2340000000:0042:+3.13:str:0x3e8:X:%",
      str(Format("{0:0.10f}:{1:04}:{2:+g}:{3}:{4}:{5}:%")
          << 1.234 << 42 << 3.13 << "str"
          << reinterpret_cast<void*>(1000) << 'X'));
}

// TODO: more tests
