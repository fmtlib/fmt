/*
 Formatting library for C++ - string utilities

 Copyright (c) 2012 - 2016, Victor Zverovich
 All rights reserved.

 For the license information refer to format.h.
 */

#ifndef FMT_STRING_H_
#define FMT_STRING_H_

#include "fmt/format.h"

namespace fmt {

/**
  \rst
  Converts *value* to ``std::string`` using the default format for type *T*.

  **Example**::

    #include "fmt/string.h"

    std::string answer = fmt::to_string(42);
  \endrst
 */
template <typename T>
std::string to_string(const T &value) {
  fmt::MemoryWriter w;
  w << value;
  return w.str();
}
}

#endif  // FMT_STRING_H_
