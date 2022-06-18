// Formatting library for C++ - experimental range support
//
// {fmt} support for variant interface.

#ifndef FMT_VARIANT_H_
#define FMT_VARIANT_H_

#include <type_traits>

#include "format.h"
#include "ranges.h"

#define FMT_HAS_VARIANT FMT_CPLUSPLUS >= 201703L
#if FMT_HAS_VARIANT

#include <variant>

FMT_BEGIN_NAMESPACE

template <typename Char> struct formatter<std::monostate, Char> {
  template <typename ParseContext>
  FMT_CONSTEXPR auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  template <typename FormatContext = format_context>
  auto format(const std::monostate&, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    auto out = ctx.out();
    *out++ = ' ';
    return out;
  }
};

namespace detail {

template <typename T>
using variant_index_sequence = make_index_sequence<std::variant_size<T>::value>;

// variant_size and variant_alternative check.
template <typename T> class is_variant_like_ {
  template <typename U>
  static auto check(U* p) -> decltype(std::variant_size<U>::value, int());
  template <typename> static void check(...);

 public:
  static constexpr const bool value =
      !std::is_void<decltype(check<T>(nullptr))>::value;
};

// formattable element check
template <typename T, typename C, bool = is_variant_like_<T>::value>
class is_variant_formattable_ {
 public:
  static constexpr const bool value = false;
};
template <typename T, typename C> class is_variant_formattable_<T, C, true> {
  template <std::size_t... I>
  static std::integral_constant<
      bool,
      (is_formattable<std::variant_alternative_t<I, T>, C>::value && ...)>
      check(index_sequence<I...>);

 public:
  static constexpr const bool value =
      decltype(check(variant_index_sequence<T>{}))::value;
};

}  // namespace detail

template <typename T> struct is_variant_like {
  static constexpr const bool value = detail::is_variant_like_<T>::value;
};

template <typename T, typename C> struct is_variant_formattable {
  static constexpr const bool value =
      detail::is_variant_formattable_<T, C>::value;
};

template <typename VariantT, typename Char>
struct formatter<
    VariantT, Char,
    enable_if_t<fmt::is_variant_like<VariantT>::value &&
                fmt::is_variant_formattable<VariantT, Char>::value>> {
  template <typename ParseContext>
  FMT_CONSTEXPR auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  template <typename FormatContext = format_context>
  auto format(const VariantT& value, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    auto out = ctx.out();
    *out++ = '<';
    std::visit(
        [&](const auto& v) { out = detail::write_range_entry<Char>(out, v); },
        value);
    *out++ = '>';
    return out;
  }
};

FMT_END_NAMESPACE

#endif  // FMT_VARIANT_H_

#endif  // FMT_HAS_VARIANT
