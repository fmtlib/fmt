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

#include <cstring>
#include <algorithm>
#include <stdexcept>
#include <gtest/gtest-spi.h>

#if defined(_WIN32) && !defined(__MINGW32__)
# include <crtdbg.h>  // for _CrtSetReportMode
#endif  // _WIN32

namespace {

#if defined(_WIN32) && !defined(__MINGW32__)

// Suppresses Windows assertions on invalid file descriptors, making
// POSIX functions return proper error codes instead of crashing on Windows.
class SuppressAssert {
 private:
  _invalid_parameter_handler original_handler_;
  int original_report_mode_;

  static void InvalidParameterHandler(const wchar_t *,
      const wchar_t *, const wchar_t *, unsigned , uintptr_t) {}

 public:
  SuppressAssert()
  : original_handler_(_set_invalid_parameter_handler(InvalidParameterHandler)),
    original_report_mode_(_CrtSetReportMode(_CRT_ASSERT, 0)) {
  }
  ~SuppressAssert() {
    _set_invalid_parameter_handler(original_handler_);
    _CrtSetReportMode(_CRT_ASSERT, original_report_mode_);
  }
};

# define SUPPRESS_ASSERT(statement) { SuppressAssert sa; statement; }

// Fix "secure" warning about using fopen without defining
// _CRT_SECURE_NO_WARNINGS.
FILE *OpenFile(const char *filename, const char *mode) {
  FILE *f = 0;
  errno = fopen_s(&f, filename, mode);
  return f;
}
#define fopen OpenFile
#else
# define SUPPRESS_ASSERT(statement) statement
using std::fopen;
#endif  // _WIN32

#define EXPECT_SYSTEM_ERROR_NOASSERT(statement, error_code, message) \
  EXPECT_SYSTEM_ERROR(SUPPRESS_ASSERT(statement), error_code, message)

// Tests that assertion macros evaluate their arguments exactly once.
class SingleEvaluationTest : public ::testing::Test {
 protected:
  SingleEvaluationTest() {
    p_ = s_;
    a_ = 0;
    b_ = 0;
  }

  static const char* const s_;
  static const char* p_;

  static int a_;
  static int b_;
};

const char* const SingleEvaluationTest::s_ = "01234";
const char* SingleEvaluationTest::p_;
int SingleEvaluationTest::a_;
int SingleEvaluationTest::b_;

void DoNothing() {}

void ThrowException() {
  throw std::runtime_error("test");
}

void ThrowSystemError() {
  throw fmt::SystemError(EDOM, "test");
}

// Tests that when EXPECT_THROW_MSG fails, it evaluates its message argument
// exactly once.
TEST_F(SingleEvaluationTest, FailedEXPECT_THROW_MSG) {
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THROW_MSG(ThrowException(), std::exception, p_++), "01234");
  EXPECT_EQ(s_ + 1, p_);
}

// Tests that when EXPECT_SYSTEM_ERROR fails, it evaluates its message argument
// exactly once.
TEST_F(SingleEvaluationTest, FailedEXPECT_SYSTEM_ERROR) {
  EXPECT_NONFATAL_FAILURE(
      EXPECT_SYSTEM_ERROR(ThrowSystemError(), EDOM, p_++), "01234");
  EXPECT_EQ(s_ + 1, p_);
}

// Tests that when EXPECT_WRITE fails, it evaluates its message argument
// exactly once.
TEST_F(SingleEvaluationTest, FailedEXPECT_WRITE) {
  EXPECT_NONFATAL_FAILURE(
      EXPECT_WRITE(stdout, std::printf("test"), p_++), "01234");
  EXPECT_EQ(s_ + 1, p_);
}

// Tests that assertion arguments are evaluated exactly once.
TEST_F(SingleEvaluationTest, ExceptionTests) {
  // successful EXPECT_THROW_MSG
  EXPECT_THROW_MSG({  // NOLINT
    a_++;
    ThrowException();
  }, std::exception, (b_++, "test"));
  EXPECT_EQ(1, a_);
  EXPECT_EQ(1, b_);

  // failed EXPECT_THROW_MSG, throws different type
  EXPECT_NONFATAL_FAILURE(EXPECT_THROW_MSG({  // NOLINT
    a_++;
    ThrowException();
  }, std::logic_error, (b_++, "test")), "throws a different type");
  EXPECT_EQ(2, a_);
  EXPECT_EQ(2, b_);

  // failed EXPECT_THROW_MSG, throws an exception with different message
  EXPECT_NONFATAL_FAILURE(EXPECT_THROW_MSG({  // NOLINT
    a_++;
    ThrowException();
  }, std::exception, (b_++, "other")),
      "throws an exception with a different message");
  EXPECT_EQ(3, a_);
  EXPECT_EQ(3, b_);

  // failed EXPECT_THROW_MSG, throws nothing
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THROW_MSG(a_++, std::exception, (b_++, "test")), "throws nothing");
  EXPECT_EQ(4, a_);
  EXPECT_EQ(4, b_);
}

