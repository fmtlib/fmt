// Formatting library for C++ - tests of the OS-specific functionality
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/os.h"

#include <cstdlib>  // std::exit
#include <cstring>
#include <memory>

#include "gtest-extra.h"
#include "util.h"

#ifdef fileno
#  undef fileno
#endif

using fmt::buffered_file;
using fmt::error_code;

#ifdef _WIN32

#  include <windows.h>

TEST(UtilTest, UTF16ToUTF8) {
  std::string s = "ёжик";
  fmt::detail::utf16_to_utf8 u(L"\x0451\x0436\x0438\x043A");
  EXPECT_EQ(s, u.str());
  EXPECT_EQ(s.size(), u.size());
}

TEST(UtilTest, UTF16ToUTF8EmptyString) {
  std::string s = "";
  fmt::detail::utf16_to_utf8 u(L"");
  EXPECT_EQ(s, u.str());
  EXPECT_EQ(s.size(), u.size());
}

template <typename Converter, typename Char>
void check_utf_conversion_error(
    const char* message,
    fmt::basic_string_view<Char> str = fmt::basic_string_view<Char>(0, 1)) {
  fmt::memory_buffer out;
  fmt::detail::format_windows_error(out, ERROR_INVALID_PARAMETER, message);
  fmt::system_error error(0, "");
  try {
    (Converter)(str);
  } catch (const fmt::system_error& e) {
    error = e;
  }
  EXPECT_EQ(ERROR_INVALID_PARAMETER, error.error_code());
  EXPECT_EQ(fmt::to_string(out), error.what());
}

TEST(UtilTest, UTF16ToUTF8Error) {
  check_utf_conversion_error<fmt::detail::utf16_to_utf8, wchar_t>(
      "cannot convert string from UTF-16 to UTF-8");
}

TEST(UtilTest, UTF16ToUTF8Convert) {
  fmt::detail::utf16_to_utf8 u;
  EXPECT_EQ(ERROR_INVALID_PARAMETER, u.convert(fmt::wstring_view(0, 1)));
  EXPECT_EQ(ERROR_INVALID_PARAMETER,
            u.convert(fmt::wstring_view(L"foo", INT_MAX + 1u)));
}

TEST(UtilTest, FormatWindowsError) {
  LPWSTR message = 0;
  FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM |
                     FORMAT_MESSAGE_IGNORE_INSERTS,
                 0, ERROR_FILE_EXISTS,
                 MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                 reinterpret_cast<LPWSTR>(&message), 0, 0);
  fmt::detail::utf16_to_utf8 utf8_message(message);
  LocalFree(message);
  fmt::memory_buffer actual_message;
  fmt::detail::format_windows_error(actual_message, ERROR_FILE_EXISTS, "test");
  EXPECT_EQ(fmt::format("test: {}", utf8_message.str()),
            fmt::to_string(actual_message));
  actual_message.resize(0);
  auto max_size = fmt::detail::max_value<size_t>() / 2;
  fmt::detail::format_windows_error(actual_message, ERROR_FILE_EXISTS,
                                    fmt::string_view(nullptr, max_size));
  EXPECT_EQ(fmt::format("error {}", ERROR_FILE_EXISTS),
            fmt::to_string(actual_message));
}

TEST(UtilTest, FormatLongWindowsError) {
  LPWSTR message = 0;
  // this error code is not available on all Windows platforms and
  // Windows SDKs, so do not fail the test if the error string cannot
  // be retrieved.
  const int provisioning_not_allowed =
      0x80284013L /*TBS_E_PROVISIONING_NOT_ALLOWED*/;
  if (FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                         FORMAT_MESSAGE_FROM_SYSTEM |
                         FORMAT_MESSAGE_IGNORE_INSERTS,
                     0, static_cast<DWORD>(provisioning_not_allowed),
                     MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                     reinterpret_cast<LPWSTR>(&message), 0, 0) == 0) {
    return;
  }
  fmt::detail::utf16_to_utf8 utf8_message(message);
  LocalFree(message);
  fmt::memory_buffer actual_message;
  fmt::detail::format_windows_error(actual_message, provisioning_not_allowed,
                                    "test");
  EXPECT_EQ(fmt::format("test: {}", utf8_message.str()),
            fmt::to_string(actual_message));
}

TEST(UtilTest, WindowsError) {
  fmt::system_error error(0, "");
  try {
    throw fmt::windows_error(ERROR_FILE_EXISTS, "test {}", "error");
  } catch (const fmt::system_error& e) {
    error = e;
  }
  fmt::memory_buffer message;
  fmt::detail::format_windows_error(message, ERROR_FILE_EXISTS, "test error");
  EXPECT_EQ(to_string(message), error.what());
  EXPECT_EQ(ERROR_FILE_EXISTS, error.error_code());
}

