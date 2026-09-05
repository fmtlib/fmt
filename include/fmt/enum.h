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
#  include <string_view>
#  include <utility>  // std::pair
#endif

FMT_BEGIN_NAMESPACE

#if FMT_USE_REFLECTION

/**
 * An annotation that makes an enum format as identifiers of its enumerators.
 *
 * **Example**:
 *
 *     enum class [[=fmt::as_identifiers()]] color { red, green, blue };
 *     auto s = fmt::format("{}", color::green);  // s == "green"
 *
 * A value that doesn't match any enumerator is represented as its underlying
 * value in decimal before applying string formatting.
 */
FMT_EXPORT struct as_identifiers {};

namespace detail {

// Returns true if T is an enum annotated with fmt::as_identifiers.
template <typename T, typename U = remove_cvref_t<T>>
consteval auto use_identifiers() -> bool {
  if constexpr (!std::is_enum<U>::value) {
    return false;
  } else {
    return !std::meta::annotations_of_with_type(^^U, ^^as_identifiers).empty();
  }
}

// Returns the underlying value of `value` converted to uint64_t. Negative
// values wrap around, so the difference of two such values is the distance
// between them.
template <typename E> constexpr auto to_uint64(E value) -> uint64_t {
  return static_cast<uint64_t>(static_cast<underlying_t<E>>(value));
}

// Returns the smallest enumerator value of E or 0 if E has no enumerators.
template <typename E> consteval auto min_enumerator() -> E {
  auto enumerators = std::meta::enumerators_of(^^E);
  if (enumerators.empty()) return E();
  auto min = std::meta::extract<E>(enumerators[0]);
  for (std::meta::info e : enumerators) {
    auto value = std::meta::extract<E>(e);
    if (value < min) min = value;
  }
  return min;
}

// Returns the size of a table that maps the distance from the smallest
// enumerator value of E to an identifier, or 0 if the values are too sparse
// for such a table to be worthwhile.
template <typename E> consteval auto identifier_table_size() -> size_t {
  auto enumerators = std::meta::enumerators_of(^^E);
  if (enumerators.empty()) return 0;
  auto min = to_uint64(min_enumerator<E>());
  auto span = uint64_t();
  for (std::meta::info e : enumerators)
    span = max_of(span, to_uint64(std::meta::extract<E>(e)) - min);
  // Require the density of at least 70% to limit the size of the table.
  // Comparing with the limit instead of adding one to span avoids overflow.
  auto limit = uint64_t(enumerators.size()) * 10 / 7;
  return span < limit ? static_cast<size_t>(span) + 1 : 0;
}

template <typename E, size_t N = identifier_table_size<E>()>
consteval auto make_identifier_table() -> std::array<std::string_view, N> {
  auto ids = std::array<std::string_view, N>();
  auto min = to_uint64(min_enumerator<E>());
  for (std::meta::info e : std::meta::enumerators_of(^^E)) {
    auto i = static_cast<size_t>(to_uint64(std::meta::extract<E>(e)) - min);
    // identifier_of returns a view of a string with static storage duration so
    // it doesn't need to be copied. Keep the identifier of the first
    // enumerator with this value.
    if (ids[i].size() == 0) ids[i] = std::meta::identifier_of(e);
  }
  return ids;
}

// Identifiers of enumerators of E indexed by the distance from the smallest
// enumerator value with empty string views in the holes.
template <typename E>
inline constexpr auto identifier_table = make_identifier_table<E>();

// Returns the size of the hash table that maps enumerator values of E to
// identifiers. It is the smallest power of two that keeps the load factor at or
// below 0.5, which guarantees a free slot and therefore terminates the search.
template <typename E> consteval auto identifier_map_size() -> size_t {
  auto size = size_t(1);
  while (size < std::meta::enumerators_of(^^E).size() * 2) size *= 2;
  return size;
}

// Returns the index of the first slot to probe for `value`. The bits of the
// underlying value are mixed because enumerator values are usually small and
// only the low bits of the hash are used.
template <typename E, size_t N = identifier_map_size<E>()>
constexpr auto identifier_slot(E value) -> size_t {
  auto h = to_uint64(value);
  h ^= h >> 33;
  h *= 0xff51afd7ed558ccd;
  h ^= h >> 33;
  return static_cast<size_t>(h & (N - 1));
}

template <typename E, size_t N = identifier_map_size<E>()>
consteval auto make_identifier_map()
    -> std::array<std::pair<E, std::string_view>, N> {
  auto map = std::array<std::pair<E, std::string_view>, N>();
  for (std::meta::info e : std::meta::enumerators_of(^^E)) {
    auto value = std::meta::extract<E>(e);
    for (auto i = identifier_slot(value);; i = (i + 1) & (N - 1)) {
      if (map[i].second.size() != 0) {
        // Keep the identifier of the first enumerator with this value.
        if (map[i].first == value) break;
        continue;  // The slot is taken by another value; probe the next one.
      }
      map[i] = {value, std::meta::identifier_of(e)};
      break;
    }
  }
  return map;
}

// Identifiers of enumerators of E in an open-addressed hash table with linear
// probing and empty string views in the free slots.
template <typename E>
inline constexpr auto identifier_map = make_identifier_map<E>();

// Returns the identifier of the first enumerator of E equal to value or an
// empty string view if there is no such enumerator.
template <typename E>
constexpr auto identifier_of(E value) -> std::string_view {
  constexpr size_t table_size = identifier_table_size<E>();
  if constexpr (table_size != 0) {
    // Values outside of the table wrap around and are rejected by the check.
    auto i = to_uint64(value) - to_uint64(min_enumerator<E>());
    return i < table_size ? identifier_table<E>[static_cast<size_t>(i)]
                          : std::string_view();
  } else {
    constexpr size_t map_size = identifier_map_size<E>();
    // A free slot terminates the search and its empty identifier is the result.
    for (auto i = identifier_slot(value);; i = (i + 1) & (map_size - 1)) {
      const auto& entry = identifier_map<E>[i];
      if (entry.second.size() == 0 || entry.first == value) return entry.second;
    }
  }
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
