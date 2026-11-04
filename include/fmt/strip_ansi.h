// Remove syntactically complete ESC-prefixed ANSI control sequences.
#ifndef FMT_STRIP_ANSI_H_
#define FMT_STRIP_ANSI_H_

#include "styled_text.h"
#include <string>

FMT_BEGIN_NAMESPACE
namespace detail {
struct ansi_escape_scan {
  std::size_t end;
  bool complete;
};

template<typename Char>
auto scan_ansi_escape(basic_string_view<Char> input, std::size_t start)
    -> ansi_escape_scan {
  auto pos = start + 1;
  if (pos == input.size()) return {pos, false};
  const auto kind = input[pos];
  if (kind == Char(']') || kind == Char('P') || kind == Char('X') ||
      kind == Char('^') || kind == Char('_')) {
    ++pos;
    while (pos < input.size()) {
      if (kind == Char(']') && input[pos] == Char('\a')) return {pos + 1, true};
      if (input[pos] == Char('\x18') || input[pos] == Char('\x1a'))
        return {pos, false}; // CAN/SUB cancel the control string.
      if (input[pos] == Char('\x1b') && pos + 1 < input.size() &&
          input[pos + 1] == Char('\\')) return {pos + 2, true};
      ++pos;
    }
    // An unterminated string is preserved whole, including embedded escapes.
    return {pos, false};
  }
  if (kind == Char('[')) {
    ++pos;
    // CSI: parameters, then intermediates, then a final byte. The numeric
    // meaning is irrelevant; private/unknown commands have the same grammar.
    while (pos < input.size() && input[pos] >= Char('0') && input[pos] <= Char('?')) ++pos;
    while (pos < input.size() && input[pos] >= Char(' ') && input[pos] <= Char('/')) ++pos;
    if (pos < input.size() && input[pos] >= Char('@') && input[pos] <= Char('~'))
      return {pos + 1, true};
    return {pos, false};
  }
  // Other ESC sequences: zero or more intermediates and a final byte.
  while (pos < input.size() && input[pos] >= Char(' ') && input[pos] <= Char('/')) ++pos;
  if (pos < input.size() && input[pos] >= Char('0') && input[pos] <= Char('~'))
    return {pos + 1, true};
  return {pos, false};
}
} // namespace detail

/**
 * Returns an owning copy with well-formed ESC-prefixed ANSI sequences removed,
 * including private or unknown commands. Handles CSI, OSC (BEL or ST terminated),
 * DCS/SOS/PM/APC (ST terminated), and ordinary ESC sequences. Standalone C0 and
 * single-byte C1 controls are retained, as are malformed/incomplete sequences.
 * Control-string payloads and their terminators are removed with the sequence.
 * This removes controls; it does not simulate terminal editing or sanitize text.
 */
template<typename Char>
FMT_NODISCARD auto strip_ansi(basic_string_view<Char> input)
    -> std::basic_string<Char> {
  auto result = std::basic_string<Char>{};
  for (std::size_t pos = 0; pos < input.size();) {
    if (input[pos] != Char('\x1b')) {
      const auto start = pos++;
      while (pos < input.size() && input[pos] != Char('\x1b')) ++pos;
      result.append(input.data() + start, pos - start);
      continue;
    }
    const auto scanned = detail::scan_ansi_escape(input, pos);
    if (!scanned.complete) result.append(input.data() + pos, scanned.end - pos);
    pos = scanned.end;
  }
  return result;
}

FMT_NODISCARD inline auto strip_ansi(string_view input) -> std::string
{ return strip_ansi<char>(input); }

FMT_NODISCARD inline auto strip_ansi(basic_string_view<wchar_t> input) -> std::wstring
{ return strip_ansi<wchar_t>(input); }

namespace detail {
struct styled_text_ansi_stripper {
  template<typename Char>
  static void strip(basic_styled_text<Char>& text) {
    if (text._is_branch) {
      for (auto& child : text._data.children) strip(child);
    } else if (text._data.text) {
      auto stripped = fmt::strip_ansi<Char>(basic_string_view<Char>{
          text._data.text->data(), text._data.text->size()});
      if (stripped == *text._data.text) return;
      if (stripped.empty())
        text._data.text.reset();
      else
        text._data.text = std::make_shared<const std::basic_string<Char>>(
            std::move(stripped));
    }
  }
};
} // namespace detail

/**
 * Returns an owning copy with ANSI sequences removed independently from each
 * leaf. Preserves all node styles, absolute boundaries, and tree structure,
 * including leaves that become empty. Sequences spanning leaves are retained.
 * Does not modify the input; unchanged leaf strings remain shared.
 */
template<typename Char>
FMT_NODISCARD auto strip_ansi(basic_styled_text<Char> input)
    -> basic_styled_text<Char> {
  detail::styled_text_ansi_stripper::strip(input);
  return input;
}

FMT_END_NAMESPACE
#endif // FMT_STRIP_ANSI_H_
