// Formatting library for C++ - std::ostream support
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_OSTREAM_H_
#define FMT_OSTREAM_H_

#include <ostream>

#include "format.h"

FMT_BEGIN_NAMESPACE

template <typename OutputIt, typename Char> class basic_printf_context;

namespace detail {

template <class Char> class formatbuf : public std::basic_streambuf<Char> {
 private:
  using int_type = typename std::basic_streambuf<Char>::int_type;
  using traits_type = typename std::basic_streambuf<Char>::traits_type;

  buffer<Char>& buffer_;

 public:
  explicit formatbuf(buffer<Char>& buf) : buffer_(buf) {}

 protected:
  // The put area is always empty. This makes the implementation simpler and has
  // the advantage that the streambuf and the buffer are always in sync and
  // sputc never writes into uninitialized memory. A disadvantage is that each
  // call to sputc always results in a (virtual) call to overflow. There is no
  // disadvantage here for sputn since this always results in a call to xsputn.

  auto overflow(int_type ch = traits_type::eof()) -> int_type override {
    if (!traits_type::eq_int_type(ch, traits_type::eof()))
      buffer_.push_back(static_cast<Char>(ch));
    return ch;
  }

  auto xsputn(const Char* s, std::streamsize count)
      -> std::streamsize override {
    buffer_.append(s, s + count);
    return count;
  }
};

// Checks if T has a user-defined operator<<.
template <typename T, typename Char, typename Enable = void>
class is_streamable {
 private:
  template <typename U>
  static auto test(int)
      -> bool_constant<sizeof(std::declval<std::basic_ostream<Char>&>()
                              << std::declval<U>()) != 0>;

  template <typename> static auto test(...) -> std::false_type;

  using result = decltype(test<T>(0));

 public:
  is_streamable() = default;

  static const bool value = result::value;
};

// Formatting of built-in types and arrays is intentionally disabled because
// it's handled by standard (non-ostream) formatters.
template <typename T, typename Char>
struct is_streamable<
    T, Char,
    enable_if_t<
        std::is_arithmetic<T>::value || std::is_array<T>::value ||
        std::is_pointer<T>::value || std::is_same<T, char8_type>::value ||
        (std::is_convertible<T, int>::value && !std::is_enum<T>::value)>>
    : std::false_type {};

// Write the content of buf to os.
// It is a separate function rather than a part of vprint to simplify testing.
template <typename Char>
void write_buffer(std::basic_ostream<Char>& os, buffer<Char>& buf) {
  const Char* buf_data = buf.data();
  using unsigned_streamsize = std::make_unsigned<std::streamsize>::type;
  unsigned_streamsize size = buf.size();
  unsigned_streamsize max_size = to_unsigned(max_value<std::streamsize>());
  do {
    unsigned_streamsize n = size <= max_size ? size : max_size;
    os.write(buf_data, static_cast<std::streamsize>(n));
    buf_data += n;
    size -= n;
  } while (size != 0);
}

template <typename Char, typename T>
void format_value(buffer<Char>& buf, const T& value,
                  locale_ref loc = locale_ref()) {
  auto&& format_buf = formatbuf<Char>(buf);
  auto&& output = std::basic_ostream<Char>(&format_buf);
#if !defined(FMT_STATIC_THOUSANDS_SEPARATOR)
  if (loc) output.imbue(loc.get<std::locale>());
#endif
  output << value;
  output.exceptions(std::ios_base::failbit | std::ios_base::badbit);
  buf.try_resize(buf.size());
}

// Formats an object of type T that has an overloaded ostream operator<<.
template <typename T, typename Char>
struct fallback_formatter<T, Char, enable_if_t<is_streamable<T, Char>::value>>
    : private formatter<basic_string_view<Char>, Char> {
  using formatter<basic_string_view<Char>, Char>::parse;

  template <typename OutputIt>
  auto format(const T& value, basic_format_context<OutputIt, Char>& ctx)
      -> OutputIt {
    auto buffer = basic_memory_buffer<Char>();
    format_value(buffer, value, ctx.locale());
    return formatter<basic_string_view<Char>, Char>::format(
        {buffer.data(), buffer.size()}, ctx);
  }

  // DEPRECATED!
  template <typename OutputIt>
  auto format(const T& value, basic_printf_context<OutputIt, Char>& ctx)
      -> OutputIt {
    auto buffer = basic_memory_buffer<Char>();
    format_value(buffer, value, ctx.locale());
    return std::copy(buffer.begin(), buffer.end(), ctx.out());
  }
};
}  // namespace detail

FMT_MODULE_EXPORT
template <typename Char>
void vprint(std::basic_ostream<Char>& os, basic_string_view<Char> format_str,
            basic_format_args<buffer_context<type_identity_t<Char>>> args) {
  auto buffer = basic_memory_buffer<Char>();
  detail::vformat_to(buffer, format_str, args);
  detail::write_buffer(os, buffer);
}

/**
  \rst
  Prints formatted data to the stream *os*.

  **Example**::

    fmt::print(cerr, "Don't {}!", "panic");
  \endrst
 */
FMT_MODULE_EXPORT
template <typename S, typename... Args,
          typename Char = enable_if_t<detail::is_string<S>::value, char_t<S>>>
void print(std::basic_ostream<Char>& os, const S& format_str, Args&&... args) {
  vprint(os, to_string_view(format_str),
         fmt::make_args_checked<Args...>(format_str, args...));
}
FMT_END_NAMESPACE

#endif  // FMT_OSTREAM_H_
