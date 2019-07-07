// Tests of the C++ interface to POSIX functions that require mocks
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

// Disable bogus MSVC warnings.
#ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include "posix-mock.h"
#include "../src/posix.cc"

#include <errno.h>
#include <fcntl.h>
#include <climits>
#include <memory>

#ifdef _WIN32
#  include <io.h>
#  undef max
#  undef ERROR
#endif

#include "gmock.h"
#include "gtest-extra.h"
#include "util.h"

using fmt::buffered_file;
using fmt::error_code;
using fmt::file;

using testing::_;
using testing::Return;
using testing::StrEq;

namespace {
int open_count;
int close_count;
int dup_count;
int dup2_count;
int fdopen_count;
int read_count;
int write_count;
int pipe_count;
int fopen_count;
int fclose_count;
int fileno_count;
std::size_t read_nbyte;
std::size_t write_nbyte;
bool sysconf_error;

enum FStatSimulation { NONE, MAX_SIZE, ERROR } fstat_sim;
}  // namespace

#define EMULATE_EINTR(func, error_result) \
  if (func##_count != 0) {                \
    if (func##_count++ != 3) {            \
      errno = EINTR;                      \
      return error_result;                \
    }                                     \
  }

#ifndef _MSC_VER
int test::open(const char* path, int oflag, int mode) {
  EMULATE_EINTR(open, -1);
  return ::open(path, oflag, mode);
}
#else
errno_t test::sopen_s(int* pfh, const char* filename, int oflag, int shflag,
                      int pmode) {
  EMULATE_EINTR(open, EINTR);
  return _sopen_s(pfh, filename, oflag, shflag, pmode);
}
#endif

#ifndef _WIN32

long test::sysconf(int name) {
  long result = ::sysconf(name);
  if (!sysconf_error) return result;
  // Simulate an error.
  errno = EINVAL;
  return -1;
}

static off_t max_file_size() { return std::numeric_limits<off_t>::max(); }

int test::fstat(int fd, struct stat* buf) {
  int result = ::fstat(fd, buf);
  if (fstat_sim == MAX_SIZE) buf->st_size = max_file_size();
  return result;
}

#else

static LONGLONG max_file_size() { return std::numeric_limits<LONGLONG>::max(); }

DWORD test::GetFileSize(HANDLE hFile, LPDWORD lpFileSizeHigh) {
  if (fstat_sim == ERROR) {
    SetLastError(ERROR_ACCESS_DENIED);
    return INVALID_FILE_SIZE;
  }
  if (fstat_sim == MAX_SIZE) {
    DWORD max = std::numeric_limits<DWORD>::max();
    *lpFileSizeHigh = max >> 1;
    return max;
  }
  return ::GetFileSize(hFile, lpFileSizeHigh);
}

#endif

int test::close(int fildes) {
  // Close the file first because close shouldn't be retried.
  int result = ::FMT_POSIX(close(fildes));
  EMULATE_EINTR(close, -1);
  return result;
}

int test::dup(int fildes) {
  EMULATE_EINTR(dup, -1);
  return ::FMT_POSIX(dup(fildes));
}

int test::dup2(int fildes, int fildes2) {
  EMULATE_EINTR(dup2, -1);
  return ::FMT_POSIX(dup2(fildes, fildes2));
}

FILE* test::fdopen(int fildes, const char* mode) {
  EMULATE_EINTR(fdopen, nullptr);
  return ::FMT_POSIX(fdopen(fildes, mode));
}

test::ssize_t test::read(int fildes, void* buf, test::size_t nbyte) {
  read_nbyte = nbyte;
  EMULATE_EINTR(read, -1);
  return ::FMT_POSIX(read(fildes, buf, nbyte));
}

test::ssize_t test::write(int fildes, const void* buf, test::size_t nbyte) {
  write_nbyte = nbyte;
  EMULATE_EINTR(write, -1);
  return ::FMT_POSIX(write(fildes, buf, nbyte));
}

#ifndef _WIN32
int test::pipe(int fildes[2]) {
  EMULATE_EINTR(pipe, -1);
  return ::pipe(fildes);
}
#else
int test::pipe(int* pfds, unsigned psize, int textmode) {
  EMULATE_EINTR(pipe, -1);
  return _pipe(pfds, psize, textmode);
}
#endif

FILE* test::fopen(const char* filename, const char* mode) {
  EMULATE_EINTR(fopen, nullptr);
  return ::fopen(filename, mode);
}

int test::fclose(FILE* stream) {
  EMULATE_EINTR(fclose, EOF);
  return ::fclose(stream);
}

