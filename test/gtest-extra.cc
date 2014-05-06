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

#endif  // _WIN32

// Retries the expression while it evaluates to -1 and error equals to EINTR.
#define FMT_RETRY(result, expression) \
  do { \
    result = (expression); \
  } while (result == -1 && errno == EINTR)

BufferedFile::~BufferedFile() FMT_NOEXCEPT(true) {
  if (file_ && std::fclose(file_) != 0)
    fmt::ReportSystemError(errno, "cannot close file");
}

void BufferedFile::close() {
  if (!file_)
    return;
  int result = std::fclose(file_);
  file_ = 0;
  if (result != 0)
    fmt::ThrowSystemError(errno, "cannot close file");
}

int BufferedFile::fileno() const {
  int fd = ::FMT_POSIX(fileno(file_));
  if (fd == -1)
    fmt::ThrowSystemError(errno, "cannot get file descriptor");
  return fd;
}

File::File(const char *path, int oflag) {
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

File::~File() FMT_NOEXCEPT(true) {
  // Don't retry close in case of EINTR!
  // See http://linux.derkeiler.com/Mailing-Lists/Kernel/2005-09/3000.html
  if (fd_ != -1 && ::FMT_POSIX(close(fd_)) != 0)
    fmt::ReportSystemError(errno, "cannot close file");
}

void File::close() {
  if (fd_ == -1)
    return;
  // Don't retry close in case of EINTR!
  // See http://linux.derkeiler.com/Mailing-Lists/Kernel/2005-09/3000.html
  int result = ::FMT_POSIX(close(fd_));
  fd_ = -1;
  if (result != 0)
    fmt::ThrowSystemError(errno, "cannot close file");
}

std::streamsize File::read(void *buffer, std::size_t count) {
  std::streamsize result = 0;
  FMT_RETRY(result, ::FMT_POSIX(read(fd_, buffer, count)));
  if (result == -1)
    fmt::ThrowSystemError(errno, "cannot read from file");
  return result;
}

std::streamsize File::write(const void *buffer, std::size_t count) {
  std::streamsize result = 0;
  FMT_RETRY(result, ::FMT_POSIX(write(fd_, buffer, count)));
  if (result == -1)
    fmt::ThrowSystemError(errno, "cannot write to file");
  return result;
}

File File::dup(int fd) {
  int new_fd = 0;
  FMT_RETRY(new_fd, ::FMT_POSIX(dup(fd)));
  if (new_fd == -1)
    fmt::ThrowSystemError(errno, "cannot duplicate file descriptor {}") << fd;
  return File(new_fd);
}

void File::dup2(int fd) {
  int result = 0;
  FMT_RETRY(result, ::FMT_POSIX(dup2(fd_, fd)));
  if (result == -1) {
    fmt::ThrowSystemError(errno,
        "cannot duplicate file descriptor {} to {}") << fd_ << fd;
  }
}

void File::dup2(int fd, ErrorCode &ec) FMT_NOEXCEPT(true) {
  int result = 0;
  FMT_RETRY(result, ::FMT_POSIX(dup2(fd_, fd)));
  if (result == -1)
    ec = ErrorCode(errno);
}

void File::pipe(File &read_end, File &write_end) {
  // Close the descriptors first to make sure that assignments don't throw
  // and there are no leaks.
  read_end.close();
  write_end.close();
  int fds[2] = {};
#ifdef _WIN32
  // Make the default pipe capacity same as on Linux 2.6.11+.
  enum { DEFAULT_CAPACITY = 65536 };
  int result = _pipe(fds, DEFAULT_CAPACITY, _O_BINARY);
#else
  // The pipe function doesn't return EINTR, so no need to retry.
  int result = ::pipe(fds);
#endif
  if (result != 0)
    fmt::ThrowSystemError(errno, "cannot create pipe");
  // The following assignments don't throw because read_fd and write_fd
  // are closed.
  read_end = File(fds[0]);
  write_end = File(fds[1]);
}

BufferedFile File::fdopen(const char *mode) {
  BufferedFile f(::FMT_POSIX(fdopen(fd_, mode)));
  fd_ = -1;
  return f;
}

void OutputRedirect::Flush() {
#if EOF != -1
# error "FMT_RETRY assumes return value of -1 indicating failure"
#endif
  int result = 0;
  FMT_RETRY(result, fflush(file_));
  if (result != 0)
    fmt::ThrowSystemError(errno, "cannot flush stream");
}

void OutputRedirect::Restore() {
  if (original_.descriptor() == -1)
    return;  // Already restored.
  Flush();
  // Restore the original file.
  original_.dup2(FMT_POSIX(fileno(file_)));
  original_.close();
}

OutputRedirect::OutputRedirect(std::FILE *file) : file_(file) {
  Flush();
  int fd = FMT_POSIX(fileno(file));
  // Create a File object referring to the original file.
  original_ = File::dup(fd);
  // Create a pipe.
  File write_end;
  File::pipe(read_end_, write_end);
  // Connect the passed FILE object to the write end of the pipe.
  write_end.dup2(fd);
}

OutputRedirect::~OutputRedirect() FMT_NOEXCEPT(true) {
  try {
    Restore();
  } catch (const std::exception &e) {
    std::fputs(e.what(), stderr);
  }
}

std::string OutputRedirect::RestoreAndRead() {
  // Restore output.
  Restore();

  // Read everything from the pipe.
  std::string content;
  if (read_end_.descriptor() == -1)
    return content;  // Already read.
  enum { BUFFER_SIZE = 4096 };
  char buffer[BUFFER_SIZE];
  std::streamsize count = 0;
  do {
    count = read_end_.read(buffer, BUFFER_SIZE);
    content.append(buffer, static_cast<std::size_t>(count));
  } while (count != 0);
  read_end_.close();
  return content;
}

#endif  // FMT_USE_FILE_DESCRIPTORS

std::string FormatSystemErrorMessage(int error_code, fmt::StringRef message) {
  fmt::Writer out;
  fmt::internal::FormatSystemErrorMessage(out, error_code, message);
  return str(out);
}
