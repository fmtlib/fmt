#ifndef FMT_C_API_H
#define FMT_C_API_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

    // 1. The Types
    typedef enum { FMT_INT, FMT_DOUBLE, FMT_STRING, FMT_UNKNOWN } FmtType;

    typedef struct {
        FmtType type;
        union {
            int64_t i64;
            double  f64;
            const char* str;
        } value;
    } FmtArg;

    // 2. Safe Constructors (These prevent compiler errors)
    static inline FmtArg fmt_from_int(int64_t x) {
        FmtArg arg; arg.type = FMT_INT; arg.value.i64 = x; return arg;
    }
    static inline FmtArg fmt_from_double(double x) {
        FmtArg arg; arg.type = FMT_DOUBLE; arg.value.f64 = x; return arg;
    }
    static inline FmtArg fmt_from_str(const char* x) {
        FmtArg arg; arg.type = FMT_STRING; arg.value.str = x; return arg;
    }

    // 3. The Safer Macro (Selects the function, then calls it)
#define FMT_MAKE_ARG(x) _Generic((x), \
int:    fmt_from_int, \
long:   fmt_from_int, \
long long: fmt_from_int, \
double: fmt_from_double, \
char*:  fmt_from_str, \
const char*: fmt_from_str \
)(x)

    // 4. Function Declaration
    int fmt_c_format(char* buffer, size_t capacity, const char* format_str,
                     FmtArg* args, size_t arg_count);

#ifdef __cplusplus
}
#endif

#endif