int(test::fileno)(FILE* stream) {
  EMULATE_EINTR(fileno, -1);
#ifdef fileno
  return FMT_POSIX(fileno(stream));
#else
  return ::FMT_POSIX(fileno(stream));
#endif
}

#ifndef _WIN32
#  define EXPECT_RETRY(statement, func, message) \
    func##_count = 1;                            \
    statement;                                   \
    EXPECT_EQ(4, func##_count);                  \
    func##_count = 0;
#  define EXPECT_EQ_POSIX(expected, actual) EXPECT_EQ(expected, actual)
#else
#  define EXPECT_RETRY(statement, func, message)    \
    func##_count = 1;                               \
    EXPECT_SYSTEM_ERROR(statement, EINTR, message); \
    func##_count = 0;
#  define EXPECT_EQ_POSIX(expected, actual)
#endif

static void write_file(fmt::cstring_view filename, fmt::string_view content) {
  fmt::buffered_file f(filename, "w");
  f.print("{}", content);
}

TEST(UtilTest, GetPageSize) {
#ifdef _WIN32
  SYSTEM_INFO si = {};
  GetSystemInfo(&si);
  EXPECT_EQ(si.dwPageSize, fmt::getpagesize());
#else
  EXPECT_EQ(sysconf(_SC_PAGESIZE), fmt::getpagesize());
  sysconf_error = true;
  EXPECT_SYSTEM_ERROR(fmt::getpagesize(), EINVAL,
                      "cannot get memory page size");
  sysconf_error = false;
#endif
}

TEST(FileTest, OpenRetry) {
  write_file("test", "there must be something here");
  std::unique_ptr<file> f{nullptr};
  EXPECT_RETRY(f.reset(new file("test", file::RDONLY)), open,
               "cannot open file test");
#ifndef _WIN32
  char c = 0;
  f->read(&c, 1);
#endif
}

TEST(FileTest, CloseNoRetryInDtor) {
  file read_end, write_end;
  file::pipe(read_end, write_end);
  std::unique_ptr<file> f(new file(std::move(read_end)));
  int saved_close_count = 0;
  EXPECT_WRITE(stderr,
               {
                 close_count = 1;
                 f.reset(nullptr);
                 saved_close_count = close_count;
                 close_count = 0;
               },
               format_system_error(EINTR, "cannot close file") + "\n");
  EXPECT_EQ(2, saved_close_count);
}

TEST(FileTest, CloseNoRetry) {
  file read_end, write_end;
  file::pipe(read_end, write_end);
  close_count = 1;
  EXPECT_SYSTEM_ERROR(read_end.close(), EINTR, "cannot close file");
  EXPECT_EQ(2, close_count);
  close_count = 0;
}

TEST(FileTest, Size) {
  std::string content = "top secret, destroy before reading";
  write_file("test", content);
  file f("test", file::RDONLY);
  EXPECT_GE(f.size(), 0);
  EXPECT_EQ(content.size(), static_cast<unsigned long long>(f.size()));
#ifdef _WIN32
  fmt::memory_buffer message;
  fmt::internal::format_windows_error(message, ERROR_ACCESS_DENIED,
                                      "cannot get file size");
  fstat_sim = ERROR;
  EXPECT_THROW_MSG(f.size(), fmt::windows_error, fmt::to_string(message));
  fstat_sim = NONE;
#else
  f.close();
  EXPECT_SYSTEM_ERROR(f.size(), EBADF, "cannot get file attributes");
#endif
}

TEST(FileTest, MaxSize) {
  write_file("test", "");
  file f("test", file::RDONLY);
  fstat_sim = MAX_SIZE;
  EXPECT_GE(f.size(), 0);
  EXPECT_EQ(max_file_size(), f.size());
  fstat_sim = NONE;
}

TEST(FileTest, ReadRetry) {
  file read_end, write_end;
  file::pipe(read_end, write_end);
  enum { SIZE = 4 };
  write_end.write("test", SIZE);
  write_end.close();
  char buffer[SIZE];
  std::size_t count = 0;
  EXPECT_RETRY(count = read_end.read(buffer, SIZE), read,
               "cannot read from file");
  EXPECT_EQ_POSIX(static_cast<std::streamsize>(SIZE), count);
}

TEST(FileTest, WriteRetry) {
  file read_end, write_end;
  file::pipe(read_end, write_end);
  enum { SIZE = 4 };
  std::size_t count = 0;
  EXPECT_RETRY(count = write_end.write("test", SIZE), write,
               "cannot write to file");
  write_end.close();
#ifndef _WIN32
  EXPECT_EQ(static_cast<std::streamsize>(SIZE), count);
  char buffer[SIZE + 1];
  read_end.read(buffer, SIZE);
  buffer[SIZE] = '\0';
  EXPECT_STREQ("test", buffer);
#endif
}

#ifdef _WIN32
TEST(FileTest, ConvertReadCount) {
  file read_end, write_end;
  file::pipe(read_end, write_end);
  char c;
  std::size_t size = UINT_MAX;
  if (sizeof(unsigned) != sizeof(std::size_t)) ++size;
  read_count = 1;
  read_nbyte = 0;
  EXPECT_THROW(read_end.read(&c, size), fmt::system_error);
  read_count = 0;
  EXPECT_EQ(UINT_MAX, read_nbyte);
}

TEST(FileTest, ConvertWriteCount) {
  file read_end, write_end;
  file::pipe(read_end, write_end);
  char c;
  std::size_t size = UINT_MAX;
  if (sizeof(unsigned) != sizeof(std::size_t)) ++size;
  write_count = 1;
  write_nbyte = 0;
  EXPECT_THROW(write_end.write(&c, size), fmt::system_error);
  write_count = 0;
  EXPECT_EQ(UINT_MAX, write_nbyte);
}
#endif

TEST(FileTest, DupNoRetry) {
  int stdout_fd = FMT_POSIX(fileno(stdout));
  dup_count = 1;
  EXPECT_SYSTEM_ERROR(
      file::dup(stdout_fd), EINTR,
      fmt::format("cannot duplicate file descriptor {}", stdout_fd));
  dup_count = 0;
}

TEST(FileTest, Dup2Retry) {
  int stdout_fd = FMT_POSIX(fileno(stdout));
  file f1 = file::dup(stdout_fd), f2 = file::dup(stdout_fd);
  EXPECT_RETRY(f1.dup2(f2.descriptor()), dup2,
               fmt::format("cannot duplicate file descriptor {} to {}",
                           f1.descriptor(), f2.descriptor()));
}

TEST(FileTest, Dup2NoExceptRetry) {
  int stdout_fd = FMT_POSIX(fileno(stdout));
  file f1 = file::dup(stdout_fd), f2 = file::dup(stdout_fd);
  error_code ec;
  dup2_count = 1;
  f1.dup2(f2.descriptor(), ec);
#ifndef _WIN32
  EXPECT_EQ(4, dup2_count);
#else
  EXPECT_EQ(EINTR, ec.get());
#endif
  dup2_count = 0;
}

TEST(FileTest, PipeNoRetry) {
  file read_end, write_end;
  pipe_count = 1;
  EXPECT_SYSTEM_ERROR(file::pipe(read_end, write_end), EINTR,
                      "cannot create pipe");
  pipe_count = 0;
}

TEST(FileTest, FdopenNoRetry) {
  file read_end, write_end;
  file::pipe(read_end, write_end);
  fdopen_count = 1;
  EXPECT_SYSTEM_ERROR(read_end.fdopen("r"), EINTR,
                      "cannot associate stream with file descriptor");
  fdopen_count = 0;
}

TEST(BufferedFileTest, OpenRetry) {
  write_file("test", "there must be something here");
  std::unique_ptr<buffered_file> f{nullptr};
  EXPECT_RETRY(f.reset(new buffered_file("test", "r")), fopen,
               "cannot open file test");
#ifndef _WIN32
  char c = 0;
  if (fread(&c, 1, 1, f->get()) < 1)
    throw fmt::system_error(errno, "fread failed");
#endif
}

TEST(BufferedFileTest, CloseNoRetryInDtor) {
  file read_end, write_end;
  file::pipe(read_end, write_end);
  std::unique_ptr<buffered_file> f(new buffered_file(read_end.fdopen("r")));
  int saved_fclose_count = 0;
  EXPECT_WRITE(stderr,
               {
                 fclose_count = 1;
                 f.reset(nullptr);
                 saved_fclose_count = fclose_count;
                 fclose_count = 0;
               },
               format_system_error(EINTR, "cannot close file") + "\n");
  EXPECT_EQ(2, saved_fclose_count);
}

TEST(BufferedFileTest, CloseNoRetry) {
  file read_end, write_end;
  file::pipe(read_end, write_end);
  buffered_file f = read_end.fdopen("r");
  fclose_count = 1;
  EXPECT_SYSTEM_ERROR(f.close(), EINTR, "cannot close file");
  EXPECT_EQ(2, fclose_count);
  fclose_count = 0;
}

TEST(BufferedFileTest, FilenoNoRetry) {
  file read_end, write_end;
  file::pipe(read_end, write_end);
  buffered_file f = read_end.fdopen("r");
  fileno_count = 1;
  EXPECT_SYSTEM_ERROR((f.fileno)(), EINTR, "cannot get file descriptor");
  EXPECT_EQ(2, fileno_count);
  fileno_count = 0;
}

struct TestMock {
  static TestMock* instance;
} * TestMock::instance;

TEST(ScopedMock, Scope) {
  {
    ScopedMock<TestMock> mock;
    EXPECT_EQ(&mock, TestMock::instance);
    TestMock& copy = mock;
    static_cast<void>(copy);
  }
  EXPECT_EQ(nullptr, TestMock::instance);
}

#ifdef FMT_LOCALE

typedef fmt::Locale::type LocaleType;

struct LocaleMock {
  static LocaleMock* instance;
  MOCK_METHOD3(newlocale, LocaleType(int category_mask, const char* locale,
                                     LocaleType base));
  MOCK_METHOD1(freelocale, void(LocaleType locale));

  MOCK_METHOD3(strtod_l,
               double(const char* nptr, char** endptr, LocaleType locale));
} * LocaleMock::instance;

#  ifdef _MSC_VER
#    pragma warning(push)
#    pragma warning(disable : 4273)
#    ifdef __clang__
#      pragma clang diagnostic push
#      pragma clang diagnostic ignored "-Winconsistent-dllimport"
#    endif

_locale_t _create_locale(int category, const char* locale) {
  return LocaleMock::instance->newlocale(category, locale, 0);
}

void _free_locale(_locale_t locale) {
  LocaleMock::instance->freelocale(locale);
}

double _strtod_l(const char* nptr, char** endptr, _locale_t locale) {
  return LocaleMock::instance->strtod_l(nptr, endptr, locale);
}
#    ifdef __clang__
#      pragma clang diagnostic pop
#    endif
#    pragma warning(pop)
#  endif

#  if defined(__THROW) && FMT_GCC_VERSION > 0 && FMT_GCC_VERSION <= 408
#    define FMT_LOCALE_THROW __THROW
#  else
#    define FMT_LOCALE_THROW
#  endif

LocaleType newlocale(int category_mask, const char* locale,
                     LocaleType base) FMT_LOCALE_THROW {
  return LocaleMock::instance->newlocale(category_mask, locale, base);
}

#  if defined(__APPLE__) || \
      (defined(__FreeBSD__) && __FreeBSD_version < 1200002)
typedef int FreeLocaleResult;
#  else
typedef void FreeLocaleResult;
#  endif

FreeLocaleResult freelocale(LocaleType locale) FMT_LOCALE_THROW {
  LocaleMock::instance->freelocale(locale);
  return FreeLocaleResult();
}

double strtod_l(const char* nptr, char** endptr,
                LocaleType locale) FMT_LOCALE_THROW {
  return LocaleMock::instance->strtod_l(nptr, endptr, locale);
}

#  undef FMT_LOCALE_THROW

TEST(LocaleTest, LocaleMock) {
  ScopedMock<LocaleMock> mock;
  LocaleType locale = reinterpret_cast<LocaleType>(11);
  EXPECT_CALL(mock, newlocale(222, StrEq("foo"), locale));
  newlocale(222, "foo", locale);
}

TEST(LocaleTest, Locale) {
#  ifndef LC_NUMERIC_MASK
  enum { LC_NUMERIC_MASK = LC_NUMERIC };
#  endif
  ScopedMock<LocaleMock> mock;
  LocaleType impl = reinterpret_cast<LocaleType>(42);
  EXPECT_CALL(mock, newlocale(LC_NUMERIC_MASK, StrEq("C"), nullptr))
      .WillOnce(Return(impl));
  EXPECT_CALL(mock, freelocale(impl));
  fmt::Locale locale;
  EXPECT_EQ(impl, locale.get());
}

TEST(LocaleTest, Strtod) {
  ScopedMock<LocaleMock> mock;
  EXPECT_CALL(mock, newlocale(_, _, _))
      .WillOnce(Return(reinterpret_cast<LocaleType>(42)));
  EXPECT_CALL(mock, freelocale(_));
  fmt::Locale locale;
  const char* str = "4.2";
  char end = 'x';
  EXPECT_CALL(mock, strtod_l(str, _, locale.get()))
      .WillOnce(testing::DoAll(testing::SetArgPointee<1>(&end), Return(777)));
  EXPECT_EQ(777, locale.strtod(str));
  EXPECT_EQ(&end, str);
}

#endif  // FMT_LOCALE
