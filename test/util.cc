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
