// Formatting library for C++ - color support for compiled format strings
//
// Copyright (c) 2018 - present, Victor Zverovich and fmt contributors
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_COMPILE_COLOR_H_
#define FMT_COMPILE_COLOR_H_

#include "color.h"
#include "compile.h"

FMT_BEGIN_NAMESPACE
FMT_BEGIN_EXPORT

#if defined(__cpp_if_constexpr) && defined(__cpp_return_type_deduction)

template <typename S, typename... Args,
          FMT_ENABLE_IF(detail::is_compiled_string<S>::value)>
FMT_INLINE std::basic_string<typename S::char_type> format(const text_style& ts,
                                                           const S& fmt,
                                                           Args&&... args) {
  memory_buffer buf;
  auto out = appender(buf);
  bool has_style = detail::apply_style<char>(out, ts);
  if (!has_style) return format(fmt, std::forward<Args>(args)...);
  out = format_to(out, fmt, std::forward<Args>(args)...);
  if (has_style) out = detail::reset_color<typename S::char_type>(out);
  return to_string(buf);
}

template <typename OutputIt, typename S, typename... Args,
          FMT_ENABLE_IF(detail::is_compiled_string<S>::value)>
FMT_CONSTEXPR OutputIt format_to(OutputIt out, const text_style& ts,
                                 const S& fmt, Args&&... args) {
  bool has_style = detail::apply_style<typename S::char_type>(out, ts);
  out = format_to(out, fmt, std::forward<Args>(args)...);
  if (has_style) out = detail::reset_color<typename S::char_type>(out);
  return out;
}

#endif

template <typename S, typename... Args,
          FMT_ENABLE_IF(detail::is_compiled_string<S>::value)>
void print(std::FILE* f, const text_style& ts, const S& fmt,
           const Args&... args) {
  memory_buffer buf;
  auto out = appender(buf);
  bool has_style = detail::apply_style<char>(out, ts);
  fmt::format_to(out, fmt, args...);
  if (has_style) detail::reset_color<char>(out);
  detail::print(f, {buf.data(), buf.size()});
}

template <typename S, typename... Args,
          FMT_ENABLE_IF(detail::is_compiled_string<S>::value)>
void print(const text_style& ts, const S& fmt, const Args&... args) {
  print(stdout, ts, fmt, args...);
}

FMT_END_EXPORT
FMT_END_NAMESPACE

#endif  // FMT_COMPILE_COLOR_H_
