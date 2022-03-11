// Formatting library for C++ - experimental range support
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.
//
// Copyright (c) 2018 - present, Remotion (Igor Schulz)
// All Rights Reserved
// {fmt} support for ranges, containers and types tuple interface.

#ifndef FMT_RANGES_H_
#define FMT_RANGES_H_

#include <initializer_list>
#include <tuple>
#include <type_traits>

#include "format.h"

FMT_BEGIN_NAMESPACE

namespace detail {

template <typename RangeT, typename OutputIterator>
OutputIterator copy(const RangeT& range, OutputIterator out) {
  for (auto it = range.begin(), end = range.end(); it != end; ++it)
    *out++ = *it;
  return out;
}

template <typename OutputIterator>
OutputIterator copy(const char* str, OutputIterator out) {
  while (*str) *out++ = *str++;
  return out;
}

template <typename OutputIterator>
OutputIterator copy(char ch, OutputIterator out) {
  *out++ = ch;
  return out;
}

template <typename OutputIterator>
OutputIterator copy(wchar_t ch, OutputIterator out) {
  *out++ = ch;
  return out;
}

// Returns true if T has a std::string-like interface, like std::string_view.
template <typename T> class is_std_string_like {
  template <typename U>
  static auto check(U* p)
      -> decltype((void)p->find('a'), p->length(), (void)p->data(), int());
  template <typename> static void check(...);

 public:
  static FMT_CONSTEXPR_DECL const bool value =
      is_string<T>::value ||
      std::is_convertible<T, std_string_view<char>>::value ||
      !std::is_void<decltype(check<T>(nullptr))>::value;
};

template <typename Char>
struct is_std_string_like<fmt::basic_string_view<Char>> : std::true_type {};

template <typename T> class is_map {
  template <typename U> static auto check(U*) -> typename U::mapped_type;
  template <typename> static void check(...);

 public:
#ifdef FMT_FORMAT_MAP_AS_LIST
  static FMT_CONSTEXPR_DECL const bool value = false;
#else
  static FMT_CONSTEXPR_DECL const bool value =
      !std::is_void<decltype(check<T>(nullptr))>::value;
#endif
};

template <typename T> class is_set {
  template <typename U> static auto check(U*) -> typename U::key_type;
  template <typename> static void check(...);

 public:
#ifdef FMT_FORMAT_SET_AS_LIST
  static FMT_CONSTEXPR_DECL const bool value = false;
#else
  static FMT_CONSTEXPR_DECL const bool value =
      !std::is_void<decltype(check<T>(nullptr))>::value && !is_map<T>::value;
#endif
};

template <typename... Ts> struct conditional_helper {};

template <typename T, typename _ = void> struct is_range_ : std::false_type {};

#if !FMT_MSC_VER || FMT_MSC_VER > 1800

#  define FMT_DECLTYPE_RETURN(val)  \
    ->decltype(val) { return val; } \
    static_assert(                  \
        true, "")  // This makes it so that a semicolon is required after the
                   // macro, which helps clang-format handle the formatting.

// C array overload
template <typename T, std::size_t N>
auto range_begin(const T (&arr)[N]) -> const T* {
  return arr;
}
template <typename T, std::size_t N>
auto range_end(const T (&arr)[N]) -> const T* {
  return arr + N;
}

template <typename T, typename Enable = void>
struct has_member_fn_begin_end_t : std::false_type {};

template <typename T>
struct has_member_fn_begin_end_t<T, void_t<decltype(std::declval<T>().begin()),
                                           decltype(std::declval<T>().end())>>
    : std::true_type {};

// Member function overload
template <typename T>
auto range_begin(T&& rng) FMT_DECLTYPE_RETURN(static_cast<T&&>(rng).begin());
template <typename T>
auto range_end(T&& rng) FMT_DECLTYPE_RETURN(static_cast<T&&>(rng).end());

// ADL overload. Only participates in overload resolution if member functions
// are not found.
template <typename T>
auto range_begin(T&& rng)
    -> enable_if_t<!has_member_fn_begin_end_t<T&&>::value,
                   decltype(begin(static_cast<T&&>(rng)))> {
  return begin(static_cast<T&&>(rng));
}
template <typename T>
auto range_end(T&& rng) -> enable_if_t<!has_member_fn_begin_end_t<T&&>::value,
                                       decltype(end(static_cast<T&&>(rng)))> {
  return end(static_cast<T&&>(rng));
}

template <typename T, typename Enable = void>
struct has_const_begin_end : std::false_type {};
template <typename T, typename Enable = void>
struct has_mutable_begin_end : std::false_type {};

template <typename T>
struct has_const_begin_end<
    T,
    void_t<
        decltype(detail::range_begin(std::declval<const remove_cvref_t<T>&>())),
        decltype(detail::range_end(std::declval<const remove_cvref_t<T>&>()))>>
    : std::true_type {};

template <typename T>
struct has_mutable_begin_end<
    T, void_t<decltype(detail::range_begin(std::declval<T>())),
              decltype(detail::range_end(std::declval<T>())),
              enable_if_t<std::is_copy_constructible<T>::value>>>
    : std::true_type {};

template <typename T>
struct is_range_<T, void>
    : std::integral_constant<bool, (has_const_begin_end<T>::value ||
                                    has_mutable_begin_end<T>::value)> {};
#  undef FMT_DECLTYPE_RETURN
#endif

// tuple_size and tuple_element check.
template <typename T> class is_tuple_like_ {
  template <typename U>
  static auto check(U* p) -> decltype(std::tuple_size<U>::value, int());
  template <typename> static void check(...);

 public:
  static FMT_CONSTEXPR_DECL const bool value =
      !std::is_void<decltype(check<T>(nullptr))>::value;
};

// Check for integer_sequence
#if defined(__cpp_lib_integer_sequence) || FMT_MSC_VER >= 1900
template <typename T, T... N>
using integer_sequence = std::integer_sequence<T, N...>;
template <size_t... N> using index_sequence = std::index_sequence<N...>;
template <size_t N> using make_index_sequence = std::make_index_sequence<N>;
#else
template <typename T, T... N> struct integer_sequence {
  using value_type = T;

  static FMT_CONSTEXPR size_t size() { return sizeof...(N); }
};

template <size_t... N> using index_sequence = integer_sequence<size_t, N...>;

template <typename T, size_t N, T... Ns>
struct make_integer_sequence : make_integer_sequence<T, N - 1, N - 1, Ns...> {};
template <typename T, T... Ns>
struct make_integer_sequence<T, 0, Ns...> : integer_sequence<T, Ns...> {};

template <size_t N>
using make_index_sequence = make_integer_sequence<size_t, N>;
#endif

template <class Tuple, class F, size_t... Is>
void for_each(index_sequence<Is...>, Tuple&& tup, F&& f) noexcept {
  using std::get;
  // using free function get<I>(T) now.
  const int _[] = {0, ((void)f(get<Is>(tup)), 0)...};
  (void)_;  // blocks warnings
}

template <class T>
FMT_CONSTEXPR make_index_sequence<std::tuple_size<T>::value> get_indexes(
    T const&) {
  return {};
}

template <class Tuple, class F> void for_each(Tuple&& tup, F&& f) {
  const auto indexes = get_indexes(tup);
  for_each(indexes, std::forward<Tuple>(tup), std::forward<F>(f));
}

#if FMT_MSC_VER
// Older MSVC doesn't get the reference type correctly for arrays.
template <typename R> struct range_reference_type_impl {
  using type = decltype(*detail::range_begin(std::declval<R&>()));
};

template <typename T, std::size_t N> struct range_reference_type_impl<T[N]> {
  using type = T&;
};

template <typename T>
using range_reference_type = typename range_reference_type_impl<T>::type;
#else
template <typename Range>
using range_reference_type =
    decltype(*detail::range_begin(std::declval<Range&>()));
#endif

// We don't use the Range's value_type for anything, but we do need the Range's
// reference type, with cv-ref stripped.
template <typename Range>
using uncvref_type = remove_cvref_t<range_reference_type<Range>>;

template <typename OutputIt> OutputIt write_delimiter(OutputIt out) {
  *out++ = ',';
  *out++ = ' ';
  return out;
}

template <typename Char, typename OutputIt>
auto write_range_entry(OutputIt out, basic_string_view<Char> str) -> OutputIt {
  return write_escaped_string(out, str);
}

template <typename Char, typename OutputIt, typename T,
          FMT_ENABLE_IF(std::is_convertible<T, std_string_view<char>>::value)>
inline auto write_range_entry(OutputIt out, const T& str) -> OutputIt {
  auto sv = std_string_view<Char>(str);
  return write_range_entry<Char>(out, basic_string_view<Char>(sv));
}

template <typename Char, typename OutputIt, typename Arg,
          FMT_ENABLE_IF(std::is_same<Arg, Char>::value)>
OutputIt write_range_entry(OutputIt out, const Arg v) {
  return write_escaped_char(out, v);
}

template <
    typename Char, typename OutputIt, typename Arg,
    FMT_ENABLE_IF(!is_std_string_like<typename std::decay<Arg>::type>::value &&
                  !std::is_same<Arg, Char>::value)>
OutputIt write_range_entry(OutputIt out, const Arg& v) {
  return write<Char>(out, v);
}

}  // namespace detail

