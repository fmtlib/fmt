// Formatting library for C++ - string utilities
//
// Copyright (c) 2012 - 2016, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_STRING_H_
#define FMT_STRING_H_

#include "fmt/format.h"

namespace fmt {

/**
  \rst
  This class template represents a character buffer backed by std::string.

  You can use one of the following typedefs for common character types
  and the standard allocator:

  +----------------+------------------------------+
  | Type           | Definition                   |
  +================+==============================+
  | string_buffer  | basic_string_buffer<char>    |
  +----------------+------------------------------+
  | wstring_buffer | basic_string_buffer<wchar_t> |
  +----------------+------------------------------+

  **Example**::

     string_buffer out;
     format_to(out, "The answer is {}", 42);

  This will write the following output to the ``out`` object:

  .. code-block:: none

     The answer is 42

  The output can be moved to an ``std::string`` with ``out.move_to()``.
  \endrst
 */template <typename Char>
class basic_string_buffer : public basic_buffer<Char> {
 private:
  std::basic_string<Char> str_;

 protected:
  virtual void grow(std::size_t capacity) {
    str_.resize(capacity);
    this->set(&str_[0], capacity);
  }

 public:
  /**
    \rst
    Moves the buffer content to *str* clearing the buffer.
    \endrst
   */
  void move_to(std::basic_string<Char> &str) {
    str_.resize(this->size());
    str.swap(str_);
    this->resize(0);
    this->set(0, 0);
  }
};

typedef basic_string_buffer<char> string_buffer;
typedef basic_string_buffer<wchar_t> wstring_buffer;

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
  string_buffer buf;
  basic_writer<buffer>(buf).write(value);
  std::string str;
  buf.move_to(str);
  return str;
}
}

#endif  // FMT_STRING_H_
