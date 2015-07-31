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

#include "util.h"

using testing::internal::scoped_ptr;

namespace {

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

void do_nothing() {}

void throw_exception() {
  throw std::runtime_error("test");
}

void throw_system_error() {
  throw fmt::SystemError(EDOM, "test");
}

// Tests that when EXPECT_THROW_MSG fails, it evaluates its message argument
// exactly once.
TEST_F(SingleEvaluationTest, FailedEXPECT_THROW_MSG) {
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THROW_MSG(throw_exception(), std::exception, p_++), "01234");
  EXPECT_EQ(s_ + 1, p_);
}

// Tests that when EXPECT_SYSTEM_ERROR fails, it evaluates its message argument
// exactly once.
TEST_F(SingleEvaluationTest, FailedEXPECT_SYSTEM_ERROR) {
  EXPECT_NONFATAL_FAILURE(
      EXPECT_SYSTEM_ERROR(throw_system_error(), EDOM, p_++), "01234");
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
    throw_exception();
  }, std::exception, (b_++, "test"));
  EXPECT_EQ(1, a_);
  EXPECT_EQ(1, b_);

  // failed EXPECT_THROW_MSG, throws different type
  EXPECT_NONFATAL_FAILURE(EXPECT_THROW_MSG({  // NOLINT
    a_++;
    throw_exception();
  }, std::logic_error, (b_++, "test")), "throws a different type");
  EXPECT_EQ(2, a_);
  EXPECT_EQ(2, b_);

  // failed EXPECT_THROW_MSG, throws an exception with different message
  EXPECT_NONFATAL_FAILURE(EXPECT_THROW_MSG({  // NOLINT
    a_++;
    throw_exception();
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
    throw_system_error();
  }, EDOM, (b_++, "test"));
  EXPECT_EQ(1, a_);
  EXPECT_EQ(1, b_);

  // failed EXPECT_SYSTEM_ERROR, throws different type
  EXPECT_NONFATAL_FAILURE(EXPECT_SYSTEM_ERROR({  // NOLINT
    a_++;
    throw_exception();
  }, EDOM, (b_++, "test")), "throws a different type");
  EXPECT_EQ(2, a_);
  EXPECT_EQ(2, b_);

  // failed EXPECT_SYSTEM_ERROR, throws an exception with different message
  EXPECT_NONFATAL_FAILURE(EXPECT_SYSTEM_ERROR({  // NOLINT
    a_++;
    throw_system_error();
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
    EXPECT_THROW_MSG(do_nothing(), std::exception, "");

  if (::testing::internal::AlwaysTrue())
    EXPECT_THROW_MSG(throw_exception(), std::exception, "test");
  else
    do_nothing();
}

TEST(AssertionSyntaxTest, SystemErrorAssertionBehavesLikeSingleStatement) {
  if (::testing::internal::AlwaysFalse())
    EXPECT_SYSTEM_ERROR(do_nothing(), EDOM, "");

  if (::testing::internal::AlwaysTrue())
    EXPECT_SYSTEM_ERROR(throw_system_error(), EDOM, "test");
  else
    do_nothing();
}

TEST(AssertionSyntaxTest, WriteAssertionBehavesLikeSingleStatement) {
  if (::testing::internal::AlwaysFalse())
    EXPECT_WRITE(stdout, std::printf("x"), "x");

  if (::testing::internal::AlwaysTrue())
    EXPECT_WRITE(stdout, std::printf("x"), "x");
  else
    do_nothing();
}

// Tests EXPECT_THROW_MSG.
TEST(ExpectTest, EXPECT_THROW_MSG) {
  EXPECT_THROW_MSG(throw_exception(), std::exception, "test");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THROW_MSG(throw_exception(), std::logic_error, "test"),
      "Expected: throw_exception() throws an exception of "
      "type std::logic_error.\n  Actual: it throws a different type.");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THROW_MSG(do_nothing(), std::exception, "test"),
      "Expected: do_nothing() throws an exception of type std::exception.\n"
      "  Actual: it throws nothing.");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THROW_MSG(throw_exception(), std::exception, "other"),
      "throw_exception() throws an exception with a different message.\n"
      "Expected: other\n"
      "  Actual: test");
}

// Tests EXPECT_SYSTEM_ERROR.
TEST(ExpectTest, EXPECT_SYSTEM_ERROR) {
  EXPECT_SYSTEM_ERROR(throw_system_error(), EDOM, "test");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_SYSTEM_ERROR(throw_exception(), EDOM, "test"),
      "Expected: throw_exception() throws an exception of "
      "type fmt::SystemError.\n  Actual: it throws a different type.");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_SYSTEM_ERROR(do_nothing(), EDOM, "test"),
      "Expected: do_nothing() throws an exception of type fmt::SystemError.\n"
      "  Actual: it throws nothing.");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_SYSTEM_ERROR(throw_system_error(), EDOM, "other"),
      fmt::format(
          "throw_system_error() throws an exception with a different message.\n"
          "Expected: {}\n"
          "  Actual: {}",
          format_system_error(EDOM, "other"),
          format_system_error(EDOM, "test")));
}