template <typename T> struct is_tuple_like {
  static FMT_CONSTEXPR_DECL const bool value =
      detail::is_tuple_like_<T>::value && !detail::is_range_<T>::value;
};

template <typename TupleT, typename Char>
struct formatter<TupleT, Char, enable_if_t<fmt::is_tuple_like<TupleT>::value>> {
 private:
  // C++11 generic lambda for format().
  template <typename FormatContext> struct format_each {
    template <typename T> void operator()(const T& v) {
      if (i > 0) out = detail::write_delimiter(out);
      out = detail::write_range_entry<Char>(out, v);
      ++i;
    }
    int i;
    typename FormatContext::iterator& out;
  };

 public:
  template <typename ParseContext>
  FMT_CONSTEXPR auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  template <typename FormatContext = format_context>
  auto format(const TupleT& values, FormatContext& ctx) const
      -> decltype(ctx.out()) {
    auto out = ctx.out();
    *out++ = '(';
    detail::for_each(values, format_each<FormatContext>{0, out});
    *out++ = ')';
    return out;
  }
};

template <typename T, typename Char> struct is_range {
  static FMT_CONSTEXPR_DECL const bool value =
      detail::is_range_<T>::value && !detail::is_std_string_like<T>::value &&
      !detail::is_map<T>::value &&
      !std::is_convertible<T, std::basic_string<Char>>::value &&
      !std::is_constructible<detail::std_string_view<Char>, T>::value;
};

