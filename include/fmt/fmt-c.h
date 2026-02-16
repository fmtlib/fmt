#ifndef FMT_C_H
#define FMT_C_H
#include <stddef.h>
#ifdef __cplusplus
#  define _Bool bool
#endif

void fmt_error_unsupported_type_detected(void);

enum { fmt_c_max_args = 16 };

typedef enum {
  fmt_err_exception = -1,
  fmt_err_memory = -2,
  fmt_err_invalid_arg = -3
} fmt_error;

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  fmt_int,
  fmt_uint,
  fmt_float,
  fmt_double,
  fmt_long_double,
  fmt_string,
  fmt_ptr,
  fmt_bool,
  fmt_char
} fmt_type;

typedef struct {
  fmt_type type;
  union {
    long long i64;
    unsigned long long u64;
    float f32;
    double f64;
    long double f128;
    const char* str;
    const void* ptr;  // Used for FMT_PTR and custom data
    _Bool bool_val;
    char char_val;
  } value;
} fmt_arg;

int fmt_vformat(char* buffer, size_t capacity, const char* format_str,
                const fmt_arg* args, size_t arg_count);

static inline fmt_arg fmt_from_int(long long x) {
  fmt_arg arg;
  arg.type = fmt_int;
  arg.value.i64 = x;
  return arg;
}

static inline fmt_arg fmt_from_uint(unsigned long long x) {
  fmt_arg arg;
  arg.type = fmt_uint;
  arg.value.u64 = x;
  return arg;
}

static inline fmt_arg fmt_from_float(float x) {
  fmt_arg arg;
  arg.type = fmt_float;
  arg.value.f32 = x;
  return arg;
}

static inline fmt_arg fmt_from_double(double x) {
  fmt_arg arg;
  arg.type = fmt_double;
  arg.value.f64 = x;
  return arg;
}

static inline fmt_arg fmt_from_long_double(long double x) {
  fmt_arg arg;
  arg.type = fmt_long_double;
  arg.value.f128 = x;
  return arg;
}

static inline fmt_arg fmt_from_str(const char* x) {
  fmt_arg arg;
  arg.type = fmt_string;
  arg.value.str = x;
  return arg;
}

static inline fmt_arg fmt_from_ptr(const void* x) {
  fmt_arg arg;
  arg.type = fmt_ptr;
  arg.value.ptr = x;
  return arg;
}

static inline fmt_arg fmt_from_bool(_Bool x) {
  fmt_arg arg;
  arg.type = fmt_bool;
  arg.value.bool_val = x;
  return arg;
}

static inline fmt_arg fmt_from_char(int x) {
  fmt_arg arg;
  arg.type = fmt_char;
  arg.value.char_val = x;
  return arg;
}

#ifdef __cplusplus
}
#endif

#ifndef __cplusplus

// Require modern MSVC with conformant preprocessor
#  if defined(_MSC_VER) && (!defined(_MSVC_TRADITIONAL) || _MSVC_TRADITIONAL)
#    error "C API requires MSVC 2019+ with /Zc:preprocessor flag."
#  endif

#  define FMT_MAKE_ARG(x)                  \
    _Generic((x),                          \
        _Bool: fmt_from_bool,              \
        char: fmt_from_char,               \
        unsigned char: fmt_from_uint,      \
        short: fmt_from_int,               \
        unsigned short: fmt_from_uint,     \
        int: fmt_from_int,                 \
        unsigned int: fmt_from_uint,       \
        long: fmt_from_int,                \
        unsigned long: fmt_from_uint,      \
        long long: fmt_from_int,           \
        unsigned long long: fmt_from_uint, \
        float: fmt_from_float,             \
        double: fmt_from_double,           \
        long double: fmt_from_long_double, \
        char*: fmt_from_str,               \
        const char*: fmt_from_str,         \
        void*: fmt_from_ptr,               \
        const void*: fmt_from_ptr,         \
        default: fmt_error_unsupported_type_detected)(x)

