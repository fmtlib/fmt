#include "fmt/fmt_c.h"
#include <fmt/core.h>
#include <fmt/args.h>
#include <cstring> // For memcpy

extern "C" {

    // Here is the actual body of the function
    int fmt_c_format(char* buffer, size_t capacity, const char* format_str,
                     FmtArg* args, size_t arg_count) {

        fmt::dynamic_format_arg_store<fmt::format_context> store;

        for (size_t i = 0; i < arg_count; ++i) {
            switch (args[i].type) {
            case FMT_INT:
                store.push_back(args[i].value.i64);
                break;
            case FMT_DOUBLE:
                store.push_back(args[i].value.f64);
                break;
            case FMT_STRING:
                store.push_back(args[i].value.str ? args[i].value.str : "(null)");
                break;
            default:
                store.push_back("<?>");
                break;
            }
        }

        try {
            std::string result = fmt::vformat(format_str, store);

            if (result.size() >= capacity) {
                return -1;
            }

            std::memcpy(buffer, result.data(), result.size());
            buffer[result.size()] = '\0';

            return static_cast<int>(result.size());
        }
        catch (const std::exception& e) {
            return -2;
        }
    }

} // extern "C"