TEST_F(SingleEvaluationTest, SystemErrorTests) {
  // successful EXPECT_SYSTEM_ERROR
  EXPECT_SYSTEM_ERROR({  // NOLINT
    a_++;
    ThrowSystemError();
  }, EDOM, (b_++, "test"));
  EXPECT_EQ(1, a_);
  EXPECT_EQ(1, b_);

  // failed EXPECT_SYSTEM_ERROR, throws different type
  EXPECT_NONFATAL_FAILURE(EXPECT_SYSTEM_ERROR({  // NOLINT
    a_++;
    ThrowException();
  }, EDOM, (b_++, "test")), "throws a different type");
  EXPECT_EQ(2, a_);
  EXPECT_EQ(2, b_);

  // failed EXPECT_SYSTEM_ERROR, throws an exception with different message
  EXPECT_NONFATAL_FAILURE(EXPECT_SYSTEM_ERROR({  // NOLINT
    a_++;
    ThrowSystemError();
  }, EDOM, (b_++, "other")),
      "throws an exception with a different message");
  EXPECT_EQ(3, a_);
  EXPECT_EQ(3, b_);

  // failed EXPECT_SYSTEM_ERROR, throws nothing
  EXPECT_NONFATAL_FAILURE(
      EXPECT_SYSTEM_ERROR(a_++, EDOM, (b_++, "test")), "throws nothing");
  EXPECT_EQ(4, a_);
  EXPECT_EQ(4, b_);
}

// Tests that assertion arguments are evaluated exactly once.
TEST_F(SingleEvaluationTest, WriteTests) {
  // successful EXPECT_WRITE
  EXPECT_WRITE(stdout, {  // NOLINT
    a_++;
    std::printf("test");
  }, (b_++, "test"));
  EXPECT_EQ(1, a_);
  EXPECT_EQ(1, b_);

  // failed EXPECT_WRITE
  EXPECT_NONFATAL_FAILURE(EXPECT_WRITE(stdout, {  // NOLINT
    a_++;
    std::printf("test");
  }, (b_++, "other")), "Actual: test");
  EXPECT_EQ(2, a_);
  EXPECT_EQ(2, b_);
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

// Tests that the compiler will not complain about unreachable code in the
// EXPECT_SYSTEM_ERROR macro.
TEST(ExpectSystemErrorTest, DoesNotGenerateUnreachableCodeWarning) {
  int n = 0;
  EXPECT_SYSTEM_ERROR(throw fmt::SystemError(EDOM, "test"), EDOM, "test");
  EXPECT_NONFATAL_FAILURE(EXPECT_SYSTEM_ERROR(n++, EDOM, ""), "");
  EXPECT_NONFATAL_FAILURE(EXPECT_SYSTEM_ERROR(throw 1, EDOM, ""), "");
  EXPECT_NONFATAL_FAILURE(EXPECT_SYSTEM_ERROR(
      throw fmt::SystemError(EDOM, "aaa"), EDOM, "bbb"), "");
}

TEST(AssertionSyntaxTest, ExceptionAssertionBehavesLikeSingleStatement) {
  if (::testing::internal::AlwaysFalse())
    EXPECT_THROW_MSG(DoNothing(), std::exception, "");

  if (::testing::internal::AlwaysTrue())
    EXPECT_THROW_MSG(ThrowException(), std::exception, "test");
  else
    DoNothing();
}

TEST(AssertionSyntaxTest, SystemErrorAssertionBehavesLikeSingleStatement) {
  if (::testing::internal::AlwaysFalse())
    EXPECT_SYSTEM_ERROR(DoNothing(), EDOM, "");

  if (::testing::internal::AlwaysTrue())
    EXPECT_SYSTEM_ERROR(ThrowSystemError(), EDOM, "test");
  else
    DoNothing();
}

TEST(AssertionSyntaxTest, WriteAssertionBehavesLikeSingleStatement) {
  if (::testing::internal::AlwaysFalse())
    EXPECT_WRITE(stdout, std::printf("x"), "x");

  if (::testing::internal::AlwaysTrue())
    EXPECT_WRITE(stdout, std::printf("x"), "x");
  else
    DoNothing();
}

// Tests EXPECT_THROW_MSG.
TEST(ExpectTest, EXPECT_THROW_MSG) {
  EXPECT_THROW_MSG(ThrowException(), std::exception, "test");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THROW_MSG(ThrowException(), std::logic_error, "test"),
      "Expected: ThrowException() throws an exception of "
      "type std::logic_error.\n  Actual: it throws a different type.");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THROW_MSG(DoNothing(), std::exception, "test"),
      "Expected: DoNothing() throws an exception of type std::exception.\n"
      "  Actual: it throws nothing.");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THROW_MSG(ThrowException(), std::exception, "other"),
      "ThrowException() throws an exception with a different message.\n"
      "Expected: other\n"
      "  Actual: test");
}

