// Formatting library for C++ - test utilities
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include <cstdarg>
#include <cstdio>
#include <string>

#include "fmt/posix.h"

enum {BUFFER_SIZE = 256};

#ifdef _MSC_VER
# define FMT_VSNPRINTF vsprintf_s
#else
# define FMT_VSNPRINTF vsnprintf
#endif

template <std::size_t SIZE>
void safe_sprintf(char (&buffer)[SIZE], const char *format, ...) {
  std::va_list args;
  va_start(args, format);
  FMT_VSNPRINTF(buffer, SIZE, format, args);
  va_end(args);
}

// Increment a number in a string.
void increment(char *s);

std::string get_system_error(int error_code);

extern const char *const FILE_CONTENT;

// Opens a buffered file for reading.
fmt::buffered_file open_buffered_file(FILE **fp = FMT_NULL);

inline FILE *safe_fopen(const char *filename, const char *mode) {
#if defined(_WIN32) && !defined(__MINGW32__)
  // Fix MSVC warning about "unsafe" fopen.
  FILE *f = 0;
  errno = fopen_s(&f, filename, mode);
  return f;
#else
  return std::fopen(filename, mode);
#endif
}

template <typename Char>
class BasicTestString {
 private:
  std::basic_string<Char> value_;

  static const Char EMPTY[];

 public:
  explicit BasicTestString(const Char *value = EMPTY) : value_(value) {}

  const std::basic_string<Char> &value() const { return value_; }
};

template <typename Char>
const Char BasicTestString<Char>::EMPTY[] = {0};

typedef BasicTestString<char> TestString;
typedef BasicTestString<wchar_t> TestWString;

template <typename Char>
std::basic_ostream<Char> &operator<<(
    std::basic_ostream<Char> &os, const BasicTestString<Char> &s) {
  os << s.value();
  return os;
}

class Date {
  int year_, month_, day_;
 public:
  Date(int year, int month, int day) : year_(year), month_(month), day_(day) {}

  int year() const { return year_; }
  int month() const { return month_; }
  int day() const { return day_; }
};
