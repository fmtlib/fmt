/*
 Tests of the C++ interface to POSIX functions

 Copyright (c) 2015, Victor Zverovich
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

#include <cstdlib>  // std::exit
#include <cstring>

#include "fmt/posix.h"
#include "gtest-extra.h"
#include "util.h"

#ifdef fileno
# undef fileno
#endif

using fmt::BufferedFile;
using fmt::ErrorCode;
using fmt::File;

using testing::internal::scoped_ptr;

// Checks if the file is open by reading one character from it.
bool isopen(int fd) {
  char buffer;
  return FMT_POSIX(read(fd, &buffer, 1)) == 1;
}

bool isclosed(int fd) {
  char buffer;
  std::streamsize result = 0;
  SUPPRESS_ASSERT(result = FMT_POSIX(read(fd, &buffer, 1)));
  return result == -1 && errno == EBADF;
}

// Opens a file for reading.
File open_file() {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  write_end.write(FILE_CONTENT, std::strlen(FILE_CONTENT));
  write_end.close();
  return read_end;
}

// Attempts to write a string to a file.
void write(File &f, fmt::StringRef s) {
  std::size_t num_chars_left = s.size();
  const char *ptr = s.data();
  do {
    std::size_t count = f.write(ptr, num_chars_left);
    ptr += count;
    // We can't write more than size_t bytes since num_chars_left
    // has type size_t.
    num_chars_left -= static_cast<std::size_t>(count);
  } while (num_chars_left != 0);
}

TEST(BufferedFileTest, DefaultCtor) {
  BufferedFile f;
  EXPECT_TRUE(f.get() == 0);
}

TEST(BufferedFileTest, MoveCtor) {
  BufferedFile bf = open_buffered_file();
  FILE *fp = bf.get();
  EXPECT_TRUE(fp != 0);
  BufferedFile bf2(std::move(bf));
  EXPECT_EQ(fp, bf2.get());
  EXPECT_TRUE(bf.get() == 0);
}

TEST(BufferedFileTest, MoveAssignment) {
  BufferedFile bf = open_buffered_file();
  FILE *fp = bf.get();
  EXPECT_TRUE(fp != 0);
  BufferedFile bf2;
  bf2 = std::move(bf);
  EXPECT_EQ(fp, bf2.get());
  EXPECT_TRUE(bf.get() == 0);
}

TEST(BufferedFileTest, MoveAssignmentClosesFile) {
  BufferedFile bf = open_buffered_file();
  BufferedFile bf2 = open_buffered_file();
  int old_fd = bf2.fileno();
  bf2 = std::move(bf);
  EXPECT_TRUE(isclosed(old_fd));
}

TEST(BufferedFileTest, MoveFromTemporaryInCtor) {
  FILE *fp = 0;
  BufferedFile f(open_buffered_file(&fp));
  EXPECT_EQ(fp, f.get());
}

TEST(BufferedFileTest, MoveFromTemporaryInAssignment) {
  FILE *fp = 0;
  BufferedFile f;
  f = open_buffered_file(&fp);
  EXPECT_EQ(fp, f.get());
}

TEST(BufferedFileTest, MoveFromTemporaryInAssignmentClosesFile) {
  BufferedFile f = open_buffered_file();
  int old_fd = f.fileno();
  f = open_buffered_file();
  EXPECT_TRUE(isclosed(old_fd));
}

TEST(BufferedFileTest, CloseFileInDtor) {
  int fd = 0;
  {
    BufferedFile f = open_buffered_file();
    fd = f.fileno();
  }
  EXPECT_TRUE(isclosed(fd));
}

TEST(BufferedFileTest, CloseErrorInDtor) {
  scoped_ptr<BufferedFile> f(new BufferedFile(open_buffered_file()));
  EXPECT_WRITE(stderr, {
      // The close function must be called inside EXPECT_WRITE, otherwise
      // the system may recycle closed file descriptor when redirecting the
      // output in EXPECT_STDERR and the second close will break output
      // redirection.
      FMT_POSIX(close(f->fileno()));
      SUPPRESS_ASSERT(f.reset());
  }, format_system_error(EBADF, "cannot close file") + "\n");
}

TEST(BufferedFileTest, Close) {
  BufferedFile f = open_buffered_file();
  int fd = f.fileno();
  f.close();
  EXPECT_TRUE(f.get() == 0);
  EXPECT_TRUE(isclosed(fd));
}

TEST(BufferedFileTest, CloseError) {
  BufferedFile f = open_buffered_file();
  FMT_POSIX(close(f.fileno()));
  EXPECT_SYSTEM_ERROR_NOASSERT(f.close(), EBADF, "cannot close file");
  EXPECT_TRUE(f.get() == 0);
}

TEST(BufferedFileTest, Fileno) {
  BufferedFile f;
#ifndef __COVERITY__
  // fileno on a null FILE pointer either crashes or returns an error.
  // Disable Coverity because this is intentional.
  EXPECT_DEATH_IF_SUPPORTED({
    try {
      f.fileno();
    } catch (fmt::SystemError) {
      std::exit(1);
    }
  }, "");
#endif
  f = open_buffered_file();
  EXPECT_TRUE(f.fileno() != -1);
  File copy = File::dup(f.fileno());
  EXPECT_READ(copy, FILE_CONTENT);
}

TEST(FileTest, DefaultCtor) {
  File f;
  EXPECT_EQ(-1, f.descriptor());
}

TEST(FileTest, OpenBufferedFileInCtor) {
  FILE *fp = safe_fopen("test-file", "w");
  std::fputs(FILE_CONTENT, fp);
  std::fclose(fp);
  File f("test-file", File::RDONLY);
  ASSERT_TRUE(isopen(f.descriptor()));
}

TEST(FileTest, OpenBufferedFileError) {
  EXPECT_SYSTEM_ERROR(File("nonexistent", File::RDONLY),
      ENOENT, "cannot open file nonexistent");
}

TEST(FileTest, MoveCtor) {
  File f = open_file();
  int fd = f.descriptor();
  EXPECT_NE(-1, fd);
  File f2(std::move(f));
  EXPECT_EQ(fd, f2.descriptor());
  EXPECT_EQ(-1, f.descriptor());
}

TEST(FileTest, MoveAssignment) {
  File f = open_file();
  int fd = f.descriptor();
  EXPECT_NE(-1, fd);
  File f2;
  f2 = std::move(f);
  EXPECT_EQ(fd, f2.descriptor());
  EXPECT_EQ(-1, f.descriptor());
}

TEST(FileTest, MoveAssignmentClosesFile) {
  File f = open_file();
  File f2 = open_file();
  int old_fd = f2.descriptor();
  f2 = std::move(f);
  EXPECT_TRUE(isclosed(old_fd));
}

File OpenBufferedFile(int &fd) {
  File f = open_file();
  fd = f.descriptor();
  return f;
}

TEST(FileTest, MoveFromTemporaryInCtor) {
  int fd = 0xdead;
  File f(OpenBufferedFile(fd));
  EXPECT_EQ(fd, f.descriptor());
}

TEST(FileTest, MoveFromTemporaryInAssignment) {
  int fd = 0xdead;
  File f;
  f = OpenBufferedFile(fd);
  EXPECT_EQ(fd, f.descriptor());
}

TEST(FileTest, MoveFromTemporaryInAssignmentClosesFile) {
  int fd = 0xdead;
  File f = open_file();
  int old_fd = f.descriptor();
  f = OpenBufferedFile(fd);
  EXPECT_TRUE(isclosed(old_fd));
}

TEST(FileTest, CloseFileInDtor) {
  int fd = 0;
  {
    File f = open_file();
    fd = f.descriptor();
  }
  EXPECT_TRUE(isclosed(fd));
}

TEST(FileTest, CloseErrorInDtor) {
  scoped_ptr<File> f(new File(open_file()));
  EXPECT_WRITE(stderr, {
      // The close function must be called inside EXPECT_WRITE, otherwise
      // the system may recycle closed file descriptor when redirecting the
      // output in EXPECT_STDERR and the second close will break output
      // redirection.
      FMT_POSIX(close(f->descriptor()));
      SUPPRESS_ASSERT(f.reset());
  }, format_system_error(EBADF, "cannot close file") + "\n");
}

TEST(FileTest, Close) {
  File f = open_file();
  int fd = f.descriptor();
  f.close();
  EXPECT_EQ(-1, f.descriptor());
  EXPECT_TRUE(isclosed(fd));
}

TEST(FileTest, CloseError) {
  File f = open_file();
  FMT_POSIX(close(f.descriptor()));
  EXPECT_SYSTEM_ERROR_NOASSERT(f.close(), EBADF, "cannot close file");
  EXPECT_EQ(-1, f.descriptor());
}

TEST(FileTest, Read) {
  File f = open_file();
  EXPECT_READ(f, FILE_CONTENT);
}

TEST(FileTest, ReadError) {
  File f("test-file", File::WRONLY);
  char buf;
  // We intentionally read from a file opened in the write-only mode to
  // cause error.
  EXPECT_SYSTEM_ERROR(f.read(&buf, 1), EBADF, "cannot read from file");
}

TEST(FileTest, Write) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  write(write_end, "test");
  write_end.close();
  EXPECT_READ(read_end, "test");
}

TEST(FileTest, WriteError) {
  File f("test-file", File::RDONLY);
  // We intentionally write to a file opened in the read-only mode to
  // cause error.
  EXPECT_SYSTEM_ERROR(f.write(" ", 1), EBADF, "cannot write to file");
}

TEST(FileTest, Dup) {
  File f = open_file();
  File copy = File::dup(f.descriptor());
  EXPECT_NE(f.descriptor(), copy.descriptor());
  EXPECT_EQ(FILE_CONTENT, read(copy, std::strlen(FILE_CONTENT)));
}

#ifndef __COVERITY__
TEST(FileTest, DupError) {
  int value = -1;
  EXPECT_SYSTEM_ERROR_NOASSERT(File::dup(value),
      EBADF, "cannot duplicate file descriptor -1");
}
#endif

TEST(FileTest, Dup2) {
  File f = open_file();
  File copy = open_file();
  f.dup2(copy.descriptor());
  EXPECT_NE(f.descriptor(), copy.descriptor());
  EXPECT_READ(copy, FILE_CONTENT);
}

TEST(FileTest, Dup2Error) {
  File f = open_file();
  EXPECT_SYSTEM_ERROR_NOASSERT(f.dup2(-1), EBADF,
    fmt::format("cannot duplicate file descriptor {} to -1", f.descriptor()));
}

TEST(FileTest, Dup2NoExcept) {
  File f = open_file();
  File copy = open_file();
  ErrorCode ec;
  f.dup2(copy.descriptor(), ec);
  EXPECT_EQ(0, ec.get());
  EXPECT_NE(f.descriptor(), copy.descriptor());
  EXPECT_READ(copy, FILE_CONTENT);
}

TEST(FileTest, Dup2NoExceptError) {
  File f = open_file();
  ErrorCode ec;
  SUPPRESS_ASSERT(f.dup2(-1, ec));
  EXPECT_EQ(EBADF, ec.get());
}

TEST(FileTest, Pipe) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  EXPECT_NE(-1, read_end.descriptor());
  EXPECT_NE(-1, write_end.descriptor());
  write(write_end, "test");
  EXPECT_READ(read_end, "test");
}

TEST(FileTest, Fdopen) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  int read_fd = read_end.descriptor();
  EXPECT_EQ(read_fd, FMT_POSIX(fileno(read_end.fdopen("r").get())));
}

TEST(FileTest, FdopenError) {
  File f;
  EXPECT_SYSTEM_ERROR_NOASSERT(
      f.fdopen("r"), EBADF, "cannot associate stream with file descriptor");
}

#ifdef FMT_LOCALE
TEST(LocaleTest, Strtod) {
  fmt::Locale locale;
  const char *start = "4.2", *ptr = start;
  EXPECT_EQ(4.2, locale.strtod(ptr));
  EXPECT_EQ(start + 3, ptr);
}
#endif
