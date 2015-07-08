/*
 Test utilities.

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

fmt::BufferedFile open_buffered_file(FILE **fp) {
  fmt::File read_end, write_end;
  fmt::File::pipe(read_end, write_end);
  write_end.write(FILE_CONTENT, std::strlen(FILE_CONTENT));
  write_end.close();
  fmt::BufferedFile f = read_end.fdopen("r");
  if (fp)
    *fp = f.get();
  return f;
}