// Tests EXPECT_SYSTEM_ERROR.
TEST(ExpectTest, EXPECT_SYSTEM_ERROR) {
  EXPECT_SYSTEM_ERROR(ThrowSystemError(), EDOM, "test");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_SYSTEM_ERROR(ThrowException(), EDOM, "test"),
      "Expected: ThrowException() throws an exception of "
      "type fmt::SystemError.\n  Actual: it throws a different type.");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_SYSTEM_ERROR(DoNothing(), EDOM, "test"),
      "Expected: DoNothing() throws an exception of type fmt::SystemError.\n"
      "  Actual: it throws nothing.");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_SYSTEM_ERROR(ThrowSystemError(), EDOM, "other"),
      fmt::format(
          "ThrowSystemError() throws an exception with a different message.\n"
          "Expected: {}\n"
          "  Actual: {}",
          FormatSystemErrorMessage(EDOM, "other"),
          FormatSystemErrorMessage(EDOM, "test")));
}

// Tests EXPECT_WRITE.
TEST(ExpectTest, EXPECT_WRITE) {
  EXPECT_WRITE(stdout, DoNothing(), "");
  EXPECT_WRITE(stdout, std::printf("test"), "test");
  EXPECT_WRITE(stderr, std::fprintf(stderr, "test"), "test");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_WRITE(stdout, std::printf("that"), "this"),
      "Expected: this\n"
      "  Actual: that");
}

TEST(StreamingAssertionsTest, EXPECT_THROW_MSG) {
  EXPECT_THROW_MSG(ThrowException(), std::exception, "test")
      << "unexpected failure";
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THROW_MSG(ThrowException(), std::exception, "other")
      << "expected failure", "expected failure");
}

TEST(StreamingAssertionsTest, EXPECT_SYSTEM_ERROR) {
  EXPECT_SYSTEM_ERROR(ThrowSystemError(), EDOM, "test")
      << "unexpected failure";
  EXPECT_NONFATAL_FAILURE(
      EXPECT_SYSTEM_ERROR(ThrowSystemError(), EDOM, "other")
      << "expected failure", "expected failure");
}

TEST(StreamingAssertionsTest, EXPECT_WRITE) {
  EXPECT_WRITE(stdout, std::printf("test"), "test")
      << "unexpected failure";
  EXPECT_NONFATAL_FAILURE(
      EXPECT_WRITE(stdout, std::printf("test"), "other")
      << "expected failure", "expected failure");
}

TEST(UtilTest, FormatSystemErrorMessage) {
  fmt::Writer out;
  fmt::internal::FormatSystemErrorMessage(out, EDOM, "test message");
  EXPECT_EQ(out.str(), FormatSystemErrorMessage(EDOM, "test message"));
}

#if FMT_USE_FILE_DESCRIPTORS

using fmt::BufferedFile;
using fmt::ErrorCode;
using fmt::File;

// Checks if the file is open by reading one character from it.
bool IsOpen(int fd) {
  char buffer;
  return FMT_POSIX(read(fd, &buffer, 1)) == 1;
}

bool IsClosed(int fd) {
  char buffer;
  std::streamsize result = 0;
  SUPPRESS_ASSERT(result = FMT_POSIX(read(fd, &buffer, 1)));
  return result == -1 && errno == EBADF;
}