TEST(UtilTest, ReportWindowsError) {
  fmt::memory_buffer out;
  fmt::detail::format_windows_error(out, ERROR_FILE_EXISTS, "test error");
  out.push_back('\n');
  EXPECT_WRITE(stderr,
               fmt::report_windows_error(ERROR_FILE_EXISTS, "test error"),
               fmt::to_string(out));
}

#endif  // _WIN32

#if FMT_USE_FCNTL

using fmt::file;

// Checks if the file is open by reading one character from it.
static bool isopen(int fd) {
  char buffer;
  return FMT_POSIX(read(fd, &buffer, 1)) == 1;
}

static bool isclosed(int fd) {
  char buffer;
  std::streamsize result = 0;
  SUPPRESS_ASSERT(result = FMT_POSIX(read(fd, &buffer, 1)));
  return result == -1 && errno == EBADF;
}

// Opens a file for reading.
static file open_file() {
  file read_end, write_end;
  file::pipe(read_end, write_end);
  write_end.write(FILE_CONTENT, std::strlen(FILE_CONTENT));
  write_end.close();
  return read_end;
}

// Attempts to write a string to a file.
static void write(file& f, fmt::string_view s) {
  size_t num_chars_left = s.size();
  const char* ptr = s.data();
  do {
    size_t count = f.write(ptr, num_chars_left);
    ptr += count;
    // We can't write more than size_t bytes since num_chars_left
    // has type size_t.
    num_chars_left -= count;
  } while (num_chars_left != 0);
}

TEST(BufferedFileTest, DefaultCtor) {
  buffered_file f;
  EXPECT_TRUE(f.get() == nullptr);
}

TEST(BufferedFileTest, MoveCtor) {
  buffered_file bf = open_buffered_file();
  FILE* fp = bf.get();
  EXPECT_TRUE(fp != nullptr);
  buffered_file bf2(std::move(bf));
  EXPECT_EQ(fp, bf2.get());
  EXPECT_TRUE(bf.get() == nullptr);
}

TEST(BufferedFileTest, MoveAssignment) {
  buffered_file bf = open_buffered_file();
  FILE* fp = bf.get();
  EXPECT_TRUE(fp != nullptr);
  buffered_file bf2;
  bf2 = std::move(bf);
  EXPECT_EQ(fp, bf2.get());
  EXPECT_TRUE(bf.get() == nullptr);
}

TEST(BufferedFileTest, MoveAssignmentClosesFile) {
  buffered_file bf = open_buffered_file();
  buffered_file bf2 = open_buffered_file();
  int old_fd = bf2.fileno();
  bf2 = std::move(bf);
  EXPECT_TRUE(isclosed(old_fd));
}

TEST(BufferedFileTest, MoveFromTemporaryInCtor) {
  FILE* fp = nullptr;
  buffered_file f(open_buffered_file(&fp));
  EXPECT_EQ(fp, f.get());
}

TEST(BufferedFileTest, MoveFromTemporaryInAssignment) {
  FILE* fp = nullptr;
  buffered_file f;
  f = open_buffered_file(&fp);
  EXPECT_EQ(fp, f.get());
}

TEST(BufferedFileTest, MoveFromTemporaryInAssignmentClosesFile) {
  buffered_file f = open_buffered_file();
  int old_fd = f.fileno();
  f = open_buffered_file();
  EXPECT_TRUE(isclosed(old_fd));
}

TEST(BufferedFileTest, CloseFileInDtor) {
  int fd = 0;
  {
    buffered_file f = open_buffered_file();
    fd = f.fileno();
  }
  EXPECT_TRUE(isclosed(fd));
}

TEST(BufferedFileTest, CloseErrorInDtor) {
  std::unique_ptr<buffered_file> f(new buffered_file(open_buffered_file()));
  EXPECT_WRITE(
      stderr,
      {
        // The close function must be called inside EXPECT_WRITE,
        // otherwise the system may recycle closed file descriptor when
        // redirecting the output in EXPECT_STDERR and the second close
        // will break output redirection.
        FMT_POSIX(close(f->fileno()));
        SUPPRESS_ASSERT(f.reset(nullptr));
      },
      format_system_error(EBADF, "cannot close file") + "\n");
}

TEST(BufferedFileTest, Close) {
  buffered_file f = open_buffered_file();
  int fd = f.fileno();
  f.close();
  EXPECT_TRUE(f.get() == nullptr);
  EXPECT_TRUE(isclosed(fd));
}