namespace detail {
template <typename Context> struct range_mapper {
  using mapper = arg_mapper<Context>;

  template <typename T,
            FMT_ENABLE_IF(has_formatter<remove_cvref_t<T>, Context>::value)>
  static auto map(T&& value) -> T&& {
    return static_cast<T&&>(value);
  }
  template <typename T,
            FMT_ENABLE_IF(!has_formatter<remove_cvref_t<T>, Context>::value)>
  static auto map(T&& value)
      -> decltype(mapper().map(static_cast<T&&>(value))) {
    return mapper().map(static_cast<T&&>(value));
  }
};

template <typename Char, typename Element>
using range_formatter_type = conditional_t<
    is_formattable<Element, Char>::value,
    formatter<remove_cvref_t<decltype(range_mapper<buffer_context<Char>>{}.map(
                  std::declval<Element>()))>,
              Char>,
    fallback_formatter<Element, Char>>;

template <typename R>
using maybe_const_range =
    conditional_t<has_const_begin_end<R>::value, const R, R>;
}  // namespace detail

template <typename R, typename Char>
struct formatter<
    R, Char,
    enable_if_t<
        fmt::is_range<R, Char>::value
// Workaround a bug in MSVC 2019 and earlier.
#if !FMT_MSC_VER
        &&
        (is_formattable<detail::uncvref_type<detail::maybe_const_range<R>>,
                        Char>::value ||
         detail::has_fallback_formatter<
             detail::uncvref_type<detail::maybe_const_range<R>>, Char>::value)
