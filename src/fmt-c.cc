#include "fmt/fmt-c.h"

#include <fmt/core.h>

#include <cassert>

extern "C" {

using format_arg = fmt::basic_format_arg<fmt::format_context>;

static bool populate_store(format_arg* out, const fmt_arg* c_args,
                           size_t arg_count) {
  if (arg_count > fmt_c_max_args) {
    return false;
  }

  for (size_t i = 0; i < arg_count; ++i) {
    switch (c_args[i].type) {
    case fmt_int:         out[i] = c_args[i].value.i64; break;
    case fmt_uint:        out[i] = c_args[i].value.u64; break;
    case fmt_float:       out[i] = c_args[i].value.f32; break;
    case fmt_double:      out[i] = c_args[i].value.f64; break;
    case fmt_long_double: out[i] = c_args[i].value.f128; break;
    case fmt_ptr:         out[i] = c_args[i].value.ptr; break;
    case fmt_char:        out[i] = c_args[i].value.char_val; break;
    case fmt_bool:        out[i] = c_args[i].value.bool_val; break;
    case fmt_string:      out[i] = c_args[i].value.str; break;
    default:              return false;
    }
  }
  return true;
}

int fmt_vformat(char* buffer, size_t capacity, const char* format_str,
                const fmt_arg* args, size_t arg_count) {
  assert(format_str);

  format_arg format_args[fmt_c_max_args];

  if (arg_count > 0) {
    assert(args);
    if (!populate_store(format_args, args, arg_count)) {
      return fmt_err_invalid_arg;
    }
  }

  auto format_args_view = fmt::basic_format_args<fmt::format_context>(
      format_args, static_cast<int>(arg_count));

  auto result =
      fmt::vformat_to_n(buffer, capacity, format_str, format_args_view);
  return static_cast<int>(result.size);
}

}  // extern "C"
