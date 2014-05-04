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
#include <cstdio>
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

// A buffered file.
class BufferedFile {
 private:
  std::FILE *file_;

  friend class File;

  explicit BufferedFile(std::FILE *f) : file_(f) {}

  void close();

 public:
  // Constructs a BufferedFile object which doesn't represent any file.
  BufferedFile() FMT_NOEXCEPT(true) : file_(0) {}

  // Destroys the object closing the file it represents if any.
  ~BufferedFile() FMT_NOEXCEPT(true);

#if !FMT_USE_RVALUE_REFERENCES
  // Emulate a move constructor and a move assignment operator if rvalue
  // references are not supported.

 private:
  // A proxy object to emulate a move constructor.
  // It is private to make it impossible call operator Proxy directly.
  struct Proxy {
    std::FILE *file;
  };

 public:
  // A "move constructor" for moving from a temporary.
  BufferedFile(Proxy p) FMT_NOEXCEPT(true) : file_(p.file) {}

  // A "move constructor" for for moving from an lvalue.
  BufferedFile(BufferedFile &f) FMT_NOEXCEPT(true) : file_(f.file_) {
    f.file_ = 0;
  }

  // A "move assignment operator" for moving from a temporary.
  BufferedFile &operator=(Proxy p) {
    close();
    file_ = p.file;
    return *this;
  }

  // A "move assignment operator" for moving from an lvalue.
  BufferedFile &operator=(BufferedFile &other) {
    close();
    file_ = other.file_;
    other.file_ = 0;
    return *this;
  }

  // Returns a proxy object for moving from a temporary:
  //   BufferedFile file = BufferedFile(...);
  operator Proxy() FMT_NOEXCEPT(true) {
    Proxy p = {file_};
    file_ = 0;
    return p;
  }
#else
 private:
  GTEST_DISALLOW_COPY_AND_ASSIGN_(BufferedFile);

 public:
  BufferedFile(BufferedFile &&other) FMT_NOEXCEPT(true) : file_(other.file_) {
    other.file_ = 0;
  }

  BufferedFile& operator=(BufferedFile &&other) {
    close();
    file_ = other.file_;
    other.file_ = 0;
    return *this;
  }
#endif

  std::FILE *get() const { return file_; }
};

// A file.
// Methods that are not declared with FMT_NOEXCEPT(true) may throw
// fmt::SystemError in case of failure. Note that some errors such as
// closing the file multiple times will cause a crash on Windows rather
// than an exception.
class File {
 private:
  int fd_;  // File descriptor.

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
  // A "move constructor" for moving from a temporary.
  File(Proxy p) FMT_NOEXCEPT(true) : fd_(p.fd) {}

  // A "move constructor" for for moving from an lvalue.
  File(File &other) FMT_NOEXCEPT(true) : fd_(other.fd_) {
    other.fd_ = -1;
  }

  // A "move assignment operator" for moving from a temporary.
  File &operator=(Proxy p) {
    close();
    fd_ = p.fd;
    return *this;
  }

  // A "move assignment operator" for moving from an lvalue.
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

  File& operator=(File &&other) {
    close();
    fd_ = other.fd_;
    other.fd_ = -1;
    return *this;
  }
#endif

  // Destroys the object closing the file it represents if any.
  ~File() FMT_NOEXCEPT(true);

  // Returns the file descriptor.
  int descriptor() const FMT_NOEXCEPT(true) { return fd_; }

  // Closes the file if its descriptor is not -1.
  void close();

  // Attempts to read count bytes from the file into the specified buffer.
  std::streamsize read(void *buffer, std::size_t count);

  // Attempts to write count bytes from the specified buffer to the file.
  std::streamsize write(const void *buffer, std::size_t count);

  // Duplicates a file descriptor with the dup function and returns
  // the duplicate as a file object.
  static File dup(int fd);

  // Makes fd be the copy of this file descriptor, closing fd first if
  // necessary.
  void dup2(int fd);

  // Makes fd be the copy of this file descriptor, closing fd first if
  // necessary.
  void dup2(int fd, ErrorCode &ec) FMT_NOEXCEPT(true);

  // Creates a pipe setting up read_end and write_end file objects for reading
  // and writing respectively.
  static void pipe(File &read_end, File &write_end);

  // Creates a BufferedFile object associated with this file and detaches
  // this File object from the file.
  BufferedFile fdopen(const char *mode);
};

#if !FMT_USE_RVALUE_REFERENCES
namespace std {
// For compatibility with C++98.
inline BufferedFile &move(BufferedFile &f) { return f; }
inline File &move(File &f) { return f; }
}
#endif

// Captures file output by redirecting it to a pipe.
// The output it can handle is limited by the pipe capacity.
class OutputRedirect {
 private:
  std::FILE *file_;
  File original_;  // Original file passed to redirector.
  File read_end_;  // Read end of the pipe where the output is redirected.

  GTEST_DISALLOW_COPY_AND_ASSIGN_(OutputRedirect);

  void Flush();
  void Restore();

 public:
  explicit OutputRedirect(std::FILE *file);
  ~OutputRedirect() FMT_NOEXCEPT(true);

  // Restores the original file, reads output from the pipe into a string
  // and returns it.
  std::string RestoreAndRead();
};

#define FMT_TEST_PRINT_(statement, expected_output, file, fail) \
  GTEST_AMBIGUOUS_ELSE_BLOCKER_ \
  if (::testing::AssertionResult gtest_ar = ::testing::AssertionSuccess()) { \
    std::string gtest_output; \
    { \
      OutputRedirect gtest_redir(file); \
      GTEST_SUPPRESS_UNREACHABLE_CODE_WARNING_BELOW_(statement); \
      gtest_output = gtest_redir.RestoreAndRead(); \
    } \
    if (gtest_output != expected_output) { \
      gtest_ar \
        << #statement " produces different output.\n" \
        << "Expected: " << expected_output << "\n" \
        << "  Actual: " << gtest_output; \
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
