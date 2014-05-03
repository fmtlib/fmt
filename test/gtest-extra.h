/*
 Custom Google Test assertions.

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

#ifndef FMT_GTEST_EXTRA_H
#define FMT_GTEST_EXTRA_H

#include <cstddef>
#include <ios>
#include <string>

#if FMT_USE_FILE_DESCRIPTORS
# include <fcntl.h>
#endif

#include <gtest/gtest.h>

#include "format.h"

#define FMT_TEST_THROW_(statement, expected_exception, expected_message, fail) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_ \
  if (::testing::AssertionResult gtest_ar = ::testing::AssertionSuccess()) { \
    bool gtest_caught_expected = false; \
    try { \
      GTEST_SUPPRESS_UNREACHABLE_CODE_WARNING_BELOW_(statement); \
    } \
    catch (expected_exception const& e) { \
      if (expected_message != std::string(e.what())) { \
        gtest_ar \
          << #statement " throws an exception with a different message.\n" \
          << "Expected: " << expected_message << "\n" \
          << "  Actual: " << e.what(); \
        goto GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__); \
      } \
      gtest_caught_expected = true; \
    } \
    catch (...) { \
      gtest_ar << \
          "Expected: " #statement " throws an exception of type " \
          #expected_exception ".\n  Actual: it throws a different type."; \
      goto GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__); \
    } \
    if (!gtest_caught_expected) { \
      gtest_ar << \
          "Expected: " #statement " throws an exception of type " \
          #expected_exception ".\n  Actual: it throws nothing."; \
      goto GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__); \
    } \
  } else \
    GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__): \
      fail(gtest_ar.failure_message())

// Tests that the statement throws the expected exception and the exception's
// what() method returns expected message.
#define EXPECT_THROW_MSG(statement, expected_exception, expected_message) \
  FMT_TEST_THROW_(statement, expected_exception, \
      expected_message, GTEST_NONFATAL_FAILURE_)

#ifndef FMT_USE_FILE_DESCRIPTORS
# define FMT_USE_FILE_DESCRIPTORS 0
#endif

#if FMT_USE_FILE_DESCRIPTORS

#ifdef _WIN32
// Fix warnings about deprecated symbols.
# define FMT_POSIX(name) _##name
#else
# define FMT_POSIX(name) name
#endif

// An error code.
class ErrorCode {
 private:
  int value_;

 public:
  explicit ErrorCode(int value = 0) FMT_NOEXCEPT(true) : value_(value) {}

  int get() const FMT_NOEXCEPT(true) { return value_; }
};

// A file.
class File {
 private:
  int fd_;  // File descriptor.

  // Closes the file if its descriptor is not -1.
  void close();

  // Constructs a File object with a given descriptor.
  explicit File(int fd) : fd_(fd) {}

 public:
  // Possible values for the oflag argument to the constructor.
  enum {
    RDONLY = FMT_POSIX(O_RDONLY), // Open for reading only.
    WRONLY = FMT_POSIX(O_WRONLY), // Open for writing only.
    RDWR   = FMT_POSIX(O_RDWR)    // Open for reading and writing.
  };

  // Constructs a File object which doesn't represent any file.
  File() FMT_NOEXCEPT(true) : fd_(-1) {}

  // Opens a file and constructs a File object representing this file.
  // Throws fmt::SystemError on error.
  File(const char *path, int oflag);

#if !FMT_USE_RVALUE_REFERENCES
  // Emulate a move constructor and a move assignment operator if rvalue
  // references are not supported.

 private:
  // A proxy object to emulate a move constructor.
  // It is private to make it impossible call operator Proxy directly.
  struct Proxy {
    int fd;
  };

 public:
  // A "move" constructor for moving from a temporary.
  File(Proxy p) FMT_NOEXCEPT(true) : fd_(p.fd) {}

  // A "move" constructor for for moving from an lvalue.
  File(File &other) FMT_NOEXCEPT(true) : fd_(other.fd_) {
    other.fd_ = -1;
  }

  // A "move" assignment operator for moving from a temporary.
  File &operator=(Proxy p) {
    close();
    fd_ = p.fd;
    return *this;
  }

  // A "move" assignment operator for moving from an lvalue.
  File &operator=(File &other) {
    close();
    fd_ = other.fd_;
    other.fd_ = -1;
    return *this;
  }

  // Returns a proxy object for moving from a temporary:
  //   File file = File(...);
  operator Proxy() FMT_NOEXCEPT(true) {
    Proxy p = {fd_};
    fd_ = -1;
    return p;
  }
#else
 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(File);

 public:
  File(File &&other) FMT_NOEXCEPT(true) : fd_(other.fd_) {
    other.fd_ = -1;
  }

  File& operator=(File &&other) FMT_NOEXCEPT(true) {
    fd_ = other.fd_;
    other.fd_ = -1;
    return *this;
  }
#endif

  // Destroys the object closing the file it represents if any.
  ~File() { close(); }

  // Returns the file descriptor.
  // TODO: rename
  int get() const FMT_NOEXCEPT(true) { return fd_; }

  // Attempts to read count bytes from the file into the specified buffer.
  std::streamsize read(void *buffer, std::size_t count);

  // Attempts to write count bytes from the specified buffer to the file.
  std::streamsize write(const void *buffer, std::size_t count);

  // Duplicates a file descriptor with the dup function and returns
  // the duplicate as a file object. Throws fmt::SystemError on error.
  static File dup(int fd);

  // Makes fd be the copy of this file descriptor, closing fd first if
  // necessary. Throws fmt::SystemError on error.
  void dup2(int fd);

  // Makes fd be the copy of this file descriptor, closing fd first if
  // necessary.
  void dup2(int fd, ErrorCode &ec) FMT_NOEXCEPT(true);

  // Creates a pipe setting up read_end and write_end file objects for reading
  // and writing respectively. Throws fmt::SystemError on error.
  static void pipe(File &read_end, File &write_end);
};

#if !FMT_USE_RVALUE_REFERENCES
namespace std {
// For compatibility with C++98.
inline File &move(File &f) { return f; }
}
#endif

// Redirect file output to a pipe.
class OutputRedirector {
 private:
  FILE *file_;
  File saved_;     // Saved file created with dup.
  File read_end_;  // Read end of the pipe where the output is redirected.

  GTEST_DISALLOW_COPY_AND_ASSIGN_(OutputRedirector);

 public:
  explicit OutputRedirector(FILE *file);
  ~OutputRedirector();

  std::string Read();
};

#define FMT_TEST_PRINT_(statement, expected_output, file, fail) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_ \
  if (::testing::AssertionResult gtest_ar = ::testing::AssertionSuccess()) { \
    std::string output; \
    { \
      OutputRedirector redir(file); \
      GTEST_SUPPRESS_UNREACHABLE_CODE_WARNING_BELOW_(statement); \
      output = redir.Read(); \
    } \
    if (output != expected_output) { \
      gtest_ar \
        << #statement " produces different output.\n" \
        << "Expected: " << expected_output << "\n" \
        << "  Actual: " << output; \
      goto GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__); \
    } \
  } else \
    GTEST_CONCAT_TOKEN_(gtest_label_testthrow_, __LINE__): \
      fail(gtest_ar.failure_message())

// Tests that the statement prints the expected output to stdout.
#define EXPECT_STDOUT(statement, expected_output) \
    FMT_TEST_PRINT_(statement, expected_output, stdout, GTEST_NONFATAL_FAILURE_)

// Tests that the statement prints the expected output to stderr.
#define EXPECT_STDERR(statement, expected_output) \
    FMT_TEST_PRINT_(statement, expected_output, stderr, GTEST_NONFATAL_FAILURE_)

#endif  // FMT_USE_FILE_DESCRIPTORS

#endif  // FMT_GTEST_EXTRA_H
