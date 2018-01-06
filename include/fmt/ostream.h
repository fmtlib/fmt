// Formatting library for C++ - std::ostream support
//
// Copyright (c) 2012 - 2016, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_OSTREAM_H_
#define FMT_OSTREAM_H_

#include "fmt/format.h"
#include <ostream>

namespace fmt {

namespace internal {

template <class Char>
class FormatBuf : public std::basic_streambuf<Char> {
 private:
  typedef typename std::basic_streambuf<Char>::int_type int_type;
  typedef typename std::basic_streambuf<Char>::traits_type traits_type;

  basic_buffer<Char> &buffer_;
  Char *start_;

 public:
  FormatBuf(basic_buffer<Char> &buffer) : buffer_(buffer), start_(&buffer[0]) {
    this->setp(start_, start_ + buffer_.capacity());
  }

  int_type overflow(int_type ch = traits_type::eof()) {
    if (!traits_type::eq_int_type(ch, traits_type::eof())) {
      size_t buf_size = size();
      buffer_.resize(buf_size);
      buffer_.reserve(buf_size * 2);

      start_ = &buffer_[0];
      start_[buf_size] = traits_type::to_char_type(ch);
      this->setp(start_+ buf_size + 1, start_ + buf_size * 2);
    }
    return ch;
  }

  size_t size() const {
    return to_unsigned(this->pptr() - start_);
  }
};

yes &convert(std::ostream &);

struct DummyStream : std::ostream {
  DummyStream();  // Suppress a bogus warning in MSVC.
  // Hide all operator<< overloads from std::ostream.
  void operator<<(null<>);
};

no &operator<<(std::ostream &, int);

template<typename T>
struct convert_to_int_impl<T, true> {
  // Convert to int only if T doesn't have an overloaded operator<<.
  enum {
    value = sizeof(convert(std::declval<DummyStream&>() << std::declval<T>()))
            == sizeof(no)
  };
};

// Write the content of buf to os.
template <typename Char>
void write(std::basic_ostream<Char> &os, basic_buffer<Char> &buf) {
  const Char *data = buf.data();
  typedef std::make_unsigned<std::streamsize>::type UnsignedStreamSize;
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

template <typename Char, typename T>
void format_value(basic_buffer<Char> &buffer, const T &value) {
  internal::FormatBuf<Char> format_buf(buffer);
  std::basic_ostream<Char> output(&format_buf);
  output << value;
  buffer.resize(format_buf.size());
}

// Disable builtin formatting of enums and use operator<< instead.
template <typename T>
struct format_enum<T,
    typename std::enable_if<std::is_enum<T>::value>::type> : std::false_type {};
}  // namespace internal

// Formats an object of type T that has an overloaded ostream operator<<.
template <typename T, typename Char>
struct formatter<T, Char,
    typename std::enable_if<!internal::format_type<T>::value>::type>
    : formatter<basic_string_view<Char>, Char> {

  template <typename Context>
  void format(const T &value, Context &ctx) {
    basic_memory_buffer<Char> buffer;
    internal::format_value(buffer, value);
    basic_string_view<Char> str(buffer.data(), buffer.size());
    formatter<basic_string_view<Char>, Char>::format(str, ctx);
  }
};

inline void vprint(std::ostream &os, string_view format_str, format_args args) {
  memory_buffer buffer;
  vformat_to(buffer, format_str, args);
  internal::write(os, buffer);
}

/**
  \rst
  Prints formatted data to the stream *os*.

  **Example**::

    print(cerr, "Don't {}!", "panic");
  \endrst
 */
template <typename... Args>
inline void print(std::ostream &os, string_view format_str,
                  const Args & ... args) {
  vprint(os, format_str, make_args(args...));
}
}  // namespace fmt

#endif  // FMT_OSTREAM_H_
