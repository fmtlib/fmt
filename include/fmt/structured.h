#ifndef FMT_STRUCTURED_H
#define FMT_STRUCTURED_H

#include <string>
#include <variant>

#include "format.h"
#include "ranges.h"  // fixme dont rely on ranges?!

FMT_BEGIN_NAMESPACE

/**
 * Structure that holds reflection information about type T.
 * @tparam T
 */
template <typename T, typename Enable = void> struct reflection {
  static constexpr bool available = false;
};

// fixme probably the reflection should provide accessors to the data instead of
//  (name, value) pairs. i.e. NamedField may have an operator() that returns the
//  value of a field given the object the field belongs to.
template <typename T> struct NamedField {
  const std::string name;
  const T value;

  bool operator==(const NamedField<T>& other) const {
    return value == other.value && name == other.name;
  };
};

template <typename T> struct Extended { const T& value; };

/**
 * Formats objects as they would be written using designated initializers.
 *
 * E.g. it will output "Outer{.a=1, b=2, .inner=Inner{.x=3, .y=4, .z=5}}"
 *
 * @tparam C The character type used.
 */
template <typename C> struct CStyleFormatter {
  template <typename ParseContext> constexpr auto parse(ParseContext& ctx) {
    auto it = ctx.begin();
    if (*it != '}') throw format_error("configuration not yet supported");
    return it;
  }

  template <typename T, typename FormatContext>
  auto format(T const& t, FormatContext& ctx) {
    std::string name = reflection<T>::name();

    std::copy(name.begin(), name.end(), ctx.out());
    *ctx.out()++ = '{';
    using base = formatter<decltype(fmt::join(reflection<T>::fields(t), ", "))>;
    base{}.format(fmt::join(reflection<T>::fields(t), ", "), ctx);
    *ctx.out()++ = '}';
    return ctx.out();
  }
};

// fixme: Would probably be better to visit all name field elements directly in
//  the Extended formatter instead of relying on user space specialization
//  mechanism. That also will allow to reuse a configuration of the formatter
//  for nested structures.
/**
 * Format a field name/value pair as ".{name}={value}"
 * @tparam T
 */
template <typename T, typename Char>
struct fmt::formatter<NamedField<T>, Char> {
  template <typename FormatContext>
  auto format(NamedField<T> const& t, FormatContext& ctx) {
    *ctx.out()++ = '.';
    std::copy(t.name.begin(), t.name.end(), ctx.out());
    *ctx.out()++ = '=';
    if constexpr (reflection<T>::available) {
      return CStyleFormatter<Char>{}.format(t.value, ctx);
    } else {
      return fmt::formatter<T>{}.format(t.value, ctx);
    }
  }
};

namespace detail {
/**
 * Select a formatter matching the style requested in the format string.
 */
template <typename T, typename Char>
struct fallback_formatter<T, Char, enable_if_t<reflection<T>::available>> {
  FMT_CONSTEXPR auto parse(basic_format_parse_context<Char>& ctx)
      -> decltype(ctx.begin()) {
    auto it = ctx.begin(), end = ctx.end();
    if (*(it++) != 'e') {  // fixme: e for extended.. rather arbitrary choice...
      throw format_error("invalid format");
    }
    ctx.advance_to(it);
    it = CStyleFormatter<Char>{}.parse(ctx);

    // Check if reached the end of the range:
    if (it != end && *it != '}') {
      throw format_error("invalid format");
    }

    // Return an iterator past the end of the parsed range:
    return it;
  }

  template <typename FormatContext>
  auto format(const T& value, FormatContext& ctx) {
    return CStyleFormatter<Char>{}.format(value, ctx);
  }
};
}  // namespace detail

FMT_END_NAMESPACE

#endif  // FMT_STRUCTURED_H