#endif
        >> {

  using range_type = detail::maybe_const_range<R>;
  using formatter_type =
      detail::range_formatter_type<Char, detail::uncvref_type<range_type>>;
  formatter_type underlying_;
  bool custom_specs_ = false;

  template <typename ParseContext>
  FMT_CONSTEXPR auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    auto it = ctx.begin();
    auto end = ctx.end();
    if (it == end || *it == '}') return it;

    if (*it != ':')
      FMT_THROW(format_error("no top-level range formatters supported"));

    custom_specs_ = true;
    ++it;
    ctx.advance_to(it);
    return underlying_.parse(ctx);
  }

  template <typename FormatContext>
  auto format(range_type& range, FormatContext& ctx) const
      -> decltype(ctx.out()) {
#ifdef FMT_DEPRECATED_BRACED_RANGES
    Char prefix = '{';
    Char postfix = '}';
#else
    Char prefix = detail::is_set<R>::value ? '{' : '[';
    Char postfix = detail::is_set<R>::value ? '}' : ']';
#endif
    detail::range_mapper<buffer_context<Char>> mapper;
    auto out = ctx.out();
    *out++ = prefix;
    int i = 0;
    auto it = detail::range_begin(range);
    auto end = detail::range_end(range);
    for (; it != end; ++it) {
      if (i > 0) out = detail::write_delimiter(out);
      if (custom_specs_) {
        ctx.advance_to(out);
        out = underlying_.format(mapper.map(*it), ctx);
      } else {
        out = detail::write_range_entry<Char>(out, *it);
      }
      ++i;
    }
    *out++ = postfix;
    return out;
  }
};

template <typename T, typename Char>
struct formatter<
    T, Char,
    enable_if_t<detail::is_map<T>::value
// Workaround a bug in MSVC 2019 and earlier.
#if !FMT_MSC_VER
                && (is_formattable<detail::uncvref_type<T>, Char>::value ||
                    detail::has_fallback_formatter<detail::uncvref_type<T>,
                                                   Char>::value)
#endif
                >> {
  template <typename ParseContext>
  FMT_CONSTEXPR auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  template <
      typename FormatContext, typename U,
      FMT_ENABLE_IF(
          std::is_same<U, conditional_t<detail::has_const_begin_end<T>::value,
                                        const T, T>>::value)>
  auto format(U& map, FormatContext& ctx) const -> decltype(ctx.out()) {
    auto out = ctx.out();
    *out++ = '{';
    int i = 0;
    for (const auto& item : map) {
      if (i > 0) out = detail::write_delimiter(out);
      out = detail::write_range_entry<Char>(out, item.first);
      *out++ = ':';
      *out++ = ' ';
      out = detail::write_range_entry<Char>(out, item.second);
      ++i;
    }
    *out++ = '}';
    return out;
  }
};

template <typename Char, typename... T> struct tuple_join_view : detail::view {
  const std::tuple<T...>& tuple;
  basic_string_view<Char> sep;

  tuple_join_view(const std::tuple<T...>& t, basic_string_view<Char> s)
      : tuple(t), sep{s} {}
};

template <typename Char, typename... T>
using tuple_arg_join = tuple_join_view<Char, T...>;

