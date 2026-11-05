// Escape control characters for inspection and decode backslash escapes.
#ifndef FMT_BACKSLASH_H_
#define FMT_BACKSLASH_H_

#include <iterator>
#include <string>

#include "format.h"

FMT_BEGIN_NAMESPACE

namespace detail {

template <typename Char> FMT_CONSTEXPR auto backslash_hex_digit(Char c) -> int {
  if (c >= Char('0') && c <= Char('9')) return c - Char('0');
  if (c >= Char('a') && c <= Char('f')) return c - Char('a') + 10;
  if (c >= Char('A') && c <= Char('F')) return c - Char('A') + 10;
  return -1;
}

}  // namespace detail

/**
 * Writes text with ASCII controls escaped to out and returns the advanced
 * iterator. Uses `\a`, `\b`, `\e`, `\f`, `\n`, `\r`, `\t`, and `\v` for their
 * corresponding controls, and exactly two lowercase hexadecimal digits after
 * `\x` for remaining C0 controls and DEL. Backslashes and double quotes are
 * escaped too. Other code units are preserved verbatim; no surrounding quotes
 * are added. Supports embedded NUL through string views.
 * The output must not invalidate or overwrite unread input.
 */
template <typename OutputIt, typename Char>
FMT_CONSTEXPR auto backslash_escape_to(OutputIt out,
                                       basic_string_view<Char> input)
    -> OutputIt {
  for (auto c : input) {
    auto escape = Char{};
    switch (c) {
    case Char('\a'):   escape = Char('a'); break;
    case Char('\b'):   escape = Char('b'); break;
    case Char('\x1b'): escape = Char('e'); break;
    case Char('\f'):   escape = Char('f'); break;
    case Char('\n'):   escape = Char('n'); break;
    case Char('\r'):   escape = Char('r'); break;
    case Char('\t'):   escape = Char('t'); break;
    case Char('\v'):   escape = Char('v'); break;
    case Char('\\'):   escape = Char('\\'); break;
    case Char('"'):    escape = Char('"'); break;
    }
    if (escape != Char{}) {
      *out++ = Char('\\');
      *out++ = escape;
    } else if ((c >= Char{} && c < Char(' ')) || c == Char(127)) {
      const auto value = static_cast<int>(c);
      const char* hex = "0123456789abcdef";
      *out++ = Char('\\');
      *out++ = Char('x');
      *out++ = Char(hex[value / 16]);
      *out++ = Char(hex[value % 16]);
    } else {
      *out++ = c;
    }
  }
  return out;
}  // backslash_escape_to

/**
 * Writes decoded text to out and returns the advanced iterator.
 * Decodes `\a`, `\b`, `\e`, `\f`, `\n`, `\r`, `\t`, `\v`, `\\`, `\"`, `\'`,
 * `\?`, and `\x` followed by exactly two hex digits (either case).
 * Further digits are ordinary text. Also decodes one to three octal digits,
 * with values limited to 0 through 255. Unknown or malformed escapes,
 * including a trailing backslash, throw format_error. Output written before
 * an error is retained. Decoding is a single
 * pass: decoded backslashes do not introduce further escapes. No Unicode
 * escapes are recognized, and surrounding quotes are not removed.
 * The output must not invalidate or overwrite unread input.
 */
