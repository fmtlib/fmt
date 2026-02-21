// Formatting library for C++ - the C API
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/fmt-c.h"

#include <fmt/base.h>

extern "C" int fmt_vformat(char* buffer, size_t size, const char* fmt,
                           const fmt_arg* args, size_t num_args) {
  constexpr size_t max_args = 16;
  if (num_args > max_args) return fmt_error_invalid_arg;

  fmt::basic_format_arg<fmt::format_context> format_args[max_args];
  for (size_t i = 0; i < num_args; ++i) {
    switch (args[i].type) {
    case fmt_int:    format_args[i] = args[i].value.int_value; break;
    case fmt_uint:   format_args[i] = args[i].value.uint_value; break;
    case fmt_float:  format_args[i] = args[i].value.float_value; break;
    case fmt_double: format_args[i] = args[i].value.double_value; break;
    case fmt_long_double:
      format_args[i] = args[i].value.long_double_value;
      break;
    case fmt_pointer: format_args[i] = args[i].value.pointer; break;
    case fmt_char:    format_args[i] = args[i].value.char_value; break;
    case fmt_bool:    format_args[i] = args[i].value.bool_value; break;
    case fmt_cstring: format_args[i] = args[i].value.cstring; break;
    default:          return fmt_error_invalid_arg;
    }
  }

  auto format_args_view = fmt::basic_format_args<fmt::format_context>(
      format_args, static_cast<int>(num_args));

  try {
    auto result = fmt::vformat_to_n(buffer, size, fmt, format_args_view);
    return static_cast<int>(result.size);
  } catch (...) {
  }
  return fmt_error;
}
