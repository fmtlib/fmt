// Formatting library for C++ - test utilities
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "util.h"

#include <cstring>

const char* const file_content = "Don't panic!";

fmt::buffered_file open_buffered_file(FILE** fp) {
#if FMT_USE_FCNTL
  fmt::file read_end, write_end;
  fmt::file::pipe(read_end, write_end);
  write_end.write(file_content, std::strlen(file_content));
  write_end.close();
  fmt::buffered_file f = read_end.fdopen("r");
  if (fp) *fp = f.get();
#else
  fmt::buffered_file f("test-file", "w");
  fputs(file_content, f.get());
  if (fp) *fp = f.get();
#endif
  return f;
}

std::locale do_get_locale(const char* name) {
  try {
    return std::locale(name);
  } catch (const std::runtime_error&) {
  }
  return std::locale::classic();
}

std::locale get_locale(const char* name, const char* alt_name) {
  auto loc = do_get_locale(name);
  if (loc == std::locale::classic() && alt_name) {
    loc = do_get_locale(alt_name);
  }
  if (loc == std::locale::classic())
    fmt::print(stderr, "{} locale is missing.\n", name);
  return loc;
}