// Define FMT_TUPLE_JOIN_SPECIFIERS to enable experimental format specifiers
// support in tuple_join. It is disabled by default because of issues with
// the dynamic width and precision.
#ifndef FMT_TUPLE_JOIN_SPECIFIERS
#  define FMT_TUPLE_JOIN_SPECIFIERS 0
#endif

template <typename Char, typename... T>
struct formatter<tuple_join_view<Char, T...>, Char> {
  template <typename ParseContext>
  FMT_CONSTEXPR auto parse(ParseContext& ctx) -> decltype(ctx.begin()) {
    return do_parse(ctx, std::integral_constant<size_t, sizeof...(T)>());
  }

  template <typename FormatContext>
  auto format(const tuple_join_view<Char, T...>& value,
              FormatContext& ctx) const -> typename FormatContext::iterator {
    return do_format(value, ctx,
                     std::integral_constant<size_t, sizeof...(T)>());
  }

 private:
  std::tuple<formatter<typename std::decay<T>::type, Char>...> formatters_;

  template <typename ParseContext>
  FMT_CONSTEXPR auto do_parse(ParseContext& ctx,
                              std::integral_constant<size_t, 0>)
      -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  template <typename ParseContext, size_t N>
  FMT_CONSTEXPR auto do_parse(ParseContext& ctx,
                              std::integral_constant<size_t, N>)
      -> decltype(ctx.begin()) {
    auto end = ctx.begin();
#if FMT_TUPLE_JOIN_SPECIFIERS
    end = std::get<sizeof...(T) - N>(formatters_).parse(ctx);
    if (N > 1) {
      auto end1 = do_parse(ctx, std::integral_constant<size_t, N - 1>());
      if (end != end1)
        FMT_THROW(format_error("incompatible format specs for tuple elements"));
    }
#endif
    return end;
  }

  template <typename FormatContext>
  auto do_format(const tuple_join_view<Char, T...>&, FormatContext& ctx,
                 std::integral_constant<size_t, 0>) const ->
      typename FormatContext::iterator {
    return ctx.out();
  }

  template <typename FormatContext, size_t N>
  auto do_format(const tuple_join_view<Char, T...>& value, FormatContext& ctx,
                 std::integral_constant<size_t, N>) const ->
      typename FormatContext::iterator {
    auto out = std::get<sizeof...(T) - N>(formatters_)
                   .format(std::get<sizeof...(T) - N>(value.tuple), ctx);
    if (N > 1) {
      out = std::copy(value.sep.begin(), value.sep.end(), out);
      ctx.advance_to(out);
      return do_format(value, ctx, std::integral_constant<size_t, N - 1>());
    }
    return out;
  }
};

FMT_MODULE_EXPORT_BEGIN

/**
  \rst
  Returns an object that formats `tuple` with elements separated by `sep`.

  **Example**::

    std::tuple<int, char> t = {1, 'a'};
    fmt::print("{}", fmt::join(t, ", "));
    // Output: "1, a"
  \endrst
 */
template <typename... T>
FMT_CONSTEXPR auto join(const std::tuple<T...>& tuple, string_view sep)
    -> tuple_join_view<char, T...> {
  return {tuple, sep};
}

template <typename... T>
FMT_CONSTEXPR auto join(const std::tuple<T...>& tuple,
                        basic_string_view<wchar_t> sep)
    -> tuple_join_view<wchar_t, T...> {
  return {tuple, sep};
}

/**
  \rst
  Returns an object that formats `initializer_list` with elements separated by
  `sep`.

  **Example**::

    fmt::print("{}", fmt::join({1, 2, 3}, ", "));
    // Output: "1, 2, 3"
  \endrst
 */
template <typename T>
auto join(std::initializer_list<T> list, string_view sep)
    -> join_view<const T*, const T*> {
  return join(std::begin(list), std::end(list), sep);
}

FMT_MODULE_EXPORT_END
FMT_END_NAMESPACE

#endif  // FMT_RANGES_H_
