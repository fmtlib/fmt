// Formatting library for C++ - experimental wchar_t support
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_WCHAR_H_
#define FMT_WCHAR_H_

#include "format.h"

namespace fmt {

#if FMT_GCC_VERSION && FMT_GCC_VERSION < 409
// Workaround broken conversion on older gcc.
template <typename... Args> using wformat_string = wstring_view;
#else
template <typename... Args>
using wformat_string = basic_format_string<wchar_t, type_identity_t<Args>...>;
#endif

template <typename... Args>
constexpr format_arg_store<wformat_context, Args...> make_wformat_args(
    const Args&... args) {
  return {args...};
}

template <typename... T>
void print(std::FILE* f, wformat_string<T...> str, T&&... args) {
  return vprint(f, wstring_view(str), make_wformat_args(args...));
}

template <typename... T> void print(wformat_string<T...> str, T&&... args) {
  return vprint(wstring_view(str), make_wformat_args(args...));
}
}  // namespace fmt

#endif  // FMT_WCHAR_H_