#  define FMT_CAT(a, b) FMT_CAT_(a, b)
#  define FMT_CAT_(a, b) a##b

#  define FMT_NARG_(_id, _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, \
                    _13, _14, _15, _16, N, ...)                             \
    N
#  define FMT_NARG(...)                                                        \
    FMT_NARG_(dummy, ##__VA_ARGS__, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, \
              4, 3, 2, 1, 0)

#  define FMT_MAP_0(...)
#  define FMT_MAP_1(f, a) f(a)
#  define FMT_MAP_2(f, a, b) f(a), f(b)
#  define FMT_MAP_3(f, a, b, c) f(a), f(b), f(c)
#  define FMT_MAP_4(f, a, b, c, d) f(a), f(b), f(c), f(d)
#  define FMT_MAP_5(f, a, b, c, d, e) f(a), f(b), f(c), f(d), f(e)
#  define FMT_MAP_6(f, a, b, c, d, e, g) f(a), f(b), f(c), f(d), f(e), f(g)
#  define FMT_MAP_7(f, a, b, c, d, e, g, h) \
    f(a), f(b), f(c), f(d), f(e), f(g), f(h)
#  define FMT_MAP_8(f, a, b, c, d, e, g, h, i) \
    f(a), f(b), f(c), f(d), f(e), f(g), f(h), f(i)
#  define FMT_MAP_9(f, a, b, c, d, e, g, h, i, j) \
    f(a), f(b), f(c), f(d), f(e), f(g), f(h), f(i), f(j)
#  define FMT_MAP_10(f, a, b, c, d, e, g, h, i, j, k) \
    f(a), f(b), f(c), f(d), f(e), f(g), f(h), f(i), f(j), f(k)
#  define FMT_MAP_11(f, a, b, c, d, e, g, h, i, j, k, l) \
    f(a), f(b), f(c), f(d), f(e), f(g), f(h), f(i), f(j), f(k), f(l)
#  define FMT_MAP_12(f, a, b, c, d, e, g, h, i, j, k, l, m) \
    f(a), f(b), f(c), f(d), f(e), f(g), f(h), f(i), f(j), f(k), f(l), f(m)
#  define FMT_MAP_13(f, a, b, c, d, e, g, h, i, j, k, l, m, n) \
    f(a), f(b), f(c), f(d), f(e), f(g), f(h), f(i), f(j), f(k), f(l), f(m), f(n)
#  define FMT_MAP_14(f, a, b, c, d, e, g, h, i, j, k, l, m, n, o)           \
    f(a), f(b), f(c), f(d), f(e), f(g), f(h), f(i), f(j), f(k), f(l), f(m), \
        f(n), f(o)
#  define FMT_MAP_15(f, a, b, c, d, e, g, h, i, j, k, l, m, n, o, p)        \
    f(a), f(b), f(c), f(d), f(e), f(g), f(h), f(i), f(j), f(k), f(l), f(m), \
        f(n), f(o), f(p)
#  define FMT_MAP_16(f, a, b, c, d, e, g, h, i, j, k, l, m, n, o, p, q)     \
    f(a), f(b), f(c), f(d), f(e), f(g), f(h), f(i), f(j), f(k), f(l), f(m), \
        f(n), f(o), f(p), f(q)

#  define FMT_MAP(f, ...) \
    FMT_CAT(FMT_MAP_, FMT_NARG(__VA_ARGS__))(f, ##__VA_ARGS__)

#  define fmt_format(buf, cap, fmt, ...)                                  \
    fmt_vformat(                                                          \
        buf, cap, fmt,                                                    \
        (fmt_arg[]){{fmt_int}, FMT_MAP(FMT_MAKE_ARG, ##__VA_ARGS__)} + 1, \
        FMT_NARG(__VA_ARGS__))

#endif  // __cplusplus

#endif  // FMT_C_H
