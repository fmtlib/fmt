#undef FMT_C_EXPORT
#define FMT_C_EXPORT
#include "fmt/fmt-c.h"

#include <fmt/core.h>

#include <array>
#include <exception>
#include <string>
#include <vector>

extern "C" {

int fmt_c_get_version(void) { return FMT_C_ABI_VERSION; }

using Context = fmt::format_context;

// Fixed-size array for type-erased format arguments
static thread_local std::array<fmt::basic_format_arg<Context>, FMT_C_MAX_ARGS>
    g_fixed_store;

static bool populate_store(const FmtArg* c_args, size_t arg_count,
                           std::vector<std::string>& custom_buffers) {
  if (arg_count > FMT_C_MAX_ARGS) {
    return false;
  }

  for (size_t i = 0; i < arg_count; ++i) {
    switch (c_args[i].type) {
    case FMT_INT:
      g_fixed_store[i] = fmt::basic_format_arg<Context>(c_args[i].value.i64);
      break;
    case FMT_UINT:
      g_fixed_store[i] = fmt::basic_format_arg<Context>(c_args[i].value.u64);
      break;
    case FMT_FLOAT:
      g_fixed_store[i] = fmt::basic_format_arg<Context>(c_args[i].value.f32);
      break;
    case FMT_DOUBLE:
      g_fixed_store[i] = fmt::basic_format_arg<Context>(c_args[i].value.f64);
      break;
    case FMT_LONG_DOUBLE:
      g_fixed_store[i] = fmt::basic_format_arg<Context>(c_args[i].value.f128);
      break;
    case FMT_PTR:
      g_fixed_store[i] = fmt::basic_format_arg<Context>(c_args[i].value.ptr);
      break;
    case FMT_CHAR:
      g_fixed_store[i] = fmt::basic_format_arg<Context>(
          static_cast<char>(c_args[i].value.char_val));
      break;
    case FMT_BOOL:
      g_fixed_store[i] =
          fmt::basic_format_arg<Context>(c_args[i].value.bool_val != 0);
      break;
    case FMT_STRING: {
      const char* s = c_args[i].value.str ? c_args[i].value.str : "(null)";
      g_fixed_store[i] = fmt::basic_format_arg<Context>(fmt::string_view(s));
      break;
    }

    case FMT_CUSTOM: {
      if (!c_args[i].custom_fn || !c_args[i].value.ptr) {
        g_fixed_store[i] =
            fmt::basic_format_arg<Context>(fmt::string_view("<error>"));
        return false;
      }

      try {
        std::string buf;
        buf.resize(64);
        int len = c_args[i].custom_fn(&buf[0], buf.size(), c_args[i].value.ptr);

        if (len < 0) return false;

        if (static_cast<size_t>(len) >= buf.size()) {
          buf.resize(len + 1);
          len = c_args[i].custom_fn(&buf[0], buf.size(), c_args[i].value.ptr);
          if (len < 0) return false;
        }

        buf.resize(len);
        custom_buffers.push_back(std::move(buf));
        g_fixed_store[i] = fmt::basic_format_arg<Context>(
            fmt::string_view(custom_buffers.back()));

      } catch (...) {
        return false;
      }
      break;
    }

    default: return false;
    }
  }
  return true;
}
int fmt_c_format(char* buffer, size_t capacity, const char* format_str,
                 const FmtArg* args, size_t arg_count) {
  if (!format_str) return FMT_ERR_NULL_FORMAT;
  if (arg_count > FMT_C_MAX_ARGS) return FMT_ERR_INVALID_ARG;

  try {
    std::vector<std::string> custom_buffers;
    if (arg_count > 0) {
      if (!args) return FMT_ERR_INVALID_ARG;
      if (!populate_store(args, arg_count, custom_buffers)) {
        return FMT_ERR_EXCEPTION;
      }
    }

    auto format_args_view = fmt::basic_format_args<Context>(
        g_fixed_store.data(), static_cast<int>(arg_count));

    if (!buffer || capacity == 0) {
      char tmp[1];
      auto result = fmt::vformat_to_n(tmp, 0, format_str, format_args_view);
      return static_cast<int>(result.size);
    }

    auto result =
        fmt::vformat_to_n(buffer, capacity - 1, format_str, format_args_view);
    *result.out = '\0';
    return static_cast<int>(result.size);

  } catch (const std::bad_alloc&) {
    return FMT_ERR_MEMORY;
  } catch (...) {
    return FMT_ERR_EXCEPTION;
  }
}

}  // extern "C"