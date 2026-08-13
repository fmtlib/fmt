// Formatting library for C++ - formatting of enums
//
// Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_ENUM_H_
#define FMT_ENUM_H_

#include "format.h"

#if FMT_HAS_INCLUDE(<version>)
#  include <version>
#endif

#ifdef FMT_USE_REFLECTION
// Use the provided definition.
#elif defined(__cpp_impl_reflection) && defined(__cpp_lib_reflection)
#  define FMT_USE_REFLECTION 1
#else
#  define FMT_USE_REFLECTION 0
#endif

#if FMT_USE_REFLECTION && !defined(FMT_MODULE)
#  include <array>
#  include <meta>
#  include <utility>  // std::pair
#endif

FMT_BEGIN_NAMESPACE

#if FMT_USE_REFLECTION

/// The type of the `fmt::as_identifiers` annotation.
FMT_EXPORT struct as_identifiers_t {};

/**
 * An annotation that makes an enum format as identifiers of its enumerators.
 *
 * **Example**:
 *
 *     enum class [[=fmt::as_identifiers]] color { red, green, blue };
 *     auto s = fmt::format("{}", color::green);  // s == "green"
 *
 * A value that doesn't match any enumerator is represented as its underlying
 * value in decimal before applying string formatting.
 */
FMT_EXPORT inline constexpr auto as_identifiers = as_identifiers_t();

namespace detail {

// Returns true if T is an enum annotated with fmt::as_identifiers.
template <typename T, typename U = remove_cvref_t<T>>
consteval auto use_identifiers() -> bool {
  if constexpr (!std::is_enum<U>::value) {
    return false;
  } else {
    return !std::meta::annotations_of_with_type(^^U, ^^as_identifiers_t)
                .empty();
  }
}

template <typename E> consteval auto count_enumerators() -> size_t {
  return std::meta::enumerators_of(^^E).size();
}

template <typename E, size_t N = count_enumerators<E>()>
consteval auto make_identifiers() -> std::array<std::pair<E, string_view>, N> {
  auto ids = std::array<std::pair<E, string_view>, N>();
  auto i = size_t();
  for (std::meta::info e : std::meta::enumerators_of(^^E)) {
    auto id = std::meta::identifier_of(e);
    // identifier_of returns a view of a string with static storage duration.
    ids[i++] = {std::meta::extract<E>(e), string_view(id.data(), id.size())};
  }
  return ids;
}

// Identifiers of enumerators of E in the order of declaration.
template <typename E> inline constexpr auto identifiers = make_identifiers<E>();

// Returns the identifier of the first enumerator of E equal to value or an
// empty string view if there is no such enumerator.
template <typename E> constexpr auto identifier_of(E value) -> string_view {
  for (const auto& id : identifiers<E>) {
    if (id.first == value) return id.second;
  }
  return {};
}

}  // namespace detail

// A formatter for enums annotated with fmt::as_identifiers.
template <typename E>
struct formatter<E, char, enable_if_t<detail::use_identifiers<E>()>> {
 private:
  formatter<string_view> impl_;

 public:
  FMT_CONSTEXPR auto parse(parse_context<char>& ctx) -> const char* {
    return impl_.parse(ctx);
  }

  template <typename FormatContext>
  auto format(E value, FormatContext& ctx) const -> decltype(ctx.out()) {
    auto id = detail::identifier_of(value);
    if (id.size() != 0) return impl_.format(id, ctx);
    // Fall back to the underlying value if there is no matching enumerator.
    // Unary plus applies integral promotion so that `char` and `bool`
    // underlying types are written in decimal instead of as a character or
    // "true"/"false".
    auto buf = memory_buffer();
    detail::write<char>(appender(buf), +underlying(value));
    return impl_.format(string_view(buf.data(), buf.size()), ctx);
  }
};

#endif  // FMT_USE_REFLECTION

FMT_END_NAMESPACE

#endif  // FMT_ENUM_H_
