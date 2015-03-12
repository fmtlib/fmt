/*
 A C++ interface to POSIX functions.

 Copyright (c) 2014 - 2015, Victor Zverovich
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

#ifndef FMT_POSIX_H_
#define FMT_POSIX_H_

#ifdef __MINGW32__
// Workaround MinGW bug https://sourceforge.net/p/mingw/bugs/2024/.
# undef __STRICT_ANSI__
#endif

#include <errno.h>
#include <fcntl.h>  // for O_RDONLY
#include <stdio.h>

#include <cstddef>

#include "format.h"

#ifdef FMT_INCLUDE_POSIX_TEST
# include "test/posix-test.h"
#endif

#ifndef FMT_POSIX
# if defined(_WIN32) && !defined(__MINGW32__)
// Fix warnings about deprecated symbols.
#  define FMT_POSIX(call) _##call
# else
#  define FMT_POSIX(call) call
# endif
#endif

// Calls to system functions are wrapped in FMT_SYSTEM for testability.
#ifdef FMT_SYSTEM
# define FMT_POSIX_CALL(call) FMT_SYSTEM(call)
#else
# define FMT_SYSTEM(call) call
# ifdef _WIN32
// Fix warnings about deprecated symbols.
#  define FMT_POSIX_CALL(call) ::_##call
# else
#  define FMT_POSIX_CALL(call) ::call
# endif
#endif

#if FMT_GCC_VERSION >= 407
# define FMT_UNUSED __attribute__((unused))
#else
# define FMT_UNUSED
#endif

#if FMT_USE_STATIC_ASSERT || FMT_HAS_CPP_ATTRIBUTE(cxx_static_assert) || \
  (FMT_GCC_VERSION >= 403 && FMT_HAS_GXX_CXX11) || _MSC_VER >= 1600
# define FMT_STATIC_ASSERT(cond, message) static_assert(cond, message)
#else
# define FMT_CONCAT_(a, b) FMT_CONCAT(a, b)
# define FMT_STATIC_ASSERT(cond, message) \
  typedef int FMT_CONCAT_(Assert, __LINE__)[(cond) ? 1 : -1] FMT_UNUSED
#endif

// Retries the expression while it evaluates to error_result and errno
// equals to EINTR.
#ifndef _WIN32
# define FMT_RETRY_VAL(result, expression, error_result) \
  do { \
    result = (expression); \
  } while (result == error_result && errno == EINTR)
#else
# define FMT_RETRY_VAL(result, expression, error_result) result = (expression)
#endif

#define FMT_RETRY(result, expression) FMT_RETRY_VAL(result, expression, -1)

namespace fmt {

// An error code.
class ErrorCode {
 private:
  int value_;

 public:
  explicit ErrorCode(int value = 0) FMT_NOEXCEPT : value_(value) {}

  int get() const FMT_NOEXCEPT { return value_; }
};

// A buffered file.
class BufferedFile {
 private:
  FILE *file_;

  friend class File;

  explicit BufferedFile(FILE *f) : file_(f) {}

 public:
  // Constructs a BufferedFile object which doesn't represent any file.
  BufferedFile() FMT_NOEXCEPT : file_(0) {}

  // Destroys the object closing the file it represents if any.
  ~BufferedFile() FMT_NOEXCEPT;

#if !FMT_USE_RVALUE_REFERENCES
  // Emulate a move constructor and a move assignment operator if rvalue
  // references are not supported.

 private:
  // A proxy object to emulate a move constructor.
  // It is private to make it impossible call operator Proxy directly.
  struct Proxy {
    FILE *file;
  };

public:
  // A "move constructor" for moving from a temporary.
  BufferedFile(Proxy p) FMT_NOEXCEPT : file_(p.file) {}

  // A "move constructor" for for moving from an lvalue.
  BufferedFile(BufferedFile &f) FMT_NOEXCEPT : file_(f.file_) {
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
  operator Proxy() FMT_NOEXCEPT {
    Proxy p = {file_};
    file_ = 0;
    return p;
  }

#else
 private:
  FMT_DISALLOW_COPY_AND_ASSIGN(BufferedFile);

 public:
  BufferedFile(BufferedFile &&other) FMT_NOEXCEPT : file_(other.file_) {
    other.file_ = 0;
  }

  BufferedFile& operator=(BufferedFile &&other) {
    close();
    file_ = other.file_;
    other.file_ = 0;
    return *this;
  }
#endif

  // Opens a file.
  BufferedFile(fmt::StringRef filename, fmt::StringRef mode);

  // Closes the file.
  void close();

  // Returns the pointer to a FILE object representing this file.
  FILE *get() const FMT_NOEXCEPT { return file_; }

  // We place parentheses around fileno to workaround a bug in some versions
  // of MinGW that define fileno as a macro.
  int (fileno)() const;

  void print(fmt::StringRef format_str, const ArgList &args) {
    fmt::print(file_, format_str, args);
  }
  FMT_VARIADIC(void, print, fmt::StringRef)
};

// A file. Closed file is represented by a File object with descriptor -1.
// Methods that are not declared with FMT_NOEXCEPT may throw
// fmt::SystemError in case of failure. Note that some errors such as
// closing the file multiple times will cause a crash on Windows rather
// than an exception. You can get standard behavior by overriding the
// invalid parameter handler with _set_invalid_parameter_handler.
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
  File() FMT_NOEXCEPT : fd_(-1) {}

  // Opens a file and constructs a File object representing this file.
  File(fmt::StringRef path, int oflag);

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
  File(Proxy p) FMT_NOEXCEPT : fd_(p.fd) {}

  // A "move constructor" for for moving from an lvalue.
  File(File &other) FMT_NOEXCEPT : fd_(other.fd_) {
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
  operator Proxy() FMT_NOEXCEPT {
    Proxy p = {fd_};
    fd_ = -1;
    return p;
  }

#else
 private:
  FMT_DISALLOW_COPY_AND_ASSIGN(File);

 public:
  File(File &&other) FMT_NOEXCEPT : fd_(other.fd_) {
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
  ~File() FMT_NOEXCEPT;

  // Returns the file descriptor.
  int descriptor() const FMT_NOEXCEPT { return fd_; }

  // Closes the file.
  void close();

  // Returns the file size.
  fmt::LongLong size() const;

  // Attempts to read count bytes from the file into the specified buffer.
  std::size_t read(void *buffer, std::size_t count);

  // Attempts to write count bytes from the specified buffer to the file.
  std::size_t write(const void *buffer, std::size_t count);

  // Duplicates a file descriptor with the dup function and returns
  // the duplicate as a file object.
  static File dup(int fd);

  // Makes fd be the copy of this file descriptor, closing fd first if
  // necessary.
  void dup2(int fd);

  // Makes fd be the copy of this file descriptor, closing fd first if
  // necessary.
  void dup2(int fd, ErrorCode &ec) FMT_NOEXCEPT;

  // Creates a pipe setting up read_end and write_end file objects for reading
  // and writing respectively.
  static void pipe(File &read_end, File &write_end);

  // Creates a BufferedFile object associated with this file and detaches
  // this File object from the file.
  BufferedFile fdopen(const char *mode);
};

// Returns the memory page size.
long getpagesize();
}  // namespace fmt

#if !FMT_USE_RVALUE_REFERENCES
namespace std {
// For compatibility with C++98.
inline fmt::BufferedFile &move(fmt::BufferedFile &f) { return f; }
inline fmt::File &move(fmt::File &f) { return f; }
}
#endif

#endif  // FMT_POSIX_H_
