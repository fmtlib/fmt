/*
 Formatting library for C++ - std::ostream support

 Copyright (c) 2012 - 2016, Victor Zverovich
 All rights reserved.

 For the license information refer to format.h.
 */

#include "fmt/ostream.h"

namespace fmt {

namespace internal {
FMT_FUNC void write(std::ostream &os, buffer &buf) {
  const char *data = buf.data();
  typedef internal::make_unsigned<std::streamsize>::type UnsignedStreamSize;
  UnsignedStreamSize size = buf.size();
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

FMT_FUNC void vprint(std::ostream &os, string_view format_str, args args) {
  memory_buffer buffer;
  vformat_to(buffer, format_str, args);
  internal::write(os, buffer);
}
}  // namespace fmt
