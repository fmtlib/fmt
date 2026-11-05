// Parse ordinary ANSI SGR into owned styled text.
#ifndef FMT_PARSE_STYLED_TEXT_H_
#define FMT_PARSE_STYLED_TEXT_H_

#include "styled_text.h"

FMT_BEGIN_NAMESPACE

namespace detail {

template <typename Char>
auto sgr_number(basic_string_view<Char> input, unsigned& value) -> bool {
  value = 0;
  for (auto c : input) {
    if (c < Char('0') || c > Char('9')) return false;
    const auto digit = static_cast<unsigned>(c - Char('0'));
    if (value > ((std::numeric_limits<unsigned>::max)() - digit) / 10)
      return false;
    value = value * 10 + digit;
  }
  return true;
}  // sgr_number

template <typename Char> struct sgr_fields {
  basic_string_view<Char> input{};
  std::size_t position{0};

  explicit sgr_fields(basic_string_view<Char> value) : input{value} {}

  auto next(basic_string_view<Char>& field) -> bool {
    if (position > input.size()) return false;
    const auto start = position;
    while (position < input.size() && input[position] != Char(';')) ++position;
    field = {input.data() + start, position - start};
    ++position;
    return true;
  }

  auto number(unsigned& value) -> bool {
    auto field = basic_string_view<Char>{};
    return next(field) && sgr_number(field, value);
  }
};  // sgr_fields

inline auto sgr_index_color(unsigned index) -> color_t {
  if (index < 16) return static_cast<term_color>(index);
  if (index < 232) return cube_color{index - 16};
  return gray_color{static_cast<int>(index - 232)};
}

template <typename Char>
auto apply_sgr(basic_string_view<Char> parameters, text_style& style) -> bool {
  // Validate atomically before the caller commits the resulting style.
  for (auto c : parameters) {
    if ((c < Char('0') || c > Char('9')) && c != Char(';') && c != Char(':'))
      return false;
  }
  auto fields = sgr_fields<Char>{parameters};
  auto field = basic_string_view<Char>{};
  while (fields.next(field)) {
    auto colon = std::size_t{0};
    while (colon < field.size() && field[colon] != Char(':')) ++colon;
    unsigned code = 0;
    if (!sgr_number(basic_string_view<Char>{field.data(), colon}, code))
      return false;
    if (colon != field.size()) {
      // Colon subparameters belong to this field, never to following
      // attributes.
      unsigned values[6] = {};
      std::size_t count = 0, start = 0;
      bool too_many = false;
      for (std::size_t i = 0; i <= field.size(); ++i) {
        if (i != field.size() && field[i] != Char(':')) continue;
        unsigned value = 0;
        if (!sgr_number(
                basic_string_view<Char>{field.data() + start, i - start},
                value)) {
          return false;
        }
        if (count < 6)
          values[count] = value;
        else
          too_many = true;
        ++count;
        start = i + 1;
      }
      if (code != 38 && code != 48) continue;
      auto color = color_t{};
      if (!too_many && count == 3 && values[1] == 5) {
        if (values[2] > 255) return false;
        color = sgr_index_color(values[2]);
      } else if (!too_many && values[1] == 2 && (count == 5 || count == 6)) {
        if (count == 6 && values[2] != 0) continue;  // Unsupported color space.
        const auto offset = count - 3;
        if (values[offset] > 255 || values[offset + 1] > 255 ||
            values[offset + 2] > 255) {
          return false;
        }
        color = rgb{static_cast<int>(values[offset]),
                    static_cast<int>(values[offset + 1]),
                    static_cast<int>(values[offset + 2])};
      } else {
        if (values[1] == 2 || values[1] == 5) return false;
        continue;
      }
      if (code == 38)
        style.fg(color);
      else
        style.bg(color);
      continue;
    }
    if (code == 38 || code == 48) {
      unsigned mode = 0, a = 0, b = 0, c = 0;
      if (!fields.number(mode)) return false;
      auto color = color_t{};
      if (mode == 5) {
        if (!fields.number(a) || a > 255) return false;
        color = sgr_index_color(a);
      } else if (mode == 2) {
        if (!fields.number(a) || !fields.number(b) || !fields.number(c) ||
            a > 255 || b > 255 || c > 255) {
          return false;
        }
        color =
            rgb{static_cast<int>(a), static_cast<int>(b), static_cast<int>(c)};
      } else {
        // Unknown extended-color syntax has no known payload boundary.
        // Ignore the remainder rather than interpreting color data as emphasis.
        while (fields.next(field)) {
          unsigned ignored = 0;
          if (!sgr_number(field, ignored)) return false;
        }
        return true;
      }
      if (code == 38)
        style.fg(color);
      else
        style.bg(color);
      continue;
    }
    switch (code) {
    case 0:  style.clear().absolute(); break;
    case 1:  style.bold(); break;
    case 2:  style.faint(); break;
    case 3:  style.italic(); break;
    case 4:  style.underline(); break;
    case 5:  style.blink(); break;
    case 7:  style.reverse(); break;
    case 8:  style.conceal(); break;
    case 9:  style.strike(); break;
    case 22: style.bold(false).faint(false); break;
    case 23: style.italic(false); break;
    case 24: style.underline(false); break;
    case 25: style.blink(false); break;
    case 27: style.reverse(false); break;
    case 28: style.conceal(false); break;
    case 29: style.strike(false); break;
    case 39: style.fg(no_color); break;
    case 49: style.bg(no_color); break;
    default:
      if (code >= 30 && code <= 37)
        style.fg(static_cast<term_color>(code - 30));
      else if (code >= 90 && code <= 97)
        style.fg(static_cast<term_color>(code - 90 + 8));
      else if (code >= 40 && code <= 47)
        style.bg(static_cast<term_color>(code - 40));
      else if (code >= 100 && code <= 107)
        style.bg(static_cast<term_color>(code - 100 + 8));
      break;
    }
  }
  return true;
}  // apply_sgr

// Preserve non-SGR escape sequences as opaque text, including OSC/DCS payloads.
template <typename Char>
auto sgr_escape_end(basic_string_view<Char> input, std::size_t start)
    -> std::size_t {
  auto pos = start + 1;
  if (pos == input.size()) return pos;
  const auto kind = input[pos++];
  if (kind == Char(']') || kind == Char('P') || kind == Char('X') ||
      kind == Char('^') || kind == Char('_')) {
    while (pos < input.size()) {
      if (kind == Char(']') && input[pos] == Char('\a')) return pos + 1;
      if (input[pos] == Char('\x1b') && pos + 1 < input.size() &&
          input[pos + 1] == Char('\\')) {
        return pos + 2;
      }
      ++pos;
    }
    return pos;
  }
  if (kind == Char('[')) {
    while (pos < input.size()) {
      const auto c = input[pos];
      if (c >= Char('@') && c <= Char('~')) return pos + 1;
      if (c < Char(' ') || c > Char('?')) return pos;
      ++pos;
    }
    return pos;
  }
  if (kind >= Char(' ') && kind <= Char('/')) {
    while (pos < input.size() && input[pos] >= Char(' ') &&
           input[pos] <= Char('/')) {
      ++pos;
    }
    if (pos < input.size() && input[pos] >= Char('0') &&
        input[pos] <= Char('~')) {
      ++pos;
    }
  }
  return pos;
}  // sgr_escape_end

}  // namespace detail

