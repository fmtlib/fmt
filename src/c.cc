#undef FMT_C_EXPORT
#define FMT_C_EXPORT
#include "fmt/c.h"

#include <fmt/core.h>
#include <exception>
#include <string>
#include <vector>
#include <array>

static const size_t MAX_PACKED_ARGS = FMT_C_MAX_ARGS;

extern "C" {
static thread_local std::string g_last_error;

const char* fmt_c_get_error(void) {
    return g_last_error.empty() ? "" : g_last_error.c_str();
}

static void set_error(const char* msg) {
    try {
        g_last_error = msg;
    } catch(...) {
    }
}

static void clear_error() {
    g_last_error.clear();
}

int fmt_c_get_version(void) {
    return FMT_C_ABI_VERSION;
}

using Context = fmt::format_context;

// Fixed-size array for type-erased format arguments
static thread_local std::array<fmt::basic_format_arg<Context>, MAX_PACKED_ARGS> g_fixed_store;

static bool populate_store(const FmtArg* c_args, size_t arg_count,
                           std::vector<std::string>& custom_buffers) {

    if (arg_count > MAX_PACKED_ARGS) {
        set_error("Argument count exceeds maximum (FMT_C_MAX_ARGS)");
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
                g_fixed_store[i] = fmt::basic_format_arg<Context>(static_cast<char>(c_args[i].value.char_val));
                break;

            case FMT_BOOL:
                g_fixed_store[i] = fmt::basic_format_arg<Context>(c_args[i].value.bool_val != 0);
                break;

            case FMT_STRING: {
                const char* s = c_args[i].value.str ? c_args[i].value.str : "(null)";
                g_fixed_store[i] = fmt::basic_format_arg<Context>(fmt::string_view(s));
                break;
            }

            case FMT_CUSTOM: {
                if (!c_args[i].custom_fn) {
                    set_error("Custom formatter function is NULL");
                    g_fixed_store[i] = fmt::basic_format_arg<Context>(fmt::string_view("<error>"));
                    return false;
                }

                if (!c_args[i].value.ptr) {
                    set_error("Custom formatter data pointer is NULL");
                    g_fixed_store[i] = fmt::basic_format_arg<Context>(fmt::string_view("<error>"));
                    return false;
                }

                try {
                    std::string buf;
                    buf.resize(64);// intial bufffer size ....
                    int len = c_args[i].custom_fn(&buf[0], buf.size(), c_args[i].value.ptr);

                    if (len < 0) {
                        set_error("Custom formatter returned error code");
                        g_fixed_store[i] = fmt::basic_format_arg<Context>(fmt::string_view("<error>"));
                        return false;
                    }
                    if (static_cast<size_t>(len) >= buf.size()) {
                        buf.resize(len + 1);
                        len = c_args[i].custom_fn(&buf[0], buf.size(), c_args[i].value.ptr);

                        if (len < 0) {
                            set_error("Custom formatter failed on second call");
                            g_fixed_store[i] = fmt::basic_format_arg<Context>(fmt::string_view("<error>"));
                            return false;
                        }
                    }

                    buf.resize(len);
                    custom_buffers.push_back(std::move(buf));
                    g_fixed_store[i] = fmt::basic_format_arg<Context>(fmt::string_view(custom_buffers.back()));

                } catch (const std::bad_alloc&) {
                    set_error("Memory allocation failed in custom formatter");
                    g_fixed_store[i] = fmt::basic_format_arg<Context>(fmt::string_view("<error>"));
                    return false;
                } catch (...) {
                    set_error("Unknown exception in custom formatter");
                    g_fixed_store[i] = fmt::basic_format_arg<Context>(fmt::string_view("<error>"));
                    return false;
                }
                break;
            }

            default:
                set_error("Unknown FmtType enum value");
                g_fixed_store[i] = fmt::basic_format_arg<Context>(fmt::string_view("<error>"));
                return false;
        }
    }

    return true;
}
int fmt_c_format(char* buffer, size_t capacity, const char* format_str,
                 const FmtArg* args, size_t arg_count) {
    clear_error();
    if (!format_str) {
        set_error("Format string is NULL");
        return FMT_ERR_NULL_FORMAT;
    }

    if (arg_count > MAX_PACKED_ARGS) {
        set_error("Too many arguments (maximum is FMT_C_MAX_ARGS)");
        return FMT_ERR_MEMORY;
    }

    try {
        std::vector<std::string> custom_buffers;
        if (arg_count > 0) {
            if (!args) {
                set_error("Argument array is NULL but arg_count > 0");
                return FMT_ERR_NULL_FORMAT;
            }
            if (!populate_store(args, arg_count, custom_buffers)) {
                return FMT_ERR_EXCEPTION;
            }
        }

        auto format_args_view = fmt::basic_format_args<Context>(
            g_fixed_store.data(),
            static_cast<int>(arg_count)
        );

        if (!buffer || capacity == 0) {
            char tmp[1];
            auto result = fmt::vformat_to_n(tmp, 0, format_str, format_args_view);
            return static_cast<int>(result.size);
        }
        auto result = fmt::vformat_to_n(buffer, capacity - 1, format_str, format_args_view);

        *result.out = '\0';
        return static_cast<int>(result.size);

    } catch (const fmt::format_error& e) {
        set_error(e.what());
        return FMT_ERR_EXCEPTION;
    } catch (const std::exception& e) {
        set_error(e.what());
        return FMT_ERR_EXCEPTION;
    } catch (...) {
        set_error("Unknown C++ exception");
        return FMT_ERR_EXCEPTION;
    }
}

void fmt_c_print(FILE* f, const char* format_str, const FmtArg* args, size_t arg_count) {
    clear_error();

    if (!f) {
        set_error("File stream is NULL");
        return;
    }

    if (!format_str) {
        set_error("Format string is NULL");
        return;
    }
    if (arg_count > MAX_PACKED_ARGS) {
        set_error("Too many arguments (maximum is FMT_C_MAX_ARGS)");
        return;
    }

    try {
        std::vector<std::string> custom_buffers;
        if (arg_count > 0) {
            if (!args) {
                set_error("Argument array is NULL but arg_count > 0");
                return;
            }
            if (!populate_store(args, arg_count, custom_buffers)) {
                return;
            }
        }
        auto format_args_view = fmt::basic_format_args<Context>(
            g_fixed_store.data(),
            static_cast<int>(arg_count)
        );
        fmt::vprint(f, format_str, format_args_view);

    } catch (const fmt::format_error& e) {
        set_error(e.what());
    } catch (const std::exception& e) {
        set_error(e.what());
    } catch (...) {
        set_error("Unknown C++ exception");
    }
}

} // extern "C"