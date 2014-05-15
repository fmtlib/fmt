/*
 Test wrappers around POSIX functions.

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

#include "posix-test.h"

#undef open

#include <errno.h>
#include <fcntl.h>

#include "gtest-extra.h"

namespace {
int open_count;
}

#ifndef _WIN32
int test::open(const char *path, int oflag, int mode) {
  if ((open_count++ % 3) == 2)
    return ::open(path, oflag, mode);
  errno = EINTR;
  return -1;
}
#else
errno_t test::sopen_s(int* pfh, const char *filename, int oflag, int shflag, int pmode) {
  return _sopen_s(pfh, filename, oflag, shflag, pmode);
}
#endif

#ifndef _WIN32
# define EXPECT_RETRY(statement, func, message) \
    func##_count = 0; \
    statement; \
    EXPECT_EQ(3, func##_count);
#else
# define EXPECT_RETRY(statement, func, message) \
    EXPECT_SYSTEM_ERROR(statement, EINTR, message);
#endif

TEST(FileTest, OpenRetry) {
  EXPECT_RETRY(File file("CMakeLists.txt", File::RDONLY),
               open, "cannot open file CMakeLists.txt");
}

// TODO: test retry on EINTR
