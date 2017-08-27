/*
 Formatting library for C++ - string utilities

 Copyright (c) 2012 - 2016, Victor Zverovich
 All rights reserved.

 For the license information refer to format.h.
 */

#ifdef FMT_INCLUDE
# error "Add the fmt's parent directory and not fmt itself to includes."
#endif

#ifndef FMT_STRING_H_
#define FMT_STRING_H_

#include "format.h"

namespace fmt {

namespace internal {

// A buffer that stores data in ``std::basic_string``.
template <typename Char, typename Allocator = std::allocator<Char> >
class StringBuffer : public Buffer<Char> {
 public:
  typedef std::basic_string<Char, std::char_traits<Char>, Allocator> StringType;

 private:
  StringType data_;

 protected:
  virtual void grow(std::size_t size) FMT_OVERRIDE {
    data_.resize(size);
    this->ptr_ = &data_[0];
    this->capacity_ = size;
  }

 public:
  explicit StringBuffer(const Allocator &allocator = Allocator())
  : data_(allocator) {}

  // Moves the data to ``str`` clearing the buffer.
  void move_to(StringType &str) {
    data_.resize(this->size_);
    str.swap(data_);
    this->capacity_ = this->size_ = 0;
    this->ptr_ = FMT_NULL;
  }
};
}  // namespace internal

/**
  \rst
  This class template provides operations for formatting and writing data
  into a character stream. The output is stored in a ``std::basic_string``
  that grows dynamically.

  You can use one of the following typedefs for common character types
  and the standard allocator:

  +---------------+----------------------------+
  | Type          | Definition                 |
  +===============+============================+
  | StringWriter  | BasicStringWriter<char>    |
  +---------------+----------------------------+
  | WStringWriter | BasicStringWriter<wchar_t> |
  +---------------+----------------------------+

  **Example**::

     StringWriter out;
     out << "The answer is " << 42 << "\n";

  This will write the following output to the ``out`` object:

  .. code-block:: none

     The answer is 42

  The output can be moved to a ``std::basic_string`` with ``out.move_to()``.
  \endrst
 */
template <typename Char, typename Allocator = std::allocator<Char> >
class BasicStringWriter : public BasicWriter<Char> {
 private:
  internal::StringBuffer<Char, Allocator> buffer_;

 public:
  /**
    \rst
    Constructs a :class:`fmt::BasicStringWriter` object.
    \endrst
   */
  explicit BasicStringWriter(const Allocator &allocator = Allocator())
  : BasicWriter<Char>(buffer_), buffer_(allocator) {}

  /**
    \rst
    Moves the buffer content to *str* clearing the buffer.
    \endrst
   */
  void move_to(std::basic_string<Char, std::char_traits<Char>, Allocator> &str) {
    buffer_.move_to(str);
  }
};

typedef BasicStringWriter<char> StringWriter;
typedef BasicStringWriter<wchar_t> WStringWriter;

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

/**
  \rst
  Converts *value* to ``std::wstring`` using the default format for type *T*.

  **Example**::

    #include "fmt/string.h"

    std::wstring answer = fmt::to_wstring(42);
  \endrst
 */
template <typename T>
std::wstring to_wstring(const T &value) {
  fmt::WMemoryWriter w;
  w << value;
  return w.str();
}
}

#endif  // FMT_STRING_H_
