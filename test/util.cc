// Formatting library for C++ - test utilities
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "util.h"
#include <cstring>

void increment(char *s) {
  for (int i = static_cast<int>(std::strlen(s)) - 1; i >= 0; --i) {
    if (s[i] != '9') {
      ++s[i];
      break;
    }
    s[i] = '0';
  }
}

std::string get_system_error(int error_code) {
#if defined(__MINGW32__) || !defined(_WIN32)
  return strerror(error_code);
#else
  enum { BUFFER_SIZE = 200 };
  char buffer[BUFFER_SIZE];
  if (strerror_s(buffer, BUFFER_SIZE, error_code))
    throw std::exception("strerror_s failed");
  return buffer;
#endif
}

const char *const FILE_CONTENT = "Don't panic!";

fmt::buffered_file open_buffered_file(FILE **fp) {
  fmt::file read_end, write_end;
  fmt::file::pipe(read_end, write_end);
  write_end.write(FILE_CONTENT, std::strlen(FILE_CONTENT));
  write_end.close();
  fmt::buffered_file f = read_end.fdopen("r");
  if (fp)
    *fp = f.get();
  return f;
}