// Attempts to read count characters from a file.
std::string Read(File &f, std::size_t count) {
  std::string buffer(count, '\0');
  std::streamsize n = 0;
  std::size_t offset = 0;
  do {
    n = f.read(&buffer[offset], count - offset);
    // We can't read more than size_t bytes since count has type size_t.
    offset += static_cast<std::size_t>(n);
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
    // We can't write more than size_t bytes since num_chars_left
    // has type size_t.
    num_chars_left -= static_cast<std::size_t>(count);
  } while (num_chars_left != 0);
}

#define EXPECT_READ(file, expected_content) \
  EXPECT_EQ(expected_content, Read(file, std::strlen(expected_content)))

TEST(ErrorCodeTest, Ctor) {
  EXPECT_EQ(0, ErrorCode().get());
  EXPECT_EQ(42, ErrorCode(42).get());
}

const char FILE_CONTENT[] = "Don't panic!";

// Opens a file for reading.
File OpenFile() {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  write_end.write(FILE_CONTENT, sizeof(FILE_CONTENT) - 1);
  write_end.close();
  return read_end;
}

// Opens a buffered file for reading.
BufferedFile OpenBufferedFile(FILE **fp = 0) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  write_end.write(FILE_CONTENT, sizeof(FILE_CONTENT) - 1);
  write_end.close();
  BufferedFile f = read_end.fdopen("r");
  if (fp)
    *fp = f.get();
  return f;
}

TEST(BufferedFileTest, DefaultCtor) {
  BufferedFile f;
  EXPECT_TRUE(f.get() == 0);
}

TEST(BufferedFileTest, MoveCtor) {
  BufferedFile bf = OpenBufferedFile();
  FILE *fp = bf.get();
  EXPECT_TRUE(fp != 0);
  BufferedFile bf2(std::move(bf));
  EXPECT_EQ(fp, bf2.get());
  EXPECT_TRUE(bf.get() == 0);
}

TEST(BufferedFileTest, MoveAssignment) {
  BufferedFile bf = OpenBufferedFile();
  FILE *fp = bf.get();
  EXPECT_TRUE(fp != 0);
  BufferedFile bf2;
  bf2 = std::move(bf);
  EXPECT_EQ(fp, bf2.get());
  EXPECT_TRUE(bf.get() == 0);
}

TEST(BufferedFileTest, MoveAssignmentClosesFile) {
  BufferedFile bf = OpenBufferedFile();
  BufferedFile bf2 = OpenBufferedFile();
  int old_fd = bf2.fileno();
  bf2 = std::move(bf);
  EXPECT_TRUE(IsClosed(old_fd));
}

TEST(BufferedFileTest, MoveFromTemporaryInCtor) {
  FILE *fp = 0;
  BufferedFile f(OpenBufferedFile(&fp));
  EXPECT_EQ(fp, f.get());
}

TEST(BufferedFileTest, MoveFromTemporaryInAssignment) {
  FILE *fp = 0;
  BufferedFile f;
  f = OpenBufferedFile(&fp);
  EXPECT_EQ(fp, f.get());
}

TEST(BufferedFileTest, MoveFromTemporaryInAssignmentClosesFile) {
  BufferedFile f = OpenBufferedFile();
  int old_fd = f.fileno();
  f = OpenBufferedFile();
  EXPECT_TRUE(IsClosed(old_fd));
}

TEST(BufferedFileTest, CloseFileInDtor) {
  int fd = 0;
  {
    BufferedFile f = OpenBufferedFile();
    fd = f.fileno();
  }
  EXPECT_TRUE(IsClosed(fd));
}

TEST(BufferedFileTest, CloseErrorInDtor) {
  BufferedFile *f = new BufferedFile(OpenBufferedFile());
  EXPECT_WRITE(stderr, {
      // The close function must be called inside EXPECT_WRITE, otherwise
      // the system may recycle closed file descriptor when redirecting the
      // output in EXPECT_STDERR and the second close will break output
      // redirection.
      FMT_POSIX(close(f->fileno()));
      SUPPRESS_ASSERT(delete f);
  }, FormatSystemErrorMessage(EBADF, "cannot close file") + "\n");
}

TEST(BufferedFileTest, Close) {
  BufferedFile f = OpenBufferedFile();
  int fd = f.fileno();
  f.close();
  EXPECT_TRUE(f.get() == 0);
  EXPECT_TRUE(IsClosed(fd));
}

TEST(BufferedFileTest, CloseError) {
  BufferedFile f = OpenBufferedFile();
  FMT_POSIX(close(f.fileno()));
  EXPECT_SYSTEM_ERROR_NOASSERT(f.close(), EBADF, "cannot close file");
  EXPECT_TRUE(f.get() == 0);
}