// Tests EXPECT_WRITE.
TEST(ExpectTest, EXPECT_WRITE) {
  EXPECT_WRITE(stdout, do_nothing(), "");
  EXPECT_WRITE(stdout, std::printf("test"), "test");
  EXPECT_WRITE(stderr, std::fprintf(stderr, "test"), "test");
  EXPECT_NONFATAL_FAILURE(
      EXPECT_WRITE(stdout, std::printf("that"), "this"),
      "Expected: this\n"
      "  Actual: that");
}

TEST(StreamingAssertionsTest, EXPECT_THROW_MSG) {
  EXPECT_THROW_MSG(throw_exception(), std::exception, "test")
      << "unexpected failure";
  EXPECT_NONFATAL_FAILURE(
      EXPECT_THROW_MSG(throw_exception(), std::exception, "other")
      << "expected failure", "expected failure");
}

TEST(StreamingAssertionsTest, EXPECT_SYSTEM_ERROR) {
  EXPECT_SYSTEM_ERROR(throw_system_error(), EDOM, "test")
      << "unexpected failure";
  EXPECT_NONFATAL_FAILURE(
      EXPECT_SYSTEM_ERROR(throw_system_error(), EDOM, "other")
      << "expected failure", "expected failure");
}

TEST(StreamingAssertionsTest, EXPECT_WRITE) {
  EXPECT_WRITE(stdout, std::printf("test"), "test")
      << "unexpected failure";
  EXPECT_NONFATAL_FAILURE(
      EXPECT_WRITE(stdout, std::printf("test"), "other")
      << "expected failure", "expected failure");
}

TEST(UtilTest, FormatSystemError) {
  fmt::MemoryWriter out;
  fmt::internal::format_system_error(out, EDOM, "test message");
  EXPECT_EQ(out.str(), format_system_error(EDOM, "test message"));
}

#if FMT_USE_FILE_DESCRIPTORS

using fmt::BufferedFile;
using fmt::ErrorCode;
using fmt::File;

TEST(ErrorCodeTest, Ctor) {
  EXPECT_EQ(0, ErrorCode().get());
  EXPECT_EQ(42, ErrorCode(42).get());
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
  scoped_ptr<OutputRedirect> redir;
  EXPECT_SYSTEM_ERROR_NOASSERT(redir.reset(new OutputRedirect(f.get())),
      EBADF, "cannot flush stream");
  redir.reset();
  write_copy.dup2(write_fd);  // "undo" close or dtor will fail
}

TEST(OutputRedirectTest, DupErrorInCtor) {
  BufferedFile f = open_buffered_file();
  int fd = (f.fileno)();
  File copy = File::dup(fd);
  FMT_POSIX(close(fd));
  scoped_ptr<OutputRedirect> redir;
  EXPECT_SYSTEM_ERROR_NOASSERT(redir.reset(new OutputRedirect(f.get())),
      EBADF, fmt::format("cannot duplicate file descriptor {}", fd));
  copy.dup2(fd);  // "undo" close or dtor will fail
}

TEST(OutputRedirectTest, RestoreAndRead) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  BufferedFile file(write_end.fdopen("w"));
  std::fprintf(file.get(), "[[[");
  OutputRedirect redir(file.get());
  std::fprintf(file.get(), "censored");
  EXPECT_EQ("censored", redir.restore_and_read());
  EXPECT_EQ("", redir.restore_and_read());
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
  EXPECT_SYSTEM_ERROR_NOASSERT(redir.restore_and_read(),
      EBADF, "cannot flush stream");
  write_copy.dup2(write_fd);  // "undo" close or dtor will fail
}

TEST(OutputRedirectTest, ErrorInDtor) {
  File read_end, write_end;
  File::pipe(read_end, write_end);
  int write_fd = write_end.descriptor();
  File write_copy = write_end.dup(write_fd);
  BufferedFile f = write_end.fdopen("w");
  scoped_ptr<OutputRedirect> redir(new OutputRedirect(f.get()));
  // Put a character in a file buffer.
  EXPECT_EQ('x', fputc('x', f.get()));
  EXPECT_WRITE(stderr, {
      // The close function must be called inside EXPECT_WRITE, otherwise
      // the system may recycle closed file descriptor when redirecting the
      // output in EXPECT_STDERR and the second close will break output
      // redirection.
      FMT_POSIX(close(write_fd));
      SUPPRESS_ASSERT(redir.reset());
  }, format_system_error(EBADF, "cannot flush stream"));
  write_copy.dup2(write_fd); // "undo" close or dtor of BufferedFile will fail
}

#endif  // FMT_USE_FILE_DESCRIPTORS

}  // namespace
