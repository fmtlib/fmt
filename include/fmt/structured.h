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

// fixme: Would probably be better to visit all name field elements directly in
//  the Extended formatter instead of relying on user space specialization
//  mechanism
/**
 * Format a field name/value pair as ".{name}={value}"
 * @tparam T
 */
template <typename T> struct fmt::formatter<NamedField<T>> {
  template <typename FormatContext>
  auto format(NamedField<T> const& t, FormatContext& ctx) {
    *ctx.out()++ = '.';
    std::copy(t.name.begin(), t.name.end(), ctx.out());
    *ctx.out()++ = '=';
    if constexpr (reflection<T>::available) {
      return fmt::formatter<Extended<T>>{}.format(t.value, ctx);
    } else {
      return fmt::formatter<T>{}.format(t.value, ctx);
    }
  }
};

/**
 * Formats objects as they would be written using designated initializers.
 *
 * E.g. {:e} will output Outer{.a=1, b=2, .inner=Inner{.x=3, .y=4, .z=5}}
 *
 * @tparam T The object to format.
 * @tparam C The character type used.
 */
// fixme remove T template parameter
template <typename T, typename C>
struct fmt::formatter<Extended<T>, C,
                      std::enable_if_t<reflection<T>::available, void>> {
  template <typename ParseContext> constexpr auto parse(ParseContext& ctx) {
    auto it = ctx.begin();
    if (*it != '}') throw format_error("configuration not yet supported");
    return it;
  }

  template <typename FormatContext>
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

// fixme remove this class and merge it with the fallback formatter in detail
// namespace
template <typename T, typename Char>
struct fmt::formatter<T, Char,
                      std::enable_if_t<reflection<T>::available, void>> {
  using extended = fmt::formatter<Extended<T>>;

  template <typename ParseContext> constexpr auto parse(ParseContext& ctx) {
    // Parse the presentation format and store it in the formatter:
    auto it = ctx.begin(), end = ctx.end();
    if (*(it++) != 'e') {
      throw format_error("invalid format");
    }
    _extended = extended{};
    ctx.advance_to(it);
    _extended.parse(ctx);

    // Check if reached the end of the range:
    if (it != end && *it != '}') {
      throw format_error("invalid format");
    }

    // Return an iterator past the end of the parsed range:
    return it;
  }

  template <typename FormatContext>
  auto format(T const& t, FormatContext& ctx) {
    return _extended.format(t, ctx);
  }

 private:
  extended _extended;
};

namespace detail {
template <typename T, typename Char>
struct fallback_formatter<T, Char, enable_if_t<reflection<T>::available>> {
  using extended = fmt::formatter<Extended<T>>;

  FMT_CONSTEXPR auto parse(basic_format_parse_context<Char>& ctx)
      -> decltype(ctx.begin()) {
    auto it = formatter<basic_string_view<Char>, Char>::parse(ctx);
    // fixme: not sure what to return, if the closing '}' remains (like required
    // for formatter specialization)
    //        some internals will throw from ErrorHandler::on_error("invalid
    //        type specifier");
    ctx.advance_to(it++);  // fixme: also not sure
    return it;
  }

  template <typename FormatContext>
  auto format(const T& value, FormatContext& ctx) {
    return extended{}.format(value, ctx);
  }
};
}  // namespace detail

FMT_END_NAMESPACE

#endif  // FMT_STRUCTURED_H