TEST(BufferedFileTest, CloseError) {
  buffered_file f = open_buffered_file();
  FMT_POSIX(close(f.fileno()));
  EXPECT_SYSTEM_ERROR_NOASSERT(f.close(), EBADF, "cannot close file");
  EXPECT_TRUE(f.get() == nullptr);
}

TEST(BufferedFileTest, Fileno) {
  buffered_file f;
#  ifndef __COVERITY__
  // fileno on a null FILE pointer either crashes or returns an error.
  // Disable Coverity because this is intentional.
  EXPECT_DEATH_IF_SUPPORTED(
      {
        try {
          f.fileno();
        } catch (const fmt::system_error&) {
          std::exit(1);
        }
      },
      "");
#  endif
  f = open_buffered_file();
  EXPECT_TRUE(f.fileno() != -1);
  file copy = file::dup(f.fileno());
  EXPECT_READ(copy, FILE_CONTENT);
}

TEST(OStreamTest, Move) {
  fmt::ostream out = fmt::output_file("test-file");
  fmt::ostream moved(std::move(out));
  moved.print("hello");
}

TEST(OStreamTest, Print) {
  fmt::ostream out = fmt::output_file("test-file");
  out.print("The answer is {}.\n", fmt::join(std::initializer_list<int>{42}, ", "));
  out.close();
  file in("test-file", file::RDONLY);
  EXPECT_READ(in, "The answer is 42.\n");
}

TEST(OStreamTest, BufferBoundary) {
  auto str = std::string(4096, 'x');
  fmt::ostream out = fmt::output_file("test-file");
  out.print("{}", str);
  out.print("{}", str);
  out.close();
  file in("test-file", file::RDONLY);
  EXPECT_READ(in, str + str);
}

TEST(OStreamTest, BufferSize) {
  fmt::ostream out = fmt::output_file("test-file", fmt::buffer_size=1);
  out.print("{}", "foo");
  out.close();
  file in("test-file", file::RDONLY);
  EXPECT_READ(in, "foo");
}

TEST(OStreamTest, Truncate) {
  {
    fmt::ostream out = fmt::output_file("test-file");
    out.print("0123456789");
  }
  {
    fmt::ostream out = fmt::output_file("test-file");
    out.print("foo");
  }
  file in("test-file", file::RDONLY);
  EXPECT_EQ("foo", read(in, 4));
}

TEST(FileTest, DefaultCtor) {
  file f;
  EXPECT_EQ(-1, f.descriptor());
}

TEST(FileTest, OpenBufferedFileInCtor) {
  FILE* fp = safe_fopen("test-file", "w");
  std::fputs(FILE_CONTENT, fp);
  std::fclose(fp);
  file f("test-file", file::RDONLY);
  ASSERT_TRUE(isopen(f.descriptor()));
}

TEST(FileTest, OpenBufferedFileError) {
  EXPECT_SYSTEM_ERROR(file("nonexistent", file::RDONLY), ENOENT,
                      "cannot open file nonexistent");
}

TEST(FileTest, MoveCtor) {
  file f = open_file();
  int fd = f.descriptor();
  EXPECT_NE(-1, fd);
  file f2(std::move(f));
  EXPECT_EQ(fd, f2.descriptor());
  EXPECT_EQ(-1, f.descriptor());
}

TEST(FileTest, MoveAssignment) {
  file f = open_file();
  int fd = f.descriptor();
  EXPECT_NE(-1, fd);
  file f2;
  f2 = std::move(f);
  EXPECT_EQ(fd, f2.descriptor());
  EXPECT_EQ(-1, f.descriptor());
}

TEST(FileTest, MoveAssignmentClosesFile) {
  file f = open_file();
  file f2 = open_file();
  int old_fd = f2.descriptor();
  f2 = std::move(f);
  EXPECT_TRUE(isclosed(old_fd));
}

static file OpenBufferedFile(int& fd) {
  file f = open_file();
  fd = f.descriptor();
  return f;
}

TEST(FileTest, MoveFromTemporaryInCtor) {
  int fd = 0xdead;
  file f(OpenBufferedFile(fd));
  EXPECT_EQ(fd, f.descriptor());
}

TEST(FileTest, MoveFromTemporaryInAssignment) {
  int fd = 0xdead;
  file f;
  f = OpenBufferedFile(fd);
  EXPECT_EQ(fd, f.descriptor());
}

TEST(FileTest, MoveFromTemporaryInAssignmentClosesFile) {
  int fd = 0xdead;
  file f = open_file();
  int old_fd = f.descriptor();
  f = OpenBufferedFile(fd);
  EXPECT_TRUE(isclosed(old_fd));
}

