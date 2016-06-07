/*
 Formatting library for C++ - std::ostream support

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

#include "fmt/ostream.h"
#include "fmt/printf.h"

namespace fmt {

namespace {
// Write the content of w to os.
void write(std::ostream &os, Writer &w) {
  const char *data = w.data();
  typedef internal::MakeUnsigned<std::streamsize>::Type UnsignedStreamSize;
  UnsignedStreamSize size = w.size();
  UnsignedStreamSize max_size =
      internal::to_unsigned((std::numeric_limits<std::streamsize>::max)());
  do {
    UnsignedStreamSize n = size <= max_size ? size : max_size;
    os.write(data, static_cast<std::streamsize>(n));
    data += n;
    size -= n;
  } while (size != 0);
}
}

FMT_FUNC void print(std::ostream &os, CStringRef format_str, ArgList args) {
  MemoryWriter w;
  w.write(format_str, args);
  write(os, w);
}

FMT_FUNC int fprintf(std::ostream &os, CStringRef format, ArgList args) {
  MemoryWriter w;
  printf(w, format, args);
  write(os, w);
  return static_cast<int>(w.size());
}
}  // namespace fmt
