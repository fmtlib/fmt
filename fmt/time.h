/*
 Formatting library for C++ - time formatting

 Copyright (c) 2012 - 2016, Victor Zverovich
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

#ifndef FMT_TIME_H_
#define FMT_TIME_H_

#include "fmt/format.h"
#include <ctime>

namespace fmt {
template <typename ArgFormatter>
void format(BasicFormatter<char, ArgFormatter> &f,
            const char *&format_str, const std::tm &tm) {
  if (*format_str == ':')
    ++format_str;
  const char *end = format_str;
  while (*end && *end != '}')
    ++end;
  if (*end != '}')
    FMT_THROW(FormatError("missing '}' in format string"));
  internal::MemoryBuffer<char, internal::INLINE_BUFFER_SIZE> format;
  format.append(format_str, end + 1);
  format[format.size() - 1] = '\0';
  Buffer<char> &buffer = f.writer().buffer();
  std::size_t start = buffer.size();
  for (;;) {
    std::size_t size = buffer.capacity() - start;
    std::size_t count = std::strftime(&buffer[start], size, &format[0], &tm);
    if (count != 0) {
      buffer.resize(start + count);
      break;
    }
    const std::size_t MIN_GROWTH = 10;
    buffer.reserve(buffer.capacity() + size > MIN_GROWTH ? size : MIN_GROWTH);
  }
  format_str = end + 1;
}
}

#endif  // FMT_TIME_H_
