/*
 Formatting library for C++ - standard container utilities

 Copyright (c) 2012 - 2016, Victor Zverovich
 All rights reserved.

 For the license information refer to format.h.
 */

#ifndef FMT_CONTAINER_H_
#define FMT_CONTAINER_H_

#include "format.h"

namespace fmt {

namespace internal {

/**
  \rst
  A "buffer" that appends data to a standard container (e.g. typically a
  ``std::vector`` or ``std::basic_string``).
  \endrst
 */
template <typename Container>
class ContainerBuffer : public Buffer<typename Container::value_type> {
 private:
  Container& container_;

 protected:
  virtual void grow(std::size_t size) FMT_OVERRIDE {
    container_.resize(size);
    this->ptr_ = &container_[0];
    this->capacity_ = size;
  }

 public:
  explicit ContainerBuffer(Container& container) : container_(container) {
    this->size_ = container_.size();
    if (this->size_ > 0) {
      this->ptr_ = &container_[0];
      this->capacity_ = this->size_;
    }
  }
};
}  // namespace internal

/**
  \rst
  This class template provides operations for formatting and appending data
  to a standard *container* like ``std::vector`` or ``std::basic_string``.

  **Example**::

    void vecformat(std::vector<char>& dest, fmt::BasicCStringRef<char> format,
                   fmt::ArgList args) {
      fmt::BasicContainerWriter<std::vector<char> > appender(dest);
      appender.write(format, args);
    }
    FMT_VARIADIC(void, vecformat, std::vector<char>&,
                 fmt::BasicCStringRef<char>);
  \endrst
 */
template <class Container>
class BasicContainerWriter
  : public BasicWriter<typename Container::value_type> {
 private:
  internal::ContainerBuffer<Container> buffer_;

 public:
  /**
    \rst
    Constructs a :class:`fmt::BasicContainerWriter` object.
    \endrst
   */
  explicit BasicContainerWriter(Container& dest)
  : BasicWriter<typename Container::value_type>(buffer_), buffer_(dest) {}
};

} // namespace fmt

#endif  // FMT_CONTAINER_H_
