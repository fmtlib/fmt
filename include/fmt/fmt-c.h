#ifndef FMT_C_API_H
#define FMT_C_API_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#define FMT_C_ABI_VERSION 1
#define FMT_C_MAX_ARGS 16

#define FMT_OK 0
#define FMT_ERR_NULL_FORMAT -1
#define FMT_ERR_EXCEPTION -2
#define FMT_ERR_MEMORY -3
#define FMT_ERR_INVALID_ARG -4

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_WIN32) && !defined(FMT_C_STATIC)
#  ifdef FMT_C_EXPORT
#    define FMT_C_API __declspec(dllexport)
#  else
#    define FMT_C_API __declspec(dllimport)
#  endif
#else
#  define FMT_C_API
#endif

// Custom formatter callback
// Returns number of bytes written (excluding null terminator), or -1 on error
typedef int (*FmtCustomFn)(char* buf, size_t cap, const void* data);

typedef enum {
  FMT_INT,
  FMT_UINT,
  FMT_FLOAT,
  FMT_DOUBLE,
  FMT_LONG_DOUBLE,
  FMT_STRING,
  FMT_PTR,
  FMT_BOOL,
  FMT_CHAR,
  FMT_CUSTOM
} FmtType;

typedef struct {
  FmtType type;

  // Explicit padding for ABI stability
  //   - type:      4 bytes (enum)
  //   - _padding:  4 bytes (explicit alignment)
  //   - value:    16 bytes (union, sized by long double)
  //   - custom_fn: 8 bytes (function pointer)
  // Ensures consistent struct size across compilers (..* 24 bytes in MSVC)
  int32_t _padding;
  union {
    int64_t i64;
    uint64_t u64;
    float f32;
    double f64;
    long double f128;
    const char* str;
    const void* ptr;  // Used for FMT_PTR and custom data
    int bool_val;
    int char_val;
  } value;

  // FMT_CUSTOM type only
  FmtCustomFn custom_fn;
} FmtArg;

FMT_C_API int fmt_c_format(char* buffer, size_t capacity,
                           const char* format_str, const FmtArg* args,
                           size_t arg_count);

FMT_C_API int fmt_c_get_version(void);

static inline FmtArg fmt_from_int(int64_t x) {
  FmtArg a;
  a.type = FMT_INT;
  a._padding = 0;
  a.value.i64 = x;
  a.custom_fn = NULL;
  return a;
}

static inline FmtArg fmt_from_uint(uint64_t x) {
  FmtArg a;
  a.type = FMT_UINT;
  a._padding = 0;
  a.value.u64 = x;
  a.custom_fn = NULL;
  return a;
}

static inline FmtArg fmt_from_float(float x) {
  FmtArg a;
  a.type = FMT_FLOAT;
  a._padding = 0;
  a.value.f32 = x;
  a.custom_fn = NULL;
  return a;
}

static inline FmtArg fmt_from_double(double x) {
  FmtArg a;
  a.type = FMT_DOUBLE;
  a._padding = 0;
  a.value.f64 = x;
  a.custom_fn = NULL;
  return a;
}

static inline FmtArg fmt_from_long_double(long double x) {
  FmtArg a;
  a.type = FMT_LONG_DOUBLE;
  a._padding = 0;
  a.value.f128 = x;
  a.custom_fn = NULL;
  return a;
}

static inline FmtArg fmt_from_str(const char* x) {
  FmtArg a;
  a.type = FMT_STRING;
  a._padding = 0;
  a.value.str = x;
  a.custom_fn = NULL;
  return a;
}

static inline FmtArg fmt_from_ptr(const void* x) {
  FmtArg a;
  a.type = FMT_PTR;
  a._padding = 0;
  a.value.ptr = x;
  a.custom_fn = NULL;
  return a;
}

static inline FmtArg fmt_from_bool(bool x) {
  FmtArg a;
  a.type = FMT_BOOL;
  a._padding = 0;
  a.value.bool_val = x;
  a.custom_fn = NULL;
  return a;
}

static inline FmtArg fmt_from_char(int x) {
  FmtArg a;
  a.type = FMT_CHAR;
  a._padding = 0;
  a.value.char_val = x;
  a.custom_fn = NULL;
  return a;
}

static inline FmtArg fmt_from_custom(const void* data, FmtCustomFn func) {
  FmtArg a;
  a.type = FMT_CUSTOM;
  a._padding = 0;
  a.value.ptr = data;
  a.custom_fn = func;
  return a;
}

static inline FmtArg fmt_identity(FmtArg x) { return x; }

#ifdef __cplusplus
}
#endif

#ifndef __cplusplus

// Require modern MSVC with conformant preprocessor
#  if defined(_MSC_VER) && (!defined(_MSVC_TRADITIONAL) || _MSVC_TRADITIONAL)
#    error \
        "C API requires MSVC 2019+ with /Zc:preprocessor flag. Add /Zc:preprocessor to your compiler flags."
#  endif

#  define FMT_MAKE_ARG(x)                  \
    _Generic((x),                          \
        FmtArg: fmt_identity,              \
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
        default: fmt_from_ptr)(x)

#  define FMT_MAKE_CUSTOM(data_ptr, func_ptr) \
    fmt_from_custom((const void*)(data_ptr), func_ptr)

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

#  define fmt_format(buf, cap, fmt, ...)                                 \
    fmt_c_format(                                                        \
        buf, cap, fmt,                                                   \
        (FmtArg[]){{FMT_INT}, FMT_MAP(FMT_MAKE_ARG, ##__VA_ARGS__)} + 1, \
        FMT_NARG(__VA_ARGS__))

#endif  // !__cplusplus

#endif  // FMT_C_API_H