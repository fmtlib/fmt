// Formatting library for C++ - optional wchar_t support
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_WCHAR_H_
#define FMT_WCHAR_H_

#include <cwchar>

#include "format.h"

FMT_BEGIN_NAMESPACE

using wformat_parse_context = basic_format_parse_context<wchar_t>;
using wformat_context = buffer_context<wchar_t>;
using wformat_args = basic_format_args<wformat_context>;

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

inline namespace literals {
constexpr auto operator"" _format(const wchar_t* s, size_t n)
    -> detail::udl_formatter<wchar_t> {
  return {{s, n}};
}
}  // namespace literals

inline void vprint(std::FILE* f, wstring_view fmt, wformat_args args) {
  wmemory_buffer buffer;
  detail::vformat_to(buffer, fmt, args);
  buffer.push_back(L'\0');
  if (std::fputws(buffer.data(), f) == -1)
    FMT_THROW(system_error(errno, "cannot write to file"));
}

inline void vprint(wstring_view fmt, wformat_args args) {
  vprint(stdout, fmt, args);
}

template <typename... T>
void print(std::FILE* f, wformat_string<T...> fmt, T&&... args) {
  return vprint(f, wstring_view(fmt), make_wformat_args(args...));
}

template <typename... T> void print(wformat_string<T...> fmt, T&&... args) {
  return vprint(wstring_view(fmt), make_wformat_args(args...));
}

FMT_END_NAMESPACE

#endif  // FMT_WCHAR_H_