TEST(BufferedFileTest, Fileno) {
  BufferedFile f;
  EXPECT_DEATH(f.fileno(), "");
  f = OpenBufferedFile();
  EXPECT_TRUE(f.fileno() != -1);
  File copy = File::dup(f.fileno());
  EXPECT_READ(copy, FILE_CONTENT);
}

TEST(FileTest, DefaultCtor) {
  File f;
  EXPECT_EQ(-1, f.descriptor());
}

TEST(FileTest, OpenBufferedFileInCtor) {
  FILE *fp = fopen("test-file", "w");
  std::fputs(FILE_CONTENT, fp);
  std::fclose(fp);
  File f("test-file", File::RDONLY);
  ASSERT_TRUE(IsOpen(f.descriptor()));
}

TEST(FileTest, OpenBufferedFileError) {
  EXPECT_SYSTEM_ERROR(File("nonexistent", File::RDONLY),
      ENOENT, "cannot open file nonexistent");
}

TEST(FileTest, MoveCtor) {
  File f = OpenFile();
  int fd = f.descriptor();
  EXPECT_NE(-1, fd);
  File f2(std::move(f));
  EXPECT_EQ(fd, f2.descriptor());
  EXPECT_EQ(-1, f.descriptor());
}

TEST(FileTest, MoveAssignment) {
  File f = OpenFile();
  int fd = f.descriptor();
  EXPECT_NE(-1, fd);
  File f2;
  f2 = std::move(f);
  EXPECT_EQ(fd, f2.descriptor());
  EXPECT_EQ(-1, f.descriptor());
}

TEST(FileTest, MoveAssignmentClosesFile) {
  File f = OpenFile();
  File f2 = OpenFile();
  int old_fd = f2.descriptor();
  f2 = std::move(f);
  EXPECT_TRUE(IsClosed(old_fd));
}

File OpenBufferedFile(int &fd) {
  File f = OpenFile();
  fd = f.descriptor();
  return std::move(f);
}

TEST(FileTest, MoveFromTemporaryInCtor) {
  int fd = 0xdeadbeef;
  File f(OpenBufferedFile(fd));
  EXPECT_EQ(fd, f.descriptor());
}

TEST(FileTest, MoveFromTemporaryInAssignment) {
  int fd = 0xdeadbeef;
  File f;
  f = OpenBufferedFile(fd);
  EXPECT_EQ(fd, f.descriptor());
}

TEST(FileTest, MoveFromTemporaryInAssignmentClosesFile) {
  int fd = 0xdeadbeef;
  File f = OpenFile();
  int old_fd = f.descriptor();
  f = OpenBufferedFile(fd);
  EXPECT_TRUE(IsClosed(old_fd));
}

TEST(FileTest, CloseFileInDtor) {
  int fd = 0;
  {
    File f = OpenFile();
    fd = f.descriptor();
  }
  EXPECT_TRUE(IsClosed(fd));
}

TEST(FileTest, CloseErrorInDtor) {
  File *f = new File(OpenFile());
  EXPECT_WRITE(stderr, {
      // The close function must be called inside EXPECT_WRITE, otherwise
      // the system may recycle closed file descriptor when redirecting the
      // output in EXPECT_STDERR and the second close will break output
      // redirection.
      FMT_POSIX(close(f->descriptor()));
      SUPPRESS_ASSERT(delete f);
  }, FormatSystemErrorMessage(EBADF, "cannot close file") + "\n");
}

TEST(FileTest, Close) {
  File f = OpenFile();
  int fd = f.descriptor();
  f.close();
  EXPECT_EQ(-1, f.descriptor());
  EXPECT_TRUE(IsClosed(fd));
}

TEST(FileTest, CloseError) {
  File f = OpenFile();
  FMT_POSIX(close(f.descriptor()));
  EXPECT_SYSTEM_ERROR_NOASSERT(f.close(), EBADF, "cannot close file");
  EXPECT_EQ(-1, f.descriptor());
}

TEST(FileTest, Read) {
  File f = OpenFile();
  EXPECT_READ(f, FILE_CONTENT);
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
  File f = OpenFile();
  File copy = File::dup(f.descriptor());
  EXPECT_NE(f.descriptor(), copy.descriptor());
  EXPECT_EQ(FILE_CONTENT, Read(copy, sizeof(FILE_CONTENT) - 1));
}

