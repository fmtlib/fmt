// Formatting library for C++ - std::locale support
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_LOCALE_H_
#define FMT_LOCALE_H_

#include "format.h"
#include <locale>

FMT_BEGIN_NAMESPACE

namespace internal {
template <typename Char>
typename buffer_context<Char>::type::iterator vformat_to(
    const std::locale &loc, basic_buffer<Char> &buf,
    basic_string_view<Char> format_str,
    basic_format_args<typename buffer_context<Char>::type> args) {
  typedef back_insert_range<basic_buffer<Char> > range;
  return vformat_to<arg_formatter<range>>(
    buf, to_string_view(format_str), args, internal::locale_ref(loc));
}

template <typename Char>
std::basic_string<Char> vformat(
    const std::locale &loc, basic_string_view<Char> format_str,
    basic_format_args<typename buffer_context<Char>::type> args) {
  basic_memory_buffer<Char> buffer;
  internal::vformat_to(loc, buffer, format_str, args);
  return fmt::to_string(buffer);
}
}

template <typename S, typename... Args>
inline std::basic_string<FMT_CHAR(S)> format(
    const std::locale &loc, const S &format_str, const Args &... args) {
  return internal::vformat(
    loc, to_string_view(format_str),
    *internal::checked_args<S, Args...>(format_str, args...));
}

FMT_END_NAMESPACE

#endif  // FMT_LOCALE_H_
