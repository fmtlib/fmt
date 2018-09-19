// Formatting library for C++
//
// Copyright (c) 2012 - 2016, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/format-inl.h"

FMT_BEGIN_NAMESPACE
template struct internal::basic_data<void>;

// Explicit instantiations for char.

template FMT_API char internal::thousands_sep(locale_provider *lp);

template void internal::basic_buffer<char>::append(const char *, const char *);

template void basic_fixed_buffer<char>::grow(std::size_t);

template void internal::arg_map<format_context>::init(
    const basic_format_args<format_context> &args);

template FMT_API int internal::char_traits<char>::format_float(
    char *, std::size_t, const char *, int, double);

template FMT_API int internal::char_traits<char>::format_float(
    char *, std::size_t, const char *, int, long double);

template FMT_API std::string internal::vformat<char>(
    string_view, basic_format_args<format_context>);

// Explicit instantiations for wchar_t.

template FMT_API wchar_t internal::thousands_sep(locale_provider *);

template void internal::basic_buffer<wchar_t>::append(
    const wchar_t *, const wchar_t *);

template void basic_fixed_buffer<wchar_t>::grow(std::size_t);

template void internal::arg_map<wformat_context>::init(
    const basic_format_args<wformat_context> &);

template FMT_API int internal::char_traits<wchar_t>::format_float(
    wchar_t *, std::size_t, const wchar_t *, int, double);

template FMT_API int internal::char_traits<wchar_t>::format_float(
    wchar_t *, std::size_t, const wchar_t *, int, long double);

template FMT_API std::wstring internal::vformat<wchar_t>(
    wstring_view, basic_format_args<wformat_context>);
FMT_END_NAMESPACE
