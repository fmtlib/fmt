// Formatting library for C++ - formatters for standard library types
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_STD_H_
#define FMT_STD_H_

#include <thread>
#include <type_traits>
#include <utility>

#include "ostream.h"

#if FMT_HAS_INCLUDE(<version>)
#  include <version>
#endif

#ifdef __cpp_lib_filesystem
#  include <filesystem>

FMT_BEGIN_NAMESPACE

namespace detail {

template <typename Char>
void write_escaped_path(basic_memory_buffer<Char>& quoted,
                        const std::filesystem::path& p) {
  write_escaped_string<Char>(std::back_inserter(quoted), p.string<Char>());
}
#  ifdef _WIN32
template <>
inline void write_escaped_path<char>(basic_memory_buffer<char>& quoted,
                                     const std::filesystem::path& p) {
  auto s = p.u8string();
  write_escaped_string<char>(
      std::back_inserter(quoted),
      string_view(reinterpret_cast<const char*>(s.c_str()), s.size()));
}
#  endif
template <>
inline void write_escaped_path<std::filesystem::path::value_type>(
    basic_memory_buffer<std::filesystem::path::value_type>& quoted,
    const std::filesystem::path& p) {
  write_escaped_string<std::filesystem::path::value_type>(
      std::back_inserter(quoted), p.native());
}

}  // namespace detail

template <typename Char>
struct formatter<std::filesystem::path, Char>
    : formatter<basic_string_view<Char>> {
  template <typename FormatContext>
  auto format(const std::filesystem::path& p, FormatContext& ctx) const ->
      typename FormatContext::iterator {
    basic_memory_buffer<Char> quoted;
    detail::write_escaped_path(quoted, p);
    return formatter<basic_string_view<Char>>::format(
        basic_string_view<Char>(quoted.data(), quoted.size()), ctx);
  }
};
FMT_END_NAMESPACE
#endif

FMT_BEGIN_NAMESPACE
template <typename Char>
struct formatter<std::thread::id, Char> : basic_ostream_formatter<Char> {};
FMT_END_NAMESPACE

#ifdef __cpp_lib_variant
#  include <variant>
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
using variant_index_sequence =
    std::make_index_sequence<std::variant_size<T>::value>;

// variant_size and variant_alternative check.
template <typename T, typename U = void>
struct is_variant_like_ : std::false_type {};
template <typename T>
struct is_variant_like_<T, std::void_t<decltype(std::variant_size<T>::value)>>
    : std::true_type {};

// formattable element check
template <typename T, typename C> class is_variant_formattable_ {
  template <std::size_t... I>
  static std::conjunction<
      is_formattable<std::variant_alternative_t<I, T>, C>...>
      check(std::index_sequence<I...>);

 public:
  static constexpr const bool value =
      decltype(check(variant_index_sequence<T>{}))::value;
};

template <typename Char, typename OutputIt>
auto write_variant_alternative(OutputIt out, basic_string_view<Char> str)
    -> OutputIt {
  return write_escaped_string(out, str);
}

// Returns true if T has a std::string-like interface, like std::string_view.
template <typename T> class is_std_string_likevv {
  template <typename U>
  static auto check(U* p)
      -> decltype((void)p->find('a'), p->length(), (void)p->data(), int());
  template <typename> static void check(...);

 public:
  static constexpr const bool value =
      is_string<T>::value ||
      std::is_convertible<T, std_string_view<char>>::value ||
      !std::is_void<decltype(check<T>(nullptr))>::value;
};

template <typename Char>
struct is_std_string_likevv<fmt::basic_string_view<Char>> : std::true_type {};

template <typename Char, typename OutputIt, typename T,
          FMT_ENABLE_IF(std::is_convertible<T, std_string_view<char>>::value)>
inline auto write_variant_alternative(OutputIt out, const T& str) -> OutputIt {
  auto sv = std_string_view<Char>(str);
  return write_variant_alternative<Char>(out, basic_string_view<Char>(sv));
}

template <typename Char, typename OutputIt, typename Arg,
          FMT_ENABLE_IF(std::is_same<Arg, Char>::value)>
OutputIt write_variant_alternative(OutputIt out, const Arg v) {
  return write_escaped_char(out, v);
}

template <typename Char, typename OutputIt, typename Arg,
          FMT_ENABLE_IF(
              !is_std_string_likevv<typename std::decay<Arg>::type>::value &&
              !std::is_same<Arg, Char>::value)>
OutputIt write_variant_alternative(OutputIt out, const Arg& v) {
  return write<Char>(out, v);
}

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
    std::enable_if_t<std::conjunction_v<
        is_variant_like<VariantT>, is_variant_formattable<VariantT, Char>>>> {
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
        [&](const auto& v) {
          out = detail::write_variant_alternative<Char>(out, v);
        },
        value);
    *out++ = '>';
    return out;
  }
};
FMT_END_NAMESPACE
#endif

#endif  // FMT_STD_H_