TEST(FileTest, CloseFileInDtor) {
  int fd = 0;
  {
    file f = open_file();
    fd = f.descriptor();
  }
  EXPECT_TRUE(isclosed(fd));
}

TEST(FileTest, CloseErrorInDtor) {
  std::unique_ptr<file> f(new file(open_file()));
  EXPECT_WRITE(
      stderr,
      {
        // The close function must be called inside EXPECT_WRITE,
        // otherwise the system may recycle closed file descriptor when
        // redirecting the output in EXPECT_STDERR and the second close
        // will break output redirection.
        FMT_POSIX(close(f->descriptor()));
        SUPPRESS_ASSERT(f.reset(nullptr));
      },
      format_system_error(EBADF, "cannot close file") + "\n");
}

TEST(FileTest, Close) {
  file f = open_file();
  int fd = f.descriptor();
  f.close();
  EXPECT_EQ(-1, f.descriptor());
  EXPECT_TRUE(isclosed(fd));
}

TEST(FileTest, CloseError) {
  file f = open_file();
  FMT_POSIX(close(f.descriptor()));
  EXPECT_SYSTEM_ERROR_NOASSERT(f.close(), EBADF, "cannot close file");
  EXPECT_EQ(-1, f.descriptor());
}

TEST(FileTest, Read) {
  file f = open_file();
  EXPECT_READ(f, FILE_CONTENT);
}

TEST(FileTest, ReadError) {
  file f("test-file", file::WRONLY);
  char buf;
  // We intentionally read from a file opened in the write-only mode to
  // cause error.
  EXPECT_SYSTEM_ERROR(f.read(&buf, 1), EBADF, "cannot read from file");
}

TEST(FileTest, Write) {
  file read_end, write_end;
  file::pipe(read_end, write_end);
  write(write_end, "test");
  write_end.close();
  EXPECT_READ(read_end, "test");
}

TEST(FileTest, WriteError) {
  file f("test-file", file::RDONLY);
  // We intentionally write to a file opened in the read-only mode to
  // cause error.
  EXPECT_SYSTEM_ERROR(f.write(" ", 1), EBADF, "cannot write to file");
}

TEST(FileTest, Dup) {
  file f = open_file();
  file copy = file::dup(f.descriptor());
  EXPECT_NE(f.descriptor(), copy.descriptor());
  EXPECT_EQ(FILE_CONTENT, read(copy, std::strlen(FILE_CONTENT)));
}

#  ifndef __COVERITY__
TEST(FileTest, DupError) {
  int value = -1;
  EXPECT_SYSTEM_ERROR_NOASSERT(file::dup(value), EBADF,
                               "cannot duplicate file descriptor -1");
}
#  endif

TEST(FileTest, Dup2) {
  file f = open_file();
  file copy = open_file();
  f.dup2(copy.descriptor());
  EXPECT_NE(f.descriptor(), copy.descriptor());
  EXPECT_READ(copy, FILE_CONTENT);
}

TEST(FileTest, Dup2Error) {
  file f = open_file();
  EXPECT_SYSTEM_ERROR_NOASSERT(
      f.dup2(-1), EBADF,
      fmt::format("cannot duplicate file descriptor {} to -1", f.descriptor()));
}

TEST(FileTest, Dup2NoExcept) {
  file f = open_file();
  file copy = open_file();
  error_code ec;
  f.dup2(copy.descriptor(), ec);
  EXPECT_EQ(ec.get(), 0);
  EXPECT_NE(f.descriptor(), copy.descriptor());
  EXPECT_READ(copy, FILE_CONTENT);
}

TEST(FileTest, Dup2NoExceptError) {
  file f = open_file();
  error_code ec;
  SUPPRESS_ASSERT(f.dup2(-1, ec));
  EXPECT_EQ(EBADF, ec.get());
}

TEST(FileTest, Pipe) {
  file read_end, write_end;
  file::pipe(read_end, write_end);
  EXPECT_NE(-1, read_end.descriptor());
  EXPECT_NE(-1, write_end.descriptor());
  write(write_end, "test");
  EXPECT_READ(read_end, "test");
}

TEST(FileTest, Fdopen) {
  file read_end, write_end;
  file::pipe(read_end, write_end);
  int read_fd = read_end.descriptor();
  EXPECT_EQ(read_fd, FMT_POSIX(fileno(read_end.fdopen("r").get())));
}

#  ifdef FMT_LOCALE
TEST(LocaleTest, Strtod) {
  fmt::locale loc;
  const char *start = "4.2", *ptr = start;
  EXPECT_EQ(4.2, loc.strtod(ptr));
  EXPECT_EQ(start + 3, ptr);
}
#  endif
#endif  // FMT_USE_FCNTL
