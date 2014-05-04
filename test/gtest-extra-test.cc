/*
 Tests of custom Google Test assertions.

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

#include <cstdio>
#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <gtest/gtest-spi.h>

namespace {

std::string FormatSystemErrorMessage(int error_code, fmt::StringRef message) {
  fmt::Writer out;
  fmt::internal::FormatSystemErrorMessage(out, error_code, message);
  return str(out);
}

#define EXPECT_SYSTEM_ERROR(statement, error_code, message) \
  EXPECT_THROW_MSG(statement, fmt::SystemError, \
      FormatSystemErrorMessage(error_code, message))

// Checks if the file is open by reading one character from it.
bool IsOpen(int fd) {
  char buffer;
  return FMT_POSIX(read(fd, &buffer, 1)) == 1;
}

bool IsClosedInternal(int fd) {
  char buffer;
  std::streamsize result = FMT_POSIX(read(fd, &buffer, 1));
  return result == -1 && errno == EBADF;
}

#ifndef _WIN32
// Checks if the file is closed.
# define EXPECT_CLOSED(fd) EXPECT_TRUE(IsClosedInternal(fd))
#else
// Reading from a closed file causes death on Windows.
# define EXPECT_CLOSED(fd) EXPECT_DEATH(IsClosedInternal(fd), "")
#endif

#ifndef _WIN32
# define EXPECT_SYSTEM_ERROR_OR_DEATH(statement, error_code, message) \
    EXPECT_SYSTEM_ERROR(statement, error_code, message)
#else
# define EXPECT_SYSTEM_ERROR_OR_DEATH(statement, error_code, message) \
    EXPECT_DEATH(statement, "")
#endif

// Tests that assertion macros evaluate their arguments exactly once.
class SingleEvaluationTest : public ::testing::Test {
 protected:
  SingleEvaluationTest() {
    a_ = 0;
  }
  
  static int a_;
};

int SingleEvaluationTest::a_;

void ThrowNothing() {}

void ThrowException() {
  throw std::runtime_error("test");
}

// Tests that assertion arguments are evaluated exactly once.
TEST_F(SingleEvaluationTest, ExceptionTests) {
  // successful EXPECT_THROW_MSG
  EXPECT_THROW_MSG({  // NOLINT
    a_++;
    ThrowException();
  }, std::exception, "test");
  EXPECT_EQ(1, a_);

  // failed EXPECT_THROW_MSG, throws different type
  EXPECT_NONFATAL_FAILURE(EXPECT_THROW_MSG({  // NOLINT
    a_++;
    ThrowException();
  }, std::logic_error, "test"), "throws a different type");
  EXPECT_EQ(2, a_);

  // failed EXPECT_THROW_MSG, throws an exception with different message
  EXPECT_NONFATAL_FAILURE(EXPECT_THROW_MSG({  // NOLINT
    a_++;
    ThrowException();
  }, std::exception, "other"), "throws an exception with a different message");
  EXPECT_EQ(3, a_);

  // failed EXPECT_THROW_MSG, throws nothing
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THROW_MSG(a_++, std::exception, "test"), "throws nothing");
  EXPECT_EQ(4, a_);
}

// Tests that the compiler will not complain about unreachable code in the
// EXPECT_THROW_MSG macro.
TEST(ExpectThrowTest, DoesNotGenerateUnreachableCodeWarning) {
  int n = 0;
  using std::runtime_error;
  EXPECT_THROW_MSG(throw runtime_error(""), runtime_error, "");
  EXPECT_NONFATAL_FAILURE(EXPECT_THROW_MSG(n++, runtime_error, ""), "");
  EXPECT_NONFATAL_FAILURE(EXPECT_THROW_MSG(throw 1, runtime_error, ""), "");
  EXPECT_NONFATAL_FAILURE(EXPECT_THROW_MSG(
      throw runtime_error("a"), runtime_error, "b"), "");
}

TEST(AssertionSyntaxTest, ExceptionAssertionsBehavesLikeSingleStatement) {
  if (::testing::internal::AlwaysFalse())
    EXPECT_THROW_MSG(ThrowNothing(), std::exception, "");

  if (::testing::internal::AlwaysTrue())
    EXPECT_THROW_MSG(ThrowException(), std::exception, "test");
  else
    ;  // NOLINT
}

// Tests EXPECT_THROW_MSG.
TEST(ExpectTest, EXPECT_THROW_MSG) {
  EXPECT_THROW_MSG(ThrowException(), std::exception, "test");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THROW_MSG(ThrowException(), std::logic_error, "test"),
      "Expected: ThrowException() throws an exception of "
      "type std::logic_error.\n  Actual: it throws a different type.");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THROW_MSG(ThrowNothing(), std::exception, "test"),
      "Expected: ThrowNothing() throws an exception of type std::exception.\n"
      "  Actual: it throws nothing.");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THROW_MSG(ThrowException(), std::exception, "other"),
      "ThrowException() throws an exception with a different message.\n"
      "Expected: other\n"
      "  Actual: test");
}

TEST(StreamingAssertionsTest, ThrowMsg) {
  EXPECT_THROW_MSG(ThrowException(), std::exception, "test")
      << "unexpected failure";
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THROW_MSG(ThrowException(), std::exception, "other")
      << "expected failure", "expected failure");
}

#if FMT_USE_FILE_DESCRIPTORS

TEST(ErrorCodeTest, Ctor) {
  EXPECT_EQ(0, ErrorCode().get());
  EXPECT_EQ(42, ErrorCode(42).get());
}

TEST(BufferedFileTest, DefaultCtor) {
  BufferedFile f;
  EXPECT_TRUE(f.get() == 0);
}

BufferedFile OpenFile(const char *name, FILE **fp = 0) {
  BufferedFile f = File(".travis.yml", File::RDONLY).fdopen("r");
  if (fp)
    *fp = f.get();
  return f;
}

TEST(BufferedFileTest, MoveCtor) {
  BufferedFile bf = OpenFile(".travis.yml");
  FILE *fp = bf.get();
  EXPECT_TRUE(fp != 0);
  BufferedFile bf2(std::move(bf));
  EXPECT_EQ(fp, bf2.get());
  EXPECT_TRUE(bf.get() == 0);
}

TEST(BufferedFileTest, MoveAssignment) {
  BufferedFile bf = OpenFile(".travis.yml");
  FILE *fp = bf.get();
  EXPECT_TRUE(fp != 0);
  BufferedFile bf2;
  bf2 = std::move(bf);
  EXPECT_EQ(fp, bf2.get());
  EXPECT_TRUE(bf.get() == 0);
}

TEST(BufferedFileTest, MoveAssignmentClosesFile) {
  BufferedFile bf = OpenFile(".travis.yml");
  BufferedFile bf2 = OpenFile("CMakeLists.txt");
  int old_fd = fileno(bf2.get());
  bf2 = std::move(bf);
  EXPECT_CLOSED(old_fd);
}

TEST(BufferedFileTest, MoveFromTemporaryInCtor) {
  FILE *fp = 0;
  BufferedFile f(OpenFile(".travis.yml", &fp));
  EXPECT_EQ(fp, f.get());
}

TEST(BufferedFileTest, MoveFromTemporaryInAssignment) {
  FILE *fp = 0;
  BufferedFile f;
  f = OpenFile(".travis.yml", &fp);
  EXPECT_EQ(fp, f.get());
}

TEST(BufferedFileTest, MoveFromTemporaryInAssignmentClosesFile) {
  BufferedFile f = OpenFile(".travis.yml");
  int old_fd = fileno(f.get());
  f = OpenFile(".travis.yml");
  EXPECT_CLOSED(old_fd);
}

TEST(BufferedFileTest, CloseFileInDtor) {
  int fd = 0;
  {
    BufferedFile f = OpenFile(".travis.yml");
    fd = fileno(f.get());
  }
  EXPECT_CLOSED(fd);
}

TEST(BufferedFileTest, CloseErrorInDtor) {
  BufferedFile *f = new BufferedFile(OpenFile(".travis.yml"));
#ifndef _WIN32
  // The close function must be called inside EXPECT_STDERR, otherwise
  // the system may recycle closed file descriptor when redirecting the
  // output in EXPECT_STDERR and the second close will break output
  // redirection.
  EXPECT_STDERR(close(fileno(f->get())); delete f,
    FormatSystemErrorMessage(EBADF, "cannot close file") + "\n");
#else
  FMT_POSIX(close(fileno(f->get())));
  // Closing file twice causes death on Windows.
  EXPECT_DEATH(delete f, "");
#endif
}

TEST(FileTest, DefaultCtor) {
  File f;
  EXPECT_EQ(-1, f.descriptor());
}

TEST(FileTest, OpenFileInCtor) {
  File f(".travis.yml", File::RDONLY);
  ASSERT_TRUE(IsOpen(f.descriptor()));
}

TEST(FileTest, OpenFileError) {
  EXPECT_SYSTEM_ERROR(File("nonexistent", File::RDONLY),
      ENOENT, "cannot open file nonexistent");
}

TEST(FileTest, MoveCtor) {
  File f(".travis.yml", File::RDONLY);
  int fd = f.descriptor();
  EXPECT_NE(-1, fd);
  File f2(std::move(f));
  EXPECT_EQ(fd, f2.descriptor());
  EXPECT_EQ(-1, f.descriptor());
}

TEST(FileTest, MoveAssignment) {
  File f(".travis.yml", File::RDONLY);
  int fd = f.descriptor();
  EXPECT_NE(-1, fd);
  File f2;
  f2 = std::move(f);
  EXPECT_EQ(fd, f2.descriptor());
  EXPECT_EQ(-1, f.descriptor());
}

TEST(FileTest, MoveAssignmentClosesFile) {
  File f(".travis.yml", File::RDONLY);
  File f2("CMakeLists.txt", File::RDONLY);
  int old_fd = f2.descriptor();
  f2 = std::move(f);
  EXPECT_CLOSED(old_fd);
}

File OpenFile(int &fd) {
  File f(".travis.yml", File::RDONLY);
  fd = f.descriptor();
  return std::move(f);
}

TEST(FileTest, MoveFromTemporaryInCtor) {
  int fd = 0xdeadbeef;
  File f(OpenFile(fd));
  EXPECT_EQ(fd, f.descriptor());
}

TEST(FileTest, MoveFromTemporaryInAssignment) {
  int fd = 0xdeadbeef;
  File f;
  f = OpenFile(fd);
  EXPECT_EQ(fd, f.descriptor());
}

TEST(FileTest, MoveFromTemporaryInAssignmentClosesFile) {
  int fd = 0xdeadbeef;
  File f(".travis.yml", File::RDONLY);
  int old_fd = f.descriptor();
  f = OpenFile(fd);
  EXPECT_CLOSED(old_fd);
}

TEST(FileTest, CloseFileInDtor) {
  int fd = 0;
  {
    File f(".travis.yml", File::RDONLY);
    fd = f.descriptor();
  }
  EXPECT_CLOSED(fd);
}

TEST(FileTest, CloseErrorInDtor) {
  File *f = new File(".travis.yml", File::RDONLY);
#ifndef _WIN32
  // The close function must be called inside EXPECT_STDERR, otherwise
  // the system may recycle closed file descriptor when redirecting the
  // output in EXPECT_STDERR and the second close will break output
  // redirection.
  EXPECT_STDERR(FMT_POSIX(close(f->descriptor())); delete f,
    FormatSystemErrorMessage(EBADF, "cannot close file") + "\n");
#else
  close(f->descriptor());
  // Closing file twice causes death on Windows.
  EXPECT_DEATH(delete f, "");
#endif
}

TEST(FileTest, Close) {
  File f(".travis.yml", File::RDONLY);
  int fd = f.descriptor();
  f.close();
  EXPECT_EQ(-1, f.descriptor());
  EXPECT_CLOSED(fd);
}

TEST(FileTest, CloseError) {
  File f(".travis.yml", File::RDONLY);
#ifndef _WIN32
  // The close function must be called inside EXPECT_STDERR, otherwise
  // the system may recycle closed file descriptor when redirecting the
  // output in EXPECT_STDERR and the second close will break output
  // redirection.
  close(f.descriptor());
  EXPECT_SYSTEM_ERROR(f.close(), EBADF, "cannot close file");
  EXPECT_EQ(-1, f.descriptor());
#else
  File dup = f.dup(f.descriptor());
  close(f.descriptor());
  // Closing file twice causes death on Windows.
  EXPECT_DEATH(f.close(), "");
  dup.dup2(f.descriptor());  // "undo" close or dtor will fail
#endif
}

// Attempts to read count characters from a file.
std::string Read(File &f, std::size_t count) {
  std::string buffer(count, '\0');
  std::streamsize offset = 0, n = 0;
  do {
    n = f.read(&buffer[offset], count - offset);
    offset += n;
  } while (offset < count && n != 0);
  buffer.resize(offset);
  return buffer;
}

// Attempts to write a string to a file.
void Write(File &f, fmt::StringRef s) {
  std::size_t num_chars_left = s.size();
  const char *ptr = s.c_str();
  do {
    std::streamsize count = f.write(ptr, num_chars_left);
    ptr += count;
    num_chars_left -= count;
  } while (num_chars_left != 0);
}

#define EXPECT_READ(file, expected_content) \
  EXPECT_EQ(expected_content, Read(file, std::strlen(expected_content)))

TEST(FileTest, Read) {
  File f(".travis.yml", File::RDONLY);
  EXPECT_READ(f, "language: cpp");
}

TEST(FileTest, ReadError) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  char buf;
  // We intentionally read from write_end to cause error.
  EXPECT_SYSTEM_ERROR(write_end.read(&buf, 1), EBADF, "cannot read from file");
}

TEST(FileTest, Write) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  Write(write_end, "test");
  write_end.close();
  EXPECT_READ(read_end, "test");
}

TEST(FileTest, WriteError) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  // We intentionally write to read_end to cause error.
  EXPECT_SYSTEM_ERROR(read_end.write(" ", 1), EBADF, "cannot write to file");
}

TEST(FileTest, Dup) {
  File f(".travis.yml", File::RDONLY);
  File dup = File::dup(f.descriptor());
  EXPECT_NE(f.descriptor(), dup.descriptor());
  const char EXPECTED[] = "language: cpp";
  EXPECT_EQ(EXPECTED, Read(dup, sizeof(EXPECTED) - 1));
}

TEST(FileTest, DupError) {
  EXPECT_SYSTEM_ERROR_OR_DEATH(File::dup(-1),
      EBADF, "cannot duplicate file descriptor -1");
}

TEST(FileTest, Dup2) {
  File f(".travis.yml", File::RDONLY);
  File dup("CMakeLists.txt", File::RDONLY);
  f.dup2(dup.descriptor());
  EXPECT_NE(f.descriptor(), dup.descriptor());
  EXPECT_READ(dup, "language: cpp");
}

TEST(FileTest, Dup2Error) {
  File f(".travis.yml", File::RDONLY);
  EXPECT_SYSTEM_ERROR_OR_DEATH(f.dup2(-1), EBADF,
    fmt::Format("cannot duplicate file descriptor {} to -1") << f.descriptor());
}

TEST(FileTest, Dup2NoExcept) {
  File f(".travis.yml", File::RDONLY);
  File dup("CMakeLists.txt", File::RDONLY);
  ErrorCode ec;
  f.dup2(dup.descriptor(), ec);
  EXPECT_EQ(0, ec.get());
  EXPECT_NE(f.descriptor(), dup.descriptor());
  EXPECT_READ(dup, "language: cpp");
}

TEST(FileTest, Dup2NoExceptError) {
  File f(".travis.yml", File::RDONLY);
  ErrorCode ec;
#ifndef _WIN32
  f.dup2(-1, ec);
  EXPECT_EQ(EBADF, ec.get());
#else
  EXPECT_DEATH(f.dup2(-1, ec), "");
#endif
}

TEST(FileTest, Pipe) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  EXPECT_NE(-1, read_end.descriptor());
  EXPECT_NE(-1, write_end.descriptor());
  Write(write_end, "test");
  EXPECT_READ(read_end, "test");
}

TEST(OutputRedirectTest, ScopedRedirect) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  {
    BufferedFile file(write_end.fdopen("w"));
    std::fprintf(file.get(), "[[[");
    {
      OutputRedirect redir(file.get());
      std::fprintf(file.get(), "censored");
    }
    std::fprintf(file.get(), "]]]");
  }
  EXPECT_READ(read_end, "[[[]]]");
}

// Test that OutputRedirect handles errors in flush correctly.
TEST(OutputRedirectTest, FlushErrorInCtor) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  int write_fd = write_end.descriptor();
  File write_dup = write_end.dup(write_fd);
  BufferedFile f = write_end.fdopen("w");
  // Put a character in a file buffer.
  EXPECT_EQ('x', fputc('x', f.get()));
  close(write_fd);
  OutputRedirect *redir = 0;
  EXPECT_SYSTEM_ERROR_OR_DEATH(redir = new OutputRedirect(f.get()),
      EBADF, fmt::Format("cannot flush stream"));
  delete redir;
  write_dup.dup2(write_fd);  // "undo" close or dtor will fail
}

TEST(OutputRedirectTest, DupErrorInCtor) {
  BufferedFile f = OpenFile(".travis.yml");
  int fd = fileno(f.get());
  File dup = File::dup(fd);
  close(fd);
  OutputRedirect *redir = 0;
  EXPECT_SYSTEM_ERROR_OR_DEATH(redir = new OutputRedirect(f.get()),
      EBADF, fmt::Format("cannot duplicate file descriptor {}") << fd);
  dup.dup2(fd);  // "undo" close or dtor will fail
  delete redir;
}

TEST(OutputRedirectTest, RestoreAndRead) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  BufferedFile file(write_end.fdopen("w"));
  std::fprintf(file.get(), "[[[");
  OutputRedirect redir(file.get());
  std::fprintf(file.get(), "censored");
  EXPECT_EQ("censored", redir.RestoreAndRead());
  std::fprintf(file.get(), "]]]");
  file = BufferedFile();
  EXPECT_READ(read_end, "[[[]]]");
}

// Test that OutputRedirect handles errors in flush correctly.
TEST(OutputRedirectTest, FlushErrorInRestoreAndRead) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  int write_fd = write_end.descriptor();
  File write_dup = write_end.dup(write_fd);
  BufferedFile f = write_end.fdopen("w");
  OutputRedirect redir(f.get());
  // Put a character in a file buffer.
  EXPECT_EQ('x', fputc('x', f.get()));
  close(write_fd);
  EXPECT_SYSTEM_ERROR_OR_DEATH(redir.RestoreAndRead(),
      EBADF, fmt::Format("cannot flush stream"));
  write_dup.dup2(write_fd);  // "undo" close or dtor will fail
}

TEST(OutputRedirectTest, ErrorInDtor) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  int write_fd = write_end.descriptor();
  File write_dup = write_end.dup(write_fd);
  BufferedFile f = write_end.fdopen("w");
  OutputRedirect *redir = new OutputRedirect(f.get());
  // Put a character in a file buffer.
  EXPECT_EQ('x', fputc('x', f.get()));
  // The close function must be called inside EXPECT_STDERR, otherwise
  // the system may recycle closed file descriptor when redirecting the
  // output in EXPECT_STDERR and the second close will break output
  // redirection.
  EXPECT_STDERR(close(write_fd); delete redir,
      FormatSystemErrorMessage(EBADF, "cannot flush stream"));
  write_dup.dup2(write_fd); // "undo" close or dtor of BufferedFile will fail
}

// TODO: test calling RestoreAndRead multiple times

// TODO: test EXPECT_STDOUT and EXPECT_STDERR

// TODO: compile both with C++11 & C++98 mode
#endif

}  // namespace