/**
 * Decodes standard ESC[...m SGR into owned text spans. Consecutive sequences
 * update one style. The first reset starts an absolute group for subsequent
 * text; later resets clear styles within that group. Text before reset remains
 * relative. No unconditional absolute root is imposed.
 * Text and non-SGR escapes are retained
 * verbatim. Malformed SGR is retained atomically; unknown valid parameters are
 * ignored. This parses appearance, not an exact escape-byte round trip.
 */
template <typename Char>
FMT_NODISCARD auto parse_styled_text(basic_string_view<Char> input)
    -> basic_styled_text<Char> {
  auto result = basic_styled_text<Char>{};
  auto group = basic_styled_text<Char>{};
  bool in_absolute_group = false;
  auto style = text_style{};
  auto text = std::basic_string<Char>{};
  auto ts = text_style{};
  const auto flush = [&]() {
    if (text.empty()) return;
    auto& destination = in_absolute_group ? group : result;
    destination += basic_styled_text<Char>{std::move(text), ts};
    text.clear();
  };
  const auto append = [&](const Char* data, std::size_t size) {
    if (style.is_absolute()) {
      if (!in_absolute_group) {
        flush();  // Keep any earlier text outside the reset's scope.
        in_absolute_group = true;
      }
      style.absolute(
          false);  // The group supplies the defaults for all children.
    }
    if (!text.empty() && ts != style) flush();
    if (text.empty()) ts = style;
    text.append(data, size);
  };
  for (std::size_t pos = 0; pos < input.size();) {
    if (input[pos] != Char('\x1b')) {
      const auto start = pos++;
      while (pos < input.size() && input[pos] != Char('\x1b')) ++pos;
      append(input.data() + start, pos - start);
      continue;
    }
    const auto end = detail::sgr_escape_end(input, pos);
    auto next = style;
    if (end >= pos + 3 && input[pos + 1] == Char('[') &&
        input[end - 1] == Char('m') &&
        detail::apply_sgr(
            basic_string_view<Char>{input.data() + pos + 2, end - pos - 3},
            next)) {
      style = next;
    } else {
      append(input.data() + pos, end - pos);
    }
    pos = end;
  }
  flush();
  if (!group.empty()) {
    group.style(fmt::absolute(group.style()));
    if (result.empty()) return group;
    result += std::move(group);
  }
  return result;
}  // parse_styled_text

FMT_NODISCARD inline auto parse_styled_text(string_view input) -> styled_text {
  return parse_styled_text<char>(input);
}

FMT_NODISCARD inline auto parse_styled_text(basic_string_view<wchar_t> input)
    -> wstyled_text {
  return parse_styled_text<wchar_t>(input);
}

FMT_END_NAMESPACE

#endif  // FMT_PARSE_STYLED_TEXT_H_
