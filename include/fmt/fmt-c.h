// Formatting library for C++ - the C API
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_C_H_
#define FMT_C_H_

#include <stdbool.h>  // bool
#include <stddef.h>   // size_t

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  fmt_int = 1,
  fmt_uint,
  fmt_bool = 7,
  fmt_char,
  fmt_float,
  fmt_double,
  fmt_long_double,
  fmt_cstring,
  fmt_pointer = 14
} fmt_type;

typedef struct {
  fmt_type type;
  union {
    long long int_value;
    unsigned long long uint_value;  // Used for FMT_PTR and custom data
    bool bool_value;
    char char_value;
    float float_value;
    double double_value;
    long double long_double_value;
    const char* cstring;
    const void* pointer;
  } value;
} fmt_arg;

enum { fmt_error = -1, fmt_error_invalid_arg = -2 };

int fmt_vformat(char* buffer, size_t size, const char* fmt, const fmt_arg* args,
                size_t num_args);

#ifdef __cplusplus
}
#endif

#ifndef __cplusplus

static inline fmt_arg fmt_from_int(long long x) {
  return (fmt_arg){.type = fmt_int, .value.int_value = x};
}

static inline fmt_arg fmt_from_uint(unsigned long long x) {
  return (fmt_arg){.type = fmt_uint, .value.uint_value = x};
}

static inline fmt_arg fmt_from_bool(bool x) {
  return (fmt_arg){.type = fmt_bool, .value.bool_value = x};
}

static inline fmt_arg fmt_from_char(char x) {
  return (fmt_arg){.type = fmt_char, .value.char_value = x};
}

static inline fmt_arg fmt_from_float(float x) {
  return (fmt_arg){.type = fmt_float, .value.float_value = x};
}

static inline fmt_arg fmt_from_double(double x) {
  return (fmt_arg){.type = fmt_double, .value.double_value = x};
}

static inline fmt_arg fmt_from_long_double(long double x) {
  return (fmt_arg){.type = fmt_long_double, .value.long_double_value = x};
}

static inline fmt_arg fmt_from_str(const char* x) {
  return (fmt_arg){.type = fmt_cstring, .value.cstring = x};
}

static inline fmt_arg fmt_from_ptr(const void* x) {
  return (fmt_arg){.type = fmt_pointer, .value.pointer = x};
}

void fmt_unsupported_type(void);

#  ifndef _MSC_VER
typedef signed char fmt_signed_char;
#  else
typedef enum {} fmt_signed_char;
#  endif
// Require modern MSVC with conformant preprocessor
#  if defined(_MSC_VER) && (!defined(_MSVC_TRADITIONAL) || _MSVC_TRADITIONAL)
#    error "C API requires MSVC 2019+ with /Zc:preprocessor flag."
#  endif

#  define FMT_MAKE_ARG(x)                  \
    _Generic((x),                          \
        fmt_signed_char: fmt_from_int,     \
        unsigned char: fmt_from_uint,      \
        short: fmt_from_int,               \
        unsigned short: fmt_from_uint,     \
        int: fmt_from_int,                 \
        unsigned int: fmt_from_uint,       \
        long: fmt_from_int,                \
        unsigned long: fmt_from_uint,      \
        long long: fmt_from_int,           \
        unsigned long long: fmt_from_uint, \
        bool: fmt_from_bool,               \
        char: fmt_from_char,               \
        float: fmt_from_float,             \
        double: fmt_from_double,           \
        long double: fmt_from_long_double, \
        char*: fmt_from_str,               \
        const char*: fmt_from_str,         \
        void*: fmt_from_ptr,               \
        const void*: fmt_from_ptr,         \
        default: fmt_unsupported_type)(x)

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

#  define fmt_format(buffer, size, fmt, ...)                              \
    fmt_vformat(                                                          \
        buffer, size, fmt,                                                \
        (fmt_arg[]){{fmt_int}, FMT_MAP(FMT_MAKE_ARG, ##__VA_ARGS__)} + 1, \
        FMT_NARG(__VA_ARGS__))

#endif  // __cplusplus

#endif  // FMT_C_H_
