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

// Tests fmt::internal::CountDigits for integer type Int.
template <typename Int>
void TestCountDigits(Int) {
  for (Int i = 0; i < 10; ++i)
          EXPECT_EQ(1u, fmt::internal::CountDigits(i));
  for (Int i = 1, n = 1,
               end = std::numeric_limits<Int>::max() / 10; n <= end; ++i) {
    n *= 10;
    EXPECT_EQ(i, fmt::internal::CountDigits(n - 1));
    EXPECT_EQ(i + 1, fmt::internal::CountDigits(n));
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
  int result = StrError(error_code, message = buffer, 1);
  EXPECT_EQ(buffer, message);  // Message should point to buffer.
  EXPECT_EQ(ERANGE, result);
  EXPECT_STREQ("", message);
  result = StrError(error_code, message = buffer, BUFFER_SIZE);
  EXPECT_EQ(0, result);
  std::size_t message_size = std::strlen(message);
  EXPECT_GE(BUFFER_SIZE - 1u, message_size);
  EXPECT_EQ(GetSystemErrorMessage(error_code), message);
  result = StrError(error_code, message = buffer, message_size);
  EXPECT_EQ(ERANGE, result);
}

TEST(UtilTest, SystemError) {
  fmt::SystemError e(EDOM, "test");
  EXPECT_EQ(fmt::format("test: {}", GetSystemErrorMessage(EDOM)), e.what());
  EXPECT_EQ(EDOM, e.error_code());
}

typedef void (*FormatErrorMessage)(
        fmt::Writer &out, int error_code, StringRef message);

template <typename Sink>
void CheckErrorSink(int error_code, FormatErrorMessage format) {
  fmt::SystemError error(0, "");
  Sink sink(error_code);
  fmt::Writer w;
  w << "test";
  try {
    sink(w);
  } catch (const fmt::SystemError &e) {
    error = e;
  }
  fmt::Writer message;
  format(message, error_code, "test");
  EXPECT_EQ(message.str(), error.what());
  EXPECT_EQ(error_code, error.error_code());
}

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

TEST(UtilTest, SystemErrorSink) {
  CheckErrorSink<fmt::SystemErrorSink>(
          EDOM, fmt::internal::FormatSystemErrorMessage);
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

TEST(UtilTest, WinErrorSink) {
  CheckErrorSink<fmt::WinErrorSink>(
      ERROR_FILE_EXISTS, fmt::internal::FormatWinErrorMessage);
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
