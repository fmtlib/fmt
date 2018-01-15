// Formatting library for C++ - time formatting
//
// Copyright (c) 2012 - 2016, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_TIME_H_
#define FMT_TIME_H_

#include "fmt/format.h"
#include <ctime>

namespace fmt {

template <>
struct formatter<std::tm> {
  template <typename ParseContext>
  auto parse(ParseContext &ctx) -> decltype(ctx.begin()) {
    auto it = internal::null_terminating_iterator<char>(ctx);
    if (*it == ':')
      ++it;
    auto end = it;
    while (*end && *end != '}')
      ++end;
    tm_format.reserve(end - it + 1);
    using internal::pointer_from;
    tm_format.append(pointer_from(it), pointer_from(end));
    tm_format.push_back('\0');
    return pointer_from(end);
  }

  auto format(const std::tm &tm, context &ctx) -> decltype(ctx.begin()) {
    internal::buffer &buf = internal::get_container(ctx.begin());
    std::size_t start = buf.size();
    for (;;) {
      std::size_t size = buf.capacity() - start;
      std::size_t count = std::strftime(&buf[start], size, &tm_format[0], &tm);
      if (count != 0) {
        buf.resize(start + count);
        break;
      }
      if (size >= tm_format.size() * 256) {
        // If the buffer is 256 times larger than the format string, assume
        // that `strftime` gives an empty result. There doesn't seem to be a
        // better way to distinguish the two cases:
        // https://github.com/fmtlib/fmt/issues/367
        break;
      }
      const std::size_t MIN_GROWTH = 10;
      buf.reserve(buf.capacity() + (size > MIN_GROWTH ? size : MIN_GROWTH));
    }
    return ctx.begin();
  }

  memory_buffer tm_format;
};
}

#endif  // FMT_TIME_H_
