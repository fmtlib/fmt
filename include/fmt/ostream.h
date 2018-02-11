// Formatting library for C++ - std::ostream support
//
// Copyright (c) 2012 - 2016, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_OSTREAM_H_
#define FMT_OSTREAM_H_

#include "format.h"
#include <ostream>

namespace fmt {

namespace internal {

template <class Char>
class FormatBuf : public std::basic_streambuf<Char> {
 private:
  typedef typename std::basic_streambuf<Char>::int_type int_type;
  typedef typename std::basic_streambuf<Char>::traits_type traits_type;

  basic_buffer<Char> &buffer_;

 public:
  FormatBuf(basic_buffer<Char> &buffer) : buffer_(buffer) {}

 protected:
  // The put-area is actually always empty. This makes the implementation
  // simpler and has the advantage that the streambuf and the buffer are always
  // in sync and sputc never writes into uninitialized memory. The obvious
  // disadvantage is that each call to sputc always results in a (virtual) call
  // to overflow. There is no disadvantage here for sputn since this always
  // results in a call to xsputn.

  int_type overflow(int_type ch = traits_type::eof()) FMT_OVERRIDE {
    if (!traits_type::eq_int_type(ch, traits_type::eof()))
      buffer_.push_back(static_cast<Char>(ch));
    return ch;
  }

  std::streamsize xsputn(const Char *s, std::streamsize count) FMT_OVERRIDE {
    buffer_.append(s, s + count);
    return count;
  }
};

struct test_stream : std::ostream {
 private:
  struct null;
  // Hide all operator<< from std::ostream.
  void operator<<(null);
};

// Disable conversion to int if T has an overloaded operator<< which is a free
// function (not a member of std::ostream).
template <typename T>
class convert_to_int<T, true> {
 private:
  template <typename U>
  static decltype(
    std::declval<test_stream&>() << std::declval<U>(), std::true_type())
      test(int);

  template <typename>
  static std::false_type test(...);

 public:
  static const bool value = !decltype(test<T>(0))::value;
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
  output.exceptions(std::ios_base::failbit | std::ios_base::badbit);
  output << value;
  buffer.resize(buffer.size());
}

// Disable builtin formatting of enums and use operator<< instead.
template <typename T>
struct format_enum<T,
    typename std::enable_if<std::is_enum<T>::value>::type> : std::false_type {};
}  // namespace internal

// Formats an object of type T that has an overloaded ostream operator<<.
template <typename T, typename Char>
struct formatter<T, Char,
    typename std::enable_if<!internal::format_type<
      typename buffer_context<Char>::type, T>::value>::type>
    : formatter<basic_string_view<Char>, Char> {

  template <typename Context>
  auto format(const T &value, Context &ctx) -> decltype(ctx.begin()) {
    basic_memory_buffer<Char> buffer;
    internal::format_value(buffer, value);
    basic_string_view<Char> str(buffer.data(), buffer.size());
    formatter<basic_string_view<Char>, Char>::format(str, ctx);
    return ctx.begin();
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
