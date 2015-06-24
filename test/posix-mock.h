/*
 Mocks of POSIX functions

 Copyright (c) 2012-2015, Victor Zverovich
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

#ifndef FMT_POSIX_TEST_H
#define FMT_POSIX_TEST_H

#include <errno.h>
#include <stdio.h>

#ifdef _WIN32
# include <windows.h>
#else
# include <sys/types.h>  // for ssize_t
#endif

#ifndef _MSC_VER
struct stat;
#endif

namespace test {

#ifndef _MSC_VER
// Size type for read and write.
typedef size_t size_t;
typedef ssize_t ssize_t;
int open(const char *path, int oflag, int mode);
int fstat(int fd, struct stat *buf);
#else
typedef unsigned size_t;
typedef int ssize_t;
errno_t sopen_s(
    int* pfh, const char *filename, int oflag, int shflag, int pmode);
#endif

#ifndef _WIN32
long sysconf(int name);
#else
DWORD GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh);
#endif

int close(int fildes);

int dup(int fildes);
int dup2(int fildes, int fildes2);

FILE *fdopen(int fildes, const char *mode);

ssize_t read(int fildes, void *buf, size_t nbyte);
ssize_t write(int fildes, const void *buf, size_t nbyte);

#ifndef _WIN32
int pipe(int fildes[2]);
#else
int pipe(int *pfds, unsigned psize, int textmode);
#endif

FILE *fopen(const char *filename, const char *mode);
int fclose(FILE *stream);
int (fileno)(FILE *stream);
}  // namespace test

#define FMT_SYSTEM(call) test::call

#endif  // FMT_POSIX_TEST_H
