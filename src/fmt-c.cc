#include "fmt/fmt-c.h"

#include <fmt/core.h>

#include <array>
#include <cassert>
#include <exception>
#include <string>
#include <vector>

extern "C" {

using Context = fmt::format_context;

static bool populate_store(fmt::basic_format_arg<Context>* out,
                           const fmt_arg* c_args, size_t arg_count) {
  if (arg_count > FMT_C_MAX_ARGS) {
    return false;
  }

  for (size_t i = 0; i < arg_count; ++i) {
    switch (c_args[i].type) {
    case fmt_int:
      out[i] = fmt::basic_format_arg<Context>(c_args[i].value.i64);
      break;
    case fmt_uint:
      out[i] = fmt::basic_format_arg<Context>(c_args[i].value.u64);
      break;
    case fmt_float:
      out[i] = fmt::basic_format_arg<Context>(c_args[i].value.f32);
      break;
    case fmt_double:
      out[i] = fmt::basic_format_arg<Context>(c_args[i].value.f64);
      break;
    case fmt_long_double:
      out[i] = fmt::basic_format_arg<Context>(c_args[i].value.f128);
      break;
    case fmt_ptr:
      out[i] = fmt::basic_format_arg<Context>(c_args[i].value.ptr);
      break;
    case fmt_char:
      out[i] = fmt::basic_format_arg<Context>(
          static_cast<char>(c_args[i].value.char_val));
      break;
    case fmt_bool:
      out[i] = fmt::basic_format_arg<Context>(c_args[i].value.bool_val != 0);
      break;
    case fmt_string: {
      const char* s = c_args[i].value.str ? c_args[i].value.str : "(null)";
      out[i] = fmt::basic_format_arg<Context>(fmt::string_view(s));
      break;
    }

    default: return false;
    }
  }
  return true;
}
int fmt_vformat(char* buffer, size_t capacity, const char* format_str,
                const fmt_arg* args, size_t arg_count) {
  assert(format_str);

  fmt::basic_format_arg<Context> format_args[FMT_C_MAX_ARGS];

  try {
    if (arg_count > 0) {
      assert(args);
      if (!populate_store(format_args, args, arg_count)) {
        return fmt_err_exception;
      }
    }

    auto format_args_view = fmt::basic_format_args<Context>(
        format_args, static_cast<int>(arg_count));

    size_t write_capacity = (capacity > 0) ? capacity - 1 : 0;
    auto result =
        fmt::vformat_to_n(buffer, write_capacity, format_str, format_args_view);

    if (capacity > 0) {
      buffer[result.size] = '\0';
    }
    return static_cast<int>(result.size);

  } catch (const std::bad_alloc&) {
    return fmt_err_memory;
  } catch (...) {
    return fmt_err_exception;
  }
}

}  // extern "C"
