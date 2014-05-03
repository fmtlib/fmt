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

#include <algorithm>
#include <stdexcept>
#include <gtest/gtest-spi.h>

namespace {
  
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

TEST(FileDescriptorTest, DefaultCtor) {
  FileDescriptor fd;
  EXPECT_EQ(-1, fd.get());
}

TEST(FileDescriptorTest, OpenFileInCtor) {
  FILE *f = 0;
  {
    FileDescriptor fd(".travis.yml", FileDescriptor::RDONLY);
    f = fdopen(fd.get(), "r");
    ASSERT_TRUE(f != 0);
  }
  // Make sure fclose is called after the file descriptor is destroyed.
  // Otherwise the destructor will report an error because fclose has
  // already closed the file.
  fclose(f);
}

TEST(FileDescriptorTest, OpenFileError) {
  fmt::Writer message;
  fmt::internal::FormatSystemErrorMessage(
      message, ENOENT, "cannot open file nonexistent");
  EXPECT_THROW_MSG(FileDescriptor("nonexistent", FileDescriptor::RDONLY),
      fmt::SystemError, str(message));
}

TEST(FileDescriptorTest, MoveCtor) {
  FileDescriptor fd(".travis.yml", FileDescriptor::RDONLY);
  int fd_value = fd.get();
  EXPECT_NE(-1, fd_value);
  FileDescriptor fd2(std::move(fd));
  EXPECT_EQ(fd_value, fd2.get());
  EXPECT_EQ(-1, fd.get());
}

TEST(FileDescriptorTest, MoveAssignment) {
  FileDescriptor fd(".travis.yml", FileDescriptor::RDONLY);
  int fd_value = fd.get();
  EXPECT_NE(-1, fd_value);
  FileDescriptor fd2;
  fd2 = std::move(fd);
  EXPECT_EQ(fd_value, fd2.get());
  EXPECT_EQ(-1, fd.get());
}

bool IsClosed(int fd) {
  char buffer[1];
  ssize_t result = read(fd, buffer, sizeof(buffer));
  return result == -1 && errno == EBADF;
}

TEST(FileDescriptorTest, MoveAssignmentClosesFile) {
  FileDescriptor fd(".travis.yml", FileDescriptor::RDONLY);
  FileDescriptor fd2("CMakeLists.txt", FileDescriptor::RDONLY);
  int old_fd = fd2.get();
  fd2 = std::move(fd);
  EXPECT_TRUE(IsClosed(old_fd));
}

FileDescriptor OpenFile(int &fd_value) {
  FileDescriptor fd(".travis.yml", FileDescriptor::RDONLY);
  fd_value = fd.get();
  return std::move(fd);
}

TEST(FileDescriptorTest, MoveFromTemporaryInCtor) {
  int fd_value = 0xdeadbeef;
  FileDescriptor fd(OpenFile(fd_value));
  EXPECT_EQ(fd_value, fd.get());
}

TEST(FileDescriptorTest, MoveFromTemporaryInAssignment) {
  int fd_value = 0xdeadbeef;
  FileDescriptor fd;
  fd = OpenFile(fd_value);
  EXPECT_EQ(fd_value, fd.get());
}

TEST(FileDescriptorTest, MoveFromTemporaryInAssignmentClosesFile) {
  int fd_value = 0xdeadbeef;
  FileDescriptor fd(".travis.yml", FileDescriptor::RDONLY);
  int old_fd = fd.get();
  fd = OpenFile(fd_value);
  EXPECT_TRUE(IsClosed(old_fd));
}

TEST(FileDescriptorTest, CloseFileInDtor) {
  int fd_value = 0;
  {
    FileDescriptor fd(".travis.yml", FileDescriptor::RDONLY);
    fd_value = fd.get();
  }
  FILE *f = fdopen(fd_value, "r");
  int error_code = errno;
  if (f)
    fclose(f);
  EXPECT_TRUE(f == 0);
  EXPECT_EQ(EBADF, error_code);
}

TEST(FileDescriptorTest, CloseError) {
  FileDescriptor *fd =
      new FileDescriptor(".travis.yml", FileDescriptor::RDONLY);
  fmt::Writer message;
  fmt::internal::FormatSystemErrorMessage(message, EBADF, "cannot close file");
  EXPECT_STDERR(close(fd->get()); delete fd, str(message) + "\n");
}

std::string ReadLine(FileDescriptor &fd) {
  enum { BUFFER_SIZE = 100 };
  char buffer[BUFFER_SIZE];
  ssize_t result = read(fd.get(), buffer, BUFFER_SIZE);
  if (result == -1)
    fmt::ThrowSystemError(errno, "cannot read file");
  buffer[std::min<ssize_t>(BUFFER_SIZE - 1, result)] = '\0';
  if (char *end = strchr(buffer, '\n'))
    *end = '\0';
  return buffer;
}

TEST(FileDescriptorTest, Dup) {
  FileDescriptor fd(".travis.yml", FileDescriptor::RDONLY);
  FileDescriptor dup = FileDescriptor::dup(fd.get());
  EXPECT_NE(fd.get(), dup.get());
  EXPECT_EQ("language: cpp", ReadLine(dup));
}

TEST(FileDescriptorTest, DupError) {
  fmt::Writer message;
  fmt::internal::FormatSystemErrorMessage(
      message, EBADF, "cannot duplicate file descriptor -1");
  EXPECT_THROW_MSG(FileDescriptor::dup(-1), fmt::SystemError, str(message));
}

TEST(FileDescriptorTest, Dup2) {
  FileDescriptor fd(".travis.yml", FileDescriptor::RDONLY);
  FileDescriptor dup("CMakeLists.txt", FileDescriptor::RDONLY);
  fd.dup2(dup.get());
  EXPECT_NE(fd.get(), dup.get());
  EXPECT_EQ("language: cpp", ReadLine(dup));
}

TEST(FileDescriptorTest, Dup2Error) {
  FileDescriptor fd(".travis.yml", FileDescriptor::RDONLY);
  fmt::Writer message;
  fmt::internal::FormatSystemErrorMessage(message, EBADF,
      fmt::Format("cannot duplicate file descriptor {} to -1") << fd.get());
  EXPECT_THROW_MSG(fd.dup2(-1), fmt::SystemError, str(message));
}

TEST(FileDescriptorTest, Dup2NoExcept) {
  FileDescriptor fd(".travis.yml", FileDescriptor::RDONLY);
  FileDescriptor dup("CMakeLists.txt", FileDescriptor::RDONLY);
  ErrorCode ec;
  fd.dup2(dup.get(), ec);
  EXPECT_EQ(0, ec.get());
  EXPECT_NE(fd.get(), dup.get());
  EXPECT_EQ("language: cpp", ReadLine(dup));
}

TEST(FileDescriptorTest, Dup2NoExceptError) {
  FileDescriptor fd(".travis.yml", FileDescriptor::RDONLY);
  ErrorCode ec;
  fd.dup2(-1, ec);
  EXPECT_EQ(EBADF, ec.get());
}

// TODO: test pipe

// TODO: compile both with C++11 & C++98 mode

#endif

// TODO: test OutputRedirector

}  // namespace
