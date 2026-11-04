// Formatting library for C++ - styled std::ostream output
//
// Copyright (c) 2026 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_COLOR_OSTREAM_H_
#define FMT_COLOR_OSTREAM_H_

#include "color.h"
#include "ostream.h"
#include "xchar.h"

FMT_BEGIN_NAMESPACE
FMT_BEGIN_EXPORT

namespace detail {

template<typename Char> struct color_ostream_types {
  using string_view = basic_string_view<Char>;
  using format_args = basic_format_args<buffered_context<Char>>;
};

inline void write_color_buffer(std::ostream& os, buffer<char>& buf) {
  fmt::print(os, FMT_STRING("{}"), string_view(buf.data(), buf.size()));
}

template<typename Char>
void write_color_buffer(std::basic_ostream<Char>& os, buffer<Char>& buf) {
  write_buffer(os, buf);
}

} // detail

/**
 * Prints formatted text to `os` using `ts`.
 * A nonempty style precedes the text with SGR sequences and appends a reset
 * to terminal defaults. An empty style emits no SGR sequences.
 */
template<typename Char>
void vprint(std::basic_ostream<Char>& os, text_style ts,
            typename detail::color_ostream_types<Char>::string_view fmt,
            typename detail::color_ostream_types<Char>::format_args args)
{
  auto buffer = basic_memory_buffer<Char>();
  detail::vformat_to(buffer, ts, fmt, args);
  detail::write_color_buffer(os, buffer);
}

/**
 * Prints formatted text to `os` using `ts`, followed by a newline.
 * The newline follows the terminal reset for a nonempty style.
 */
template<typename Char>
void vprintln(std::basic_ostream<Char>& os, text_style ts,
              typename detail::color_ostream_types<Char>::string_view fmt,
              typename detail::color_ostream_types<Char>::format_args args)
{
  auto buffer = basic_memory_buffer<Char>();
  detail::vformat_to(buffer, ts, fmt, args);
  buffer.push_back(static_cast<Char>('\n'));
  detail::write_color_buffer(os, buffer);
}

/**
 * Prints formatted text to `os` using `ts`, followed by a terminal reset for
 * a nonempty style. Reset does not restore a previously active style.
 *
 * **Example**:
 *
 *     fmt::print(os, fmt::text_style{fmt::emphasis::bold}, "Result: {}", 42);
 */
template<typename... T>
void print(std::ostream& os, text_style ts, format_string<T...> fmt, T&&... args)
{ fmt::vprint(os, ts, fmt.get(), fmt::make_format_args(args...)); }

/// Prints styled formatted text to a wide stream.
template<typename... T>
void print(std::wostream& os, text_style ts, wformat_string<T...> fmt, T&&... args)
{ fmt::vprint(os, ts, fmt.get(), fmt::make_wformat_args(args...)); }

/**
 * Prints formatted text to `os` using `ts`, followed by a newline.
 * A nonempty style ends with a reset to terminal defaults before the newline.
 */
template<typename... T>
void println(std::ostream& os, text_style ts, format_string<T...> fmt,
             T&&... args)
{ fmt::vprintln(os, ts, fmt.get(), fmt::make_format_args(args...)); }

/// Prints styled formatted text to a wide stream, followed by a newline.
template<typename... T>
void println(std::wostream& os, text_style ts, wformat_string<T...> fmt,
             T&&... args)
{ fmt::vprintln(os, ts, fmt.get(), fmt::make_wformat_args(args...)); }

namespace detail {

template<typename T, typename Char>
auto operator<<(std::basic_ostream<Char>& os, const styled_arg<T>& arg)
  -> std::basic_ostream<Char>&
{
  if (arg.style.empty()) return os << arg.value;

  const auto sgr = to_sgr<Char>(arg.style);
  os.write(sgr.data(), static_cast<std::streamsize>(sgr.size()));
  constexpr auto reset = sgr_reset<Char>();
#if FMT_USE_EXCEPTIONS
  try {
#endif
    os << arg.value;
#if FMT_USE_EXCEPTIONS
  }
  catch (...) {
    try {
      os.write(reset.data(), static_cast<std::streamsize>(reset.size()));
    }
    catch (...) {
      // Reset is best-effort; preserve the original insertion exception.
    }
    throw;
  }
#endif
  return os.write(reset.data(), static_cast<std::streamsize>(reset.size()));
}

} // detail

FMT_END_EXPORT
FMT_END_NAMESPACE

#endif  // FMT_COLOR_OSTREAM_H_
