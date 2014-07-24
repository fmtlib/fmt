/*
 Utility tests.

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

#include <cfloat>
#include <climits>
#include <cstring>
#include <limits>
#include "gtest-extra.h"
#include "util.h"

// Check if format.h compiles with windows.h included.
#ifdef _WIN32
# include <windows.h>
#endif

#include "format.h"

#undef max

using fmt::StringRef;
using fmt::internal::Arg;
using fmt::internal::MakeArg;

namespace {
std::string GetSystemErrorMessage(int error_code) {
#if defined(__MINGW32__) || !defined(_WIN32)
  return strerror(error_code);
#else
  enum { BUFFER_SIZE = 200 };
  char buffer[BUFFER_SIZE];
  EXPECT_EQ(0, strerror_s(buffer, BUFFER_SIZE, error_code));
  std::size_t max_len = BUFFER_SIZE - 1;
  EXPECT_LT(std::strlen(buffer), max_len);
  return buffer;
#endif
}

struct Test {};
template <typename Char>
std::basic_ostream<Char> &operator<<(std::basic_ostream<Char> &os, Test) {
  return os << "test";
}
}

TEST(UtilTest, Increment) {
  char s[10] = "123";
  Increment(s);
  EXPECT_STREQ("124", s);
  s[2] = '8';
  Increment(s);
  EXPECT_STREQ("129", s);
  Increment(s);
  EXPECT_STREQ("130", s);
  s[1] = s[2] = '9';
  Increment(s);
  EXPECT_STREQ("200", s);
}

template <Arg::Type>
struct ArgInfo;

#define ARG_INFO(type_code, Type, field) \
  template <> \
  struct ArgInfo<Arg::type_code> { \
    static Type get(const Arg &arg) { return arg.field; } \
  };

ARG_INFO(INT, int, int_value);
ARG_INFO(UINT, unsigned, uint_value);
ARG_INFO(LONG_LONG, fmt::LongLong, long_long_value);
ARG_INFO(ULONG_LONG, fmt::ULongLong, ulong_long_value);
ARG_INFO(DOUBLE, double, double_value);
ARG_INFO(LONG_DOUBLE, long double, long_double_value);
ARG_INFO(CHAR, int, int_value);
ARG_INFO(STRING, const char *, string.value);
ARG_INFO(WSTRING, const wchar_t *, wstring.value);
ARG_INFO(POINTER, const void *, pointer_value);
ARG_INFO(CUSTOM, Arg::CustomValue, custom);

#define CHECK_ARG_INFO(Type, field, value) { \
  Arg arg = {Arg::Type}; \
  arg.field = value; \
  EXPECT_EQ(value, ArgInfo<Arg::Type>::get(arg)); \
}

TEST(ArgTest, ArgInfo) {
  CHECK_ARG_INFO(INT, int_value, 42);
  CHECK_ARG_INFO(UINT, uint_value, 42);
  CHECK_ARG_INFO(LONG_LONG, long_long_value, 42);
  CHECK_ARG_INFO(ULONG_LONG, ulong_long_value, 42);
  CHECK_ARG_INFO(DOUBLE, double_value, 4.2);
  CHECK_ARG_INFO(LONG_DOUBLE, long_double_value, 4.2);
  CHECK_ARG_INFO(CHAR, int_value, 'x');
  const char STR[] = "abc";
  CHECK_ARG_INFO(STRING, string.value, STR);
  const wchar_t WSTR[] = L"abc";
  CHECK_ARG_INFO(WSTRING, wstring.value, WSTR);
  int p = 0;
  CHECK_ARG_INFO(POINTER, pointer_value, &p);
  Arg arg = {Arg::CUSTOM};
  arg.custom.value = &p;
  EXPECT_EQ(&p, ArgInfo<Arg::CUSTOM>::get(arg).value);
}

#define EXPECT_ARG_(Char, type_code, Type, value) { \
  Type expected_value = static_cast<Type>(value); \
  Arg arg = MakeArg<Char>(expected_value); \
  EXPECT_EQ(Arg::type_code, arg.type); \
  EXPECT_EQ(expected_value, ArgInfo<Arg::type_code>::get(arg)); \
}

#define EXPECT_ARG(type_code, Type, value) \
  EXPECT_ARG_(char, type_code, Type, value)

#define EXPECT_ARGW(type_code, Type, value) \
  EXPECT_ARG_(wchar_t, type_code, Type, value)

TEST(ArgTest, MakeArg) {
  // Test bool.
  EXPECT_ARG(INT, bool, true);

  // Test char.
  EXPECT_ARG(CHAR, signed char, 'a');
  EXPECT_ARG(CHAR, signed char, SCHAR_MIN);
  EXPECT_ARG(CHAR, signed char, SCHAR_MAX);
  EXPECT_ARG(CHAR, unsigned char, 'a');
  EXPECT_ARG(CHAR, unsigned char, UCHAR_MAX );
  EXPECT_ARG(CHAR, char, 'a');
  EXPECT_ARG(CHAR, char, CHAR_MIN);
  EXPECT_ARG(CHAR, char, CHAR_MAX);

  // Test wchar_t.
  EXPECT_ARGW(CHAR, wchar_t, L'a');
  EXPECT_ARGW(CHAR, wchar_t, WCHAR_MIN);
  EXPECT_ARGW(CHAR, wchar_t, WCHAR_MAX);

  // Test short.
  EXPECT_ARG(INT, short, 42);
  EXPECT_ARG(INT, short, SHRT_MIN);
  EXPECT_ARG(INT, short, SHRT_MAX);
  EXPECT_ARG(UINT, unsigned short, 42);
  EXPECT_ARG(UINT, unsigned short, USHRT_MAX);

  // Test int.
  EXPECT_ARG(INT, int, 42);
  EXPECT_ARG(INT, int, INT_MIN);
  EXPECT_ARG(INT, int, INT_MAX);
  EXPECT_ARG(UINT, unsigned, 42);
  EXPECT_ARG(UINT, unsigned, UINT_MAX);

  // Test long.
#if LONG_MAX == INT_MAX
# define LONG INT
# define ULONG UINT
# define long_value int_value
# define ulong_value uint_value
#else
# define LONG LONG_LONG
# define ULONG ULONG_LONG
# define long_value long_long_value
# define ulong_value ulong_long_value
#endif
  EXPECT_ARG(LONG, long, 42);
  EXPECT_ARG(LONG, long, LONG_MIN);
  EXPECT_ARG(LONG, long, LONG_MAX);
  EXPECT_ARG(ULONG, unsigned long, 42);
  EXPECT_ARG(ULONG, unsigned long, ULONG_MAX);

  // Test long long.
  EXPECT_ARG(LONG_LONG, fmt::LongLong, 42);
  EXPECT_ARG(LONG_LONG, fmt::LongLong, LLONG_MIN);
  EXPECT_ARG(LONG_LONG, fmt::LongLong, LLONG_MAX);
  EXPECT_ARG(ULONG_LONG, fmt::ULongLong, 42);
  EXPECT_ARG(ULONG_LONG, fmt::ULongLong, ULLONG_MAX);

  // Test float.
  EXPECT_ARG(DOUBLE, float, 4.2);
  EXPECT_ARG(DOUBLE, float, FLT_MIN);
  EXPECT_ARG(DOUBLE, float, FLT_MAX);

  // Test double.
  EXPECT_ARG(DOUBLE, double, 4.2);
  EXPECT_ARG(DOUBLE, double, DBL_MIN);
  EXPECT_ARG(DOUBLE, double, DBL_MAX);

  // Test long double.
  EXPECT_ARG(LONG_DOUBLE, long double, 4.2);
  EXPECT_ARG(LONG_DOUBLE, long double, LDBL_MIN);
  EXPECT_ARG(LONG_DOUBLE, long double, LDBL_MAX);

  // Test string.
  char STR[] = "test";
  EXPECT_ARG(STRING, char*, STR);
  EXPECT_ARG(STRING, const char*, STR);
  EXPECT_ARG(STRING, std::string, STR);
  EXPECT_ARG(STRING, fmt::StringRef, STR);

  // Test wide string.
  wchar_t WSTR[] = L"test";
  EXPECT_ARGW(WSTRING, wchar_t*, WSTR);
  EXPECT_ARGW(WSTRING, const wchar_t*, WSTR);
  EXPECT_ARGW(WSTRING, std::wstring, WSTR);
  EXPECT_ARGW(WSTRING, fmt::WStringRef, WSTR);

  int n = 42;
  EXPECT_ARG(POINTER, void*, &n);
  EXPECT_ARG(POINTER, const void*, &n);

  ::Test t;
  fmt::internal::Arg arg = MakeArg<char>(t);
  EXPECT_EQ(fmt::internal::Arg::CUSTOM, arg.type);
  EXPECT_EQ(&t, arg.custom.value);
  fmt::Writer w;
  fmt::BasicFormatter<char> formatter(w);
  arg.custom.format(&formatter, &t, "}");
  EXPECT_EQ("test", w.str());
}

struct Result {
  fmt::internal::Arg arg;

  Result() : arg(MakeArg<char>(0xdeadbeef)) {}

  template <typename T>
  Result(const T& value) : arg(MakeArg<char>(value)) {}
  Result(const wchar_t *s) : arg(MakeArg<wchar_t>(s)) {}
};

struct TestVisitor : fmt::internal::ArgVisitor<TestVisitor, Result> {
  Result visit_int(int value) { return value; }
  Result visit_uint(unsigned value) { return value; }
  Result visit_long_long(fmt::LongLong value) { return value; }
  Result visit_ulong_long(fmt::ULongLong value) { return value; }
  Result visit_double(double value) { return value; }
  Result visit_long_double(long double value) { return value; }
  Result visit_char(int value) { return static_cast<char>(value); }
  Result visit_string(Arg::StringValue<char> s) { return s.value; }
  Result visit_wstring(Arg::StringValue<wchar_t> s) { return s.value; }
  Result visit_pointer(const void *p) { return p; }
  Result visit_custom(Arg::CustomValue c) {
    return *static_cast<const ::Test*>(c.value);
  }
};

#define EXPECT_RESULT_(Char, type_code, value) { \
  Result result = TestVisitor().visit(MakeArg<Char>(value)); \
  EXPECT_EQ(Arg::type_code, result.arg.type); \
  EXPECT_EQ(value, ArgInfo<Arg::type_code>::get(result.arg)); \
}

#define EXPECT_RESULT(type_code, value) \
  EXPECT_RESULT_(char, type_code, value)
#define EXPECT_RESULTW(type_code, value) \
  EXPECT_RESULT_(wchar_t, type_code, value)

TEST(ArgVisitorTest, VisitAll) {
  EXPECT_RESULT(INT, 42);
  EXPECT_RESULT(UINT, 42u);
  EXPECT_RESULT(LONG_LONG, 42ll);
  EXPECT_RESULT(ULONG_LONG, 42ull);
  EXPECT_RESULT(DOUBLE, 4.2);
  EXPECT_RESULT(LONG_DOUBLE, 4.2l);
  EXPECT_RESULT(CHAR, 'x');
  const char STR[] = "abc";
  EXPECT_RESULT(STRING, STR);
  const wchar_t WSTR[] = L"abc";
  EXPECT_RESULTW(WSTRING, WSTR);
  const void *p = STR;
  EXPECT_RESULT(POINTER, p);
  ::Test t;
  Result result = TestVisitor().visit(MakeArg<char>(t));
  EXPECT_EQ(Arg::CUSTOM, result.arg.type);
  EXPECT_EQ(&t, result.arg.custom.value);
}

struct TestAnyVisitor : fmt::internal::ArgVisitor<TestAnyVisitor, Result> {
  template <typename T>
  Result visit_any_int(T value) { return value; }

  template <typename T>
  Result visit_any_double(T value) { return value; }
};

#undef EXPECT_RESULT
#define EXPECT_RESULT(type_code, value) { \
  Result result = TestAnyVisitor().visit(MakeArg<char>(value)); \
  EXPECT_EQ(Arg::type_code, result.arg.type); \
  EXPECT_EQ(value, ArgInfo<Arg::type_code>::get(result.arg)); \
}

TEST(ArgVisitorTest, VisitAny) {
  EXPECT_RESULT(INT, 42);
  EXPECT_RESULT(UINT, 42u);
  EXPECT_RESULT(LONG_LONG, 42ll);
  EXPECT_RESULT(ULONG_LONG, 42ull);
  EXPECT_RESULT(DOUBLE, 4.2);
  EXPECT_RESULT(LONG_DOUBLE, 4.2l);
}

struct TestUnhandledVisitor :
    fmt::internal::ArgVisitor<TestUnhandledVisitor, const char *> {
  const char *visit_unhandled_arg() { return "test"; }
};

#define EXPECT_UNHANDLED(value) \
  EXPECT_STREQ("test", TestUnhandledVisitor().visit(MakeArg<wchar_t>(value)));

TEST(ArgVisitorTest, VisitUnhandledArg) {
  EXPECT_UNHANDLED(42);
  EXPECT_UNHANDLED(42u);
  EXPECT_UNHANDLED(42ll);
  EXPECT_UNHANDLED(42ull);
  EXPECT_UNHANDLED(4.2);
  EXPECT_UNHANDLED(4.2l);
  EXPECT_UNHANDLED('x');
  const char STR[] = "abc";
  EXPECT_UNHANDLED(STR);
  const wchar_t WSTR[] = L"abc";
  EXPECT_UNHANDLED(WSTR);
  const void *p = STR;
  EXPECT_UNHANDLED(p);
  EXPECT_UNHANDLED(::Test());
}

TEST(ArgVisitorTest, VisitInvalidArg) {
  Arg arg = {static_cast<Arg::Type>(Arg::CUSTOM + 1)};
  EXPECT_DEBUG_DEATH(TestVisitor().visit(arg), "Assertion");
}

// Tests fmt::internal::count_digits for integer type Int.
template <typename Int>
void TestCountDigits(Int) {
  for (Int i = 0; i < 10; ++i)
    EXPECT_EQ(1u, fmt::internal::count_digits(i));
  for (Int i = 1, n = 1,
      end = std::numeric_limits<Int>::max() / 10; n <= end; ++i) {
    n *= 10;
    EXPECT_EQ(i, fmt::internal::count_digits(n - 1));
    EXPECT_EQ(i + 1, fmt::internal::count_digits(n));
  }
}

TEST(UtilTest, CountDigits) {
  TestCountDigits(uint32_t());
  TestCountDigits(uint64_t());
}

#ifdef _WIN32
TEST(UtilTest, UTF16ToUTF8) {
  std::string s = "ёжик";
  fmt::internal::UTF16ToUTF8 u(L"\x0451\x0436\x0438\x043A");
  EXPECT_EQ(s, u.str());
  EXPECT_EQ(s.size(), u.size());
}

TEST(UtilTest, UTF8ToUTF16) {
  std::string s = "лошадка";
  fmt::internal::UTF8ToUTF16 u(s.c_str());
  EXPECT_EQ(L"\x043B\x043E\x0448\x0430\x0434\x043A\x0430", u.str());
  EXPECT_EQ(7, u.size());
}

template <typename Converter>
void CheckUTFConversionError(const char *message) {
  fmt::Writer out;
  fmt::internal::FormatWinErrorMessage(out, ERROR_INVALID_PARAMETER, message);
  fmt::SystemError error(0, "");
  try {
    Converter(0);
  } catch (const fmt::SystemError &e) {
    error = e;
  }
  EXPECT_EQ(ERROR_INVALID_PARAMETER, error.error_code());
  EXPECT_EQ(out.str(), error.what());
}

TEST(UtilTest, UTF16ToUTF8Error) {
  CheckUTFConversionError<fmt::internal::UTF16ToUTF8>(
      "cannot convert string from UTF-16 to UTF-8");
}

TEST(UtilTest, UTF8ToUTF16Error) {
  CheckUTFConversionError<fmt::internal::UTF8ToUTF16>(
      "cannot convert string from UTF-8 to UTF-16");
}

TEST(UtilTest, UTF16ToUTF8Convert) {
  fmt::internal::UTF16ToUTF8 u;
  EXPECT_EQ(ERROR_INVALID_PARAMETER, u.Convert(0));
}
#endif  // _WIN32

TEST(UtilTest, StrError) {
  using fmt::internal::StrError;
  char *message = 0;
  char buffer[BUFFER_SIZE];
#ifndef NDEBUG
  EXPECT_DEBUG_DEATH(StrError(EDOM, message = 0, 0), "Assertion");
  EXPECT_DEBUG_DEATH(StrError(EDOM, message = buffer, 0), "Assertion");
#endif
  buffer[0] = 'x';
#ifdef _GNU_SOURCE
  // Use invalid error code to make sure that StrError returns an error
  // message in the buffer rather than a pointer to a static string.
  int error_code = -1;
#else
  int error_code = EDOM;
#endif

  int result = StrError(error_code, message = buffer, BUFFER_SIZE);
  EXPECT_EQ(0, result);
  std::size_t message_size = std::strlen(message);
  EXPECT_GE(BUFFER_SIZE - 1u, message_size);
  EXPECT_EQ(GetSystemErrorMessage(error_code), message);

  // StrError never uses buffer on MinGW.
#ifndef __MINGW32__
  result = StrError(error_code, message = buffer, message_size);
  EXPECT_EQ(ERANGE, result);
  result = StrError(error_code, message = buffer, 1);
  EXPECT_EQ(buffer, message);  // Message should point to buffer.
  EXPECT_EQ(ERANGE, result);
  EXPECT_STREQ("", message);
#endif
}

TEST(UtilTest, SystemError) {
  fmt::SystemError e(EDOM, "test");
  EXPECT_EQ(fmt::format("test: {}", GetSystemErrorMessage(EDOM)), e.what());
  EXPECT_EQ(EDOM, e.error_code());
}

typedef void (*FormatErrorMessage)(
        fmt::Writer &out, int error_code, StringRef message);

template <typename Error>
void CheckThrowError(int error_code, FormatErrorMessage format) {
  fmt::SystemError error(0, "");
  try {
    throw Error(error_code, "test {}", "error");
  } catch (const fmt::SystemError &e) {
    error = e;
  }
  fmt::Writer message;
  format(message, error_code, "test error");
  EXPECT_EQ(message.str(), error.what());
  EXPECT_EQ(error_code, error.error_code());
}

TEST(UtilTest, FormatSystemErrorMessage) {
  fmt::Writer message;
  fmt::internal::FormatSystemErrorMessage(message, EDOM, "test");
  EXPECT_EQ(fmt::format("test: {}",
          GetSystemErrorMessage(EDOM)), message.str());
}

TEST(UtilTest, ThrowSystemError) {
  CheckThrowError<fmt::SystemError>(EDOM, fmt::internal::FormatSystemErrorMessage);
}

TEST(UtilTest, ReportSystemError) {
  fmt::Writer out;
  fmt::internal::FormatSystemErrorMessage(out, EDOM, "test error");
  out << '\n';
  EXPECT_WRITE(stderr, fmt::ReportSystemError(EDOM, "test error"), out.str());
}

#ifdef _WIN32

TEST(UtilTest, FormatWinErrorMessage) {
  LPWSTR message = 0;
  FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
      FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, 0,
      ERROR_FILE_EXISTS, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
      reinterpret_cast<LPWSTR>(&message), 0, 0);
  fmt::internal::UTF16ToUTF8 utf8_message(message);
  LocalFree(message);
  fmt::Writer actual_message;
  fmt::internal::FormatWinErrorMessage(
      actual_message, ERROR_FILE_EXISTS, "test");
  EXPECT_EQ(fmt::format("test: {}", utf8_message.str()),
      actual_message.str());
}

TEST(UtilTest, ThrowWinError) {
  CheckThrowError<fmt::WindowsError>(
      ERROR_FILE_EXISTS, fmt::internal::FormatWinErrorMessage);
}

TEST(UtilTest, ReportWinError) {
  fmt::Writer out;
  fmt::internal::FormatWinErrorMessage(out, ERROR_FILE_EXISTS, "test error");
  out << '\n';
  EXPECT_WRITE(stderr,
      fmt::ReportWinError(ERROR_FILE_EXISTS, "test error"), out.str());
}

#endif  // _WIN32