template <typename OutputIt, typename Char>
FMT_CONSTEXPR auto backslash_unescape_to(OutputIt out,
                                         basic_string_view<Char> input)
    -> OutputIt {
  while (input.size() != 0) {
    if (input[0] != Char('\\')) {
      *out++ = input[0];
      input.remove_prefix(1);
      continue;
    }
    if (input.size() < 2) fmt::report_error("incomplete backslash escape");
    auto c = Char{};
    auto length = std::size_t{2};
    if (input[1] >= Char('0') && input[1] <= Char('7')) {
      auto value = static_cast<int>(input[1] - Char('0'));
      while (length < 4 && length < input.size() &&
             input[length] >= Char('0') && input[length] <= Char('7')) {
        value = value * 8 + static_cast<int>(input[length] - Char('0'));
        ++length;
      }
      if (value > 255) fmt::report_error("octal escape is out of range");
      *out++ = static_cast<Char>(value);
      input.remove_prefix(length);
      continue;
    }
    switch (input[1]) {
    case Char('a'):  c = Char('\a'); break;
    case Char('b'):  c = Char('\b'); break;
    case Char('e'):  c = Char('\x1b'); break;
    case Char('f'):  c = Char('\f'); break;
    case Char('n'):  c = Char('\n'); break;
    case Char('r'):  c = Char('\r'); break;
    case Char('t'):  c = Char('\t'); break;
    case Char('v'):  c = Char('\v'); break;
    case Char('\\'): c = Char('\\'); break;
    case Char('"'):  c = Char('"'); break;
    case Char('\''): c = Char('\''); break;
    case Char('?'):  c = Char('?'); break;
    case Char('x'):  {
      const auto high =
          input.size() >= 4 ? detail::backslash_hex_digit(input[2]) : -1;
      const auto low =
          input.size() >= 4 ? detail::backslash_hex_digit(input[3]) : -1;
      if (high < 0 || low < 0) fmt::report_error("invalid hexadecimal escape");
      c = static_cast<Char>(high * 16 + low);
      length = 4;
      break;
    }
    default: fmt::report_error("unknown backslash escape");
    }
    *out++ = c;
    input.remove_prefix(length);
  }
  return out;
}  // backslash_unescape_to

// Fixed character-type overloads allow string literals and std::basic_string
// arguments without explicit template arguments.
template <typename OutputIt>
FMT_CONSTEXPR auto backslash_escape_to(OutputIt out, string_view input)
    -> OutputIt {
  return backslash_escape_to<OutputIt, char>(out, input);
}

template <typename OutputIt>
FMT_CONSTEXPR auto backslash_escape_to(OutputIt out,
                                       basic_string_view<wchar_t> input)
    -> OutputIt {
  return backslash_escape_to<OutputIt, wchar_t>(out, input);
}

template <typename OutputIt>
FMT_CONSTEXPR auto backslash_unescape_to(OutputIt out, string_view input)
    -> OutputIt {
  return backslash_unescape_to<OutputIt, char>(out, input);
}

template <typename OutputIt>
FMT_CONSTEXPR auto backslash_unescape_to(OutputIt out,
                                         basic_string_view<wchar_t> input)
    -> OutputIt {
  return backslash_unescape_to<OutputIt, wchar_t>(out, input);
}

/** Returns an owning string using the same escaping as backslash_escape_to. */
template <typename Char>
FMT_NODISCARD FMT_CONSTEXPR_STRING auto backslash_escape(
    basic_string_view<Char> input) -> std::basic_string<Char> {
  auto result = std::basic_string<Char>{};
  backslash_escape_to(std::back_inserter(result), input);
  return result;
}

/** Returns an owning string using the same decoding as backslash_unescape_to.
 */
template <typename Char>
FMT_NODISCARD FMT_CONSTEXPR_STRING auto backslash_unescape(
    basic_string_view<Char> input) -> std::basic_string<Char> {
  auto result = std::basic_string<Char>{};
  backslash_unescape_to(std::back_inserter(result), input);
  return result;
}

FMT_NODISCARD FMT_CONSTEXPR_STRING inline auto backslash_escape(
    string_view input) -> std::string {
  return backslash_escape<char>(input);
}

FMT_NODISCARD FMT_CONSTEXPR_STRING inline auto backslash_escape(
    basic_string_view<wchar_t> input) -> std::wstring {
  return backslash_escape<wchar_t>(input);
}

FMT_NODISCARD FMT_CONSTEXPR_STRING inline auto backslash_unescape(
    string_view input) -> std::string {
  return backslash_unescape<char>(input);
}

FMT_NODISCARD FMT_CONSTEXPR_STRING inline auto backslash_unescape(
    basic_string_view<wchar_t> input) -> std::wstring {
  return backslash_unescape<wchar_t>(input);
}

FMT_END_NAMESPACE

#endif  // FMT_BACKSLASH_H_