TEST(FileTest, DupError) {
  EXPECT_SYSTEM_ERROR_NOASSERT(File::dup(-1),
      EBADF, "cannot duplicate file descriptor -1");
}

TEST(FileTest, Dup2) {
  File f = OpenFile();
  File copy = OpenFile();
  f.dup2(copy.descriptor());
  EXPECT_NE(f.descriptor(), copy.descriptor());
  EXPECT_READ(copy, FILE_CONTENT);
}

TEST(FileTest, Dup2Error) {
  File f = OpenFile();
  EXPECT_SYSTEM_ERROR_NOASSERT(f.dup2(-1), EBADF,
    fmt::format("cannot duplicate file descriptor {} to -1", f.descriptor()));
}

TEST(FileTest, Dup2NoExcept) {
  File f = OpenFile();
  File copy = OpenFile();
  ErrorCode ec;
  f.dup2(copy.descriptor(), ec);
  EXPECT_EQ(0, ec.get());
  EXPECT_NE(f.descriptor(), copy.descriptor());
  EXPECT_READ(copy, FILE_CONTENT);
}

TEST(FileTest, Dup2NoExceptError) {
  File f = OpenFile();
  ErrorCode ec;
  SUPPRESS_ASSERT(f.dup2(-1, ec));
  EXPECT_EQ(EBADF, ec.get());
}

TEST(FileTest, Pipe) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  EXPECT_NE(-1, read_end.descriptor());
  EXPECT_NE(-1, write_end.descriptor());
  Write(write_end, "test");
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
  File write_copy = write_end.dup(write_fd);
  BufferedFile f = write_end.fdopen("w");
  // Put a character in a file buffer.
  EXPECT_EQ('x', fputc('x', f.get()));
  FMT_POSIX(close(write_fd));
  OutputRedirect *redir = 0;
  EXPECT_SYSTEM_ERROR_NOASSERT(redir = new OutputRedirect(f.get()),
      EBADF, "cannot flush stream");
  delete redir;
  write_copy.dup2(write_fd);  // "undo" close or dtor will fail
}

TEST(OutputRedirectTest, DupErrorInCtor) {
  BufferedFile f = OpenBufferedFile();
  int fd = f.fileno();
  File copy = File::dup(fd);
  FMT_POSIX(close(fd));
  OutputRedirect *redir = 0;
  EXPECT_SYSTEM_ERROR_NOASSERT(redir = new OutputRedirect(f.get()),
      EBADF, fmt::format("cannot duplicate file descriptor {}", fd));
  copy.dup2(fd);  // "undo" close or dtor will fail
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
  EXPECT_EQ("", redir.RestoreAndRead());
  std::fprintf(file.get(), "]]]");
  file = BufferedFile();
  EXPECT_READ(read_end, "[[[]]]");
}

// Test that OutputRedirect handles errors in flush correctly.
TEST(OutputRedirectTest, FlushErrorInRestoreAndRead) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  int write_fd = write_end.descriptor();
  File write_copy = write_end.dup(write_fd);
  BufferedFile f = write_end.fdopen("w");
  OutputRedirect redir(f.get());
  // Put a character in a file buffer.
  EXPECT_EQ('x', fputc('x', f.get()));
  FMT_POSIX(close(write_fd));
  EXPECT_SYSTEM_ERROR_NOASSERT(redir.RestoreAndRead(),
      EBADF, "cannot flush stream");
  write_copy.dup2(write_fd);  // "undo" close or dtor will fail
}

TEST(OutputRedirectTest, ErrorInDtor) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  int write_fd = write_end.descriptor();
  File write_copy = write_end.dup(write_fd);
  BufferedFile f = write_end.fdopen("w");
  OutputRedirect *redir = new OutputRedirect(f.get());
  // Put a character in a file buffer.
  EXPECT_EQ('x', fputc('x', f.get()));
  EXPECT_WRITE(stderr, {
      // The close function must be called inside EXPECT_WRITE, otherwise
      // the system may recycle closed file descriptor when redirecting the
      // output in EXPECT_STDERR and the second close will break output
      // redirection.
      FMT_POSIX(close(write_fd));
      SUPPRESS_ASSERT(delete redir);
  }, FormatSystemErrorMessage(EBADF, "cannot flush stream"));
  write_copy.dup2(write_fd); // "undo" close or dtor of BufferedFile will fail
}

#endif  // FMT_USE_FILE_DESCRIPTORS

}  // namespace
