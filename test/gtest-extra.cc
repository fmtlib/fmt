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

#include "gtest-extra.h"

#if FMT_USE_FILE_DESCRIPTORS

#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef _WIN32
# include <unistd.h>
#else
# include <io.h>

# define O_CREAT _O_CREAT
# define O_TRUNC _O_TRUNC
# define S_IRUSR _S_IREAD
# define S_IWUSR _S_IWRITE

// The read function is defined as returning int on Windows.
typedef int ssize_t;

#endif  // _WIN32

// Retries the expression while it evaluates to -1 and error equals to EINTR.
#define FMT_RETRY(result, expression) \
  do { \
    result = (expression); \
  } while (result == -1 && errno == EINTR)

FileDescriptor::FileDescriptor(const char *path, int oflag) {
  int mode = S_IRUSR | S_IWUSR;
#ifdef _WIN32
  fd_ = -1;
  _sopen_s(&fd_, path, oflag, _SH_DENYNO, mode);
#else
  FMT_RETRY(fd_, open(path, oflag, mode));
#endif
  if (fd_ == -1)
    fmt::ThrowSystemError(errno, "cannot open file {}") << path;
}

void FileDescriptor::close() {
  if (fd_ == -1)
    return;
  // Don't need to retry close in case of EINTR.
  // See http://linux.derkeiler.com/Mailing-Lists/Kernel/2005-09/3000.html
  if (::FMT_POSIX(close(fd_)) != 0)
    fmt::ReportSystemError(errno, "cannot close file");
}

FileDescriptor FileDescriptor::dup(int fd) {
  int new_fd = 0;
  FMT_RETRY(new_fd, ::FMT_POSIX(dup(fd)));
  if (new_fd == -1)
    fmt::ThrowSystemError(errno, "cannot duplicate file descriptor {}") << fd;
  return FileDescriptor(new_fd);
}

void FileDescriptor::dup2(int fd) {
  int result = 0;
  FMT_RETRY(result, ::FMT_POSIX(dup2(fd_, fd)));
  if (result == -1) {
    fmt::ThrowSystemError(errno,
        "cannot duplicate file descriptor {} to {}") << fd_ << fd;
  }
}

void FileDescriptor::dup2(int fd, ErrorCode &ec) FMT_NOEXCEPT(true) {
  int result = 0;
  FMT_RETRY(result, ::FMT_POSIX(dup2(fd_, fd)));
  if (result == -1)
    ec = ErrorCode(errno);
}

void FileDescriptor::pipe(FileDescriptor &read_fd, FileDescriptor &write_fd) {
  // Close the descriptors first to make sure that assignments don't throw
  // and there are no leaks.
  read_fd.close();
  write_fd.close();
  int fds[2] = {};
#ifdef _WIN32
  // Make the default pipe capacity same as on Linux 2.6.11+.
  enum { DEFAULT_CAPACITY = 65536 };
  int result = _pipe(fds, DEFAULT_CAPACITY, _O_BINARY);
#else
  int result = ::pipe(fds);
#endif
  if (result != 0)
    fmt::ThrowSystemError(errno, "cannot create pipe");
  // The following assignments don't throw because read_fd and write_fd
  // are closed.
  read_fd = FileDescriptor(fds[0]);
  write_fd = FileDescriptor(fds[1]);
}

OutputRedirector::OutputRedirector(FILE *file) : file_(file) {
  if (std::fflush(file) != 0)
    fmt::ThrowSystemError(errno, "cannot flush stream");
  int fd = fileno(file);
  saved_fd_ = FileDescriptor::dup(fd);
  FileDescriptor write_fd;
  FileDescriptor::pipe(read_fd_, write_fd);
  write_fd.dup2(fd);
}

OutputRedirector::~OutputRedirector() {
  if (std::fflush(file_) != 0)
    fmt::ReportSystemError(errno, "cannot flush stream");
  ErrorCode ec;
  saved_fd_.dup2(fileno(file_), ec);
  if (ec.get())
    fmt::ReportSystemError(errno, "cannot restore output");
}

std::string OutputRedirector::Read() {
  // Restore output.
  if (std::fflush(file_) != 0)
    fmt::ThrowSystemError(errno, "cannot flush stream");
  saved_fd_.dup2(fileno(file_));

  // TODO: move to FileDescriptor
  enum { BUFFER_SIZE = 100 };
  char buffer[BUFFER_SIZE];
  ssize_t result = read(read_fd_.get(), buffer, BUFFER_SIZE);
  if (result == -1)
    fmt::ThrowSystemError(errno, "cannot read file");
  buffer[std::min<ssize_t>(BUFFER_SIZE - 1, result)] = '\0';
  return buffer;
}

// TODO: test EXPECT_STDOUT and EXPECT_STDERR

#endif  // FMT_USE_FILE_DESCRIPTORS
