// Formatting library for C++ - custom Google Test assertions
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_GTEST_EXTRA_H_
#define FMT_GTEST_EXTRA_H_

#include <string>

#include "fmt/os.h"
#include "gmock.h"

#define FMT_TEST_THROW_(statement, expected_exception, expected_message, fail) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_                                                \
  if (::testing::AssertionResult gtest_ar = ::testing::AssertionSuccess()) {   \
    std::string gtest_expected_message = expected_message;                     \
    bool gtest_caught_expected = false;                                        \
    try {                                                                      \
      GTEST_SUPPRESS_UNREACHABLE_CODE_WARNING_BELOW_(statement);               \
    } catch (expected_exception const& e) {                                    \
      if (gtest_expected_message != e.what()) {                                \
        gtest_ar << #statement                                                 \
            " throws an exception with a different message.\n"                 \
                 << "Expected: " << gtest_expected_message << "\n"             \
                 << "  Actual: " << e.what();                                  \
        goto GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__);            \
      }                                                                        \
      gtest_caught_expected = true;                                            \
    } catch (...) {                                                            \
      gtest_ar << "Expected: " #statement                                      \
                  " throws an exception of type " #expected_exception          \
                  ".\n  Actual: it throws a different type.";                  \
      goto GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__);              \
    }                                                                          \
    if (!gtest_caught_expected) {                                              \
      gtest_ar << "Expected: " #statement                                      \
                  " throws an exception of type " #expected_exception          \
                  ".\n  Actual: it throws nothing.";                           \
      goto GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__);              \
    }                                                                          \
  } else                                                                       \
    GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__)                      \
        : fail(gtest_ar.failure_message())

// Tests that the statement throws the expected exception and the exception's
// what() method returns expected message.
#define EXPECT_THROW_MSG(statement, expected_exception, expected_message) \
  FMT_TEST_THROW_(statement, expected_exception, expected_message,        \
                  GTEST_NONFATAL_FAILURE_)

std::string format_system_error(int error_code, fmt::string_view message);

#define EXPECT_SYSTEM_ERROR(statement, error_code, message) \
  EXPECT_THROW_MSG(statement, fmt::system_error,            \
                   format_system_error(error_code, message))

#if FMT_USE_FCNTL

// Captures file output by redirecting it to a pipe.
// The output it can handle is limited by the pipe capacity.
class OutputRedirect {
 private:
  FILE* file_;
  fmt::file original_;  // Original file passed to redirector.
  fmt::file read_end_;  // Read end of the pipe where the output is redirected.

  GTEST_DISALLOW_COPY_AND_ASSIGN_(OutputRedirect);

  void flush();
  void restore();

 public:
  explicit OutputRedirect(FILE* file);
  ~OutputRedirect() FMT_NOEXCEPT;

  // Restores the original file, reads output from the pipe into a string
  // and returns it.
  std::string restore_and_read();
};

#  define FMT_TEST_WRITE_(statement, expected_output, file, fail)              \
    GTEST_AMBIGUOUS_ELSE_BLOCKER_                                              \
    if (::testing::AssertionResult gtest_ar = ::testing::AssertionSuccess()) { \
      std::string gtest_expected_output = expected_output;                     \
      OutputRedirect gtest_redir(file);                                        \
      GTEST_SUPPRESS_UNREACHABLE_CODE_WARNING_BELOW_(statement);               \
      std::string gtest_output = gtest_redir.restore_and_read();               \
      if (gtest_output != gtest_expected_output) {                             \
        gtest_ar << #statement " produces different output.\n"                 \
                 << "Expected: " << gtest_expected_output << "\n"              \
                 << "  Actual: " << gtest_output;                              \
        goto GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__);            \
      }                                                                        \
    } else                                                                     \
      GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__)                    \
          : fail(gtest_ar.failure_message())

// Tests that the statement writes the expected output to file.
#  define EXPECT_WRITE(file, statement, expected_output) \
    FMT_TEST_WRITE_(statement, expected_output, file, GTEST_NONFATAL_FAILURE_)

#  ifdef _MSC_VER

// Suppresses Windows assertions on invalid file descriptors, making
// POSIX functions return proper error codes instead of crashing on Windows.
class SuppressAssert {
 private:
  _invalid_parameter_handler original_handler_;
  int original_report_mode_;

  static void handle_invalid_parameter(const wchar_t*, const wchar_t*,
                                       const wchar_t*, unsigned, uintptr_t) {}

 public:
  SuppressAssert()
      : original_handler_(
            _set_invalid_parameter_handler(handle_invalid_parameter)),
        original_report_mode_(_CrtSetReportMode(_CRT_ASSERT, 0)) {}
  ~SuppressAssert() {
    _set_invalid_parameter_handler(original_handler_);
    _CrtSetReportMode(_CRT_ASSERT, original_report_mode_);
  }
};

#    define SUPPRESS_ASSERT(statement) \
      {                                \
        SuppressAssert sa;             \
        statement;                     \
      }
#  else
#    define SUPPRESS_ASSERT(statement) statement
#  endif  // _MSC_VER

#  define EXPECT_SYSTEM_ERROR_NOASSERT(statement, error_code, message) \
    EXPECT_SYSTEM_ERROR(SUPPRESS_ASSERT(statement), error_code, message)

// Attempts to read count characters from a file.
std::string read(fmt::file& f, size_t count);

#  define EXPECT_READ(file, expected_content) \
    EXPECT_EQ(expected_content, read(file, std::strlen(expected_content)))

#else
#  define EXPECT_WRITE(file, statement, expected_output) SUCCEED()
#endif  // FMT_USE_FCNTL

template <typename Mock> struct ScopedMock : testing::StrictMock<Mock> {
  ScopedMock() { Mock::instance = this; }
  ~ScopedMock() { Mock::instance = nullptr; }
};

#endif  // FMT_GTEST_EXTRA_H_
