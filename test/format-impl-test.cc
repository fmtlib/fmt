// Formatting library for C++ - formatting library implementation tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#define FMT_NOEXCEPT
#undef FMT_SHARED
#include "test-assert.h"

// Include format.cc instead of format.h to test implementation.
#include "../src/format.cc"
#include "fmt/color.h"
#include "fmt/printf.h"

#include <algorithm>
#include <cstring>

#include "gmock.h"
#include "gtest-extra.h"
#include "util.h"

#undef min
#undef max

using fmt::internal::fp;

template <bool is_iec559>
void test_construct_from_double() {
  fmt::print("warning: double is not IEC559, skipping FP tests\n");
}

template <>
void test_construct_from_double<true>() {
  auto v = fp(1.23);
  EXPECT_EQ(v.f, 0x13ae147ae147aeu);
  EXPECT_EQ(v.e, -52);
}

TEST(FPTest, ConstructFromDouble) {
  test_construct_from_double<std::numeric_limits<double>::is_iec559>();
}

TEST(FPTest, Normalize) {
  auto v = fp(0xbeef, 42);
  v.normalize();
  EXPECT_EQ(0xbeef000000000000, v.f);
  EXPECT_EQ(-6, v.e);
}

TEST(FPTest, ComputeBoundariesSubnormal) {
  auto v = fp(0xbeef, 42);
  fp lower, upper;
  v.compute_boundaries(lower, upper);
  EXPECT_EQ(0xbeee800000000000, lower.f);
  EXPECT_EQ(-6, lower.e);
  EXPECT_EQ(0xbeef800000000000, upper.f);
  EXPECT_EQ(-6, upper.e);
}

TEST(FPTest, ComputeBoundaries) {
  auto v = fp(0x10000000000000, 42);
  fp lower, upper;
  v.compute_boundaries(lower, upper);
  EXPECT_EQ(0x7ffffffffffffe00, lower.f);
  EXPECT_EQ(31, lower.e);
  EXPECT_EQ(0x8000000000000400, upper.f);
  EXPECT_EQ(31, upper.e);
}

TEST(FPTest, Subtract) {
  auto v = fp(123, 1) - fp(102, 1);
  EXPECT_EQ(v.f, 21u);
  EXPECT_EQ(v.e, 1);
}

TEST(FPTest, Multiply) {
  auto v = fp(123ULL << 32, 4) * fp(56ULL << 32, 7);
  EXPECT_EQ(v.f, 123u * 56u);
  EXPECT_EQ(v.e, 4 + 7 + 64);
  v = fp(123ULL << 32, 4) * fp(567ULL << 31, 8);
  EXPECT_EQ(v.f, (123 * 567 + 1u) / 2);
  EXPECT_EQ(v.e, 4 + 8 + 64);
}

TEST(FPTest, GetCachedPower) {
  typedef std::numeric_limits<double> limits;
  for (auto exp = limits::min_exponent; exp <= limits::max_exponent; ++exp) {
    int dec_exp = 0;
    auto fp = fmt::internal::get_cached_power(exp, dec_exp);
    EXPECT_LE(exp, fp.e);
    int dec_exp_step = 8;
    EXPECT_LE(fp.e, exp + dec_exp_step * log2(10));
    EXPECT_DOUBLE_EQ(pow(10, dec_exp), ldexp(fp.f, fp.e));
  }
}

template <typename T>
struct ValueExtractor: fmt::internal::function<T> {
  T operator()(T value) {
    return value;
  }

  template <typename U>
  FMT_NORETURN T operator()(U) {
    throw std::runtime_error(fmt::format("invalid type {}", typeid(U).name()));
  }
};

TEST(FormatTest, ArgConverter) {
  long long value = std::numeric_limits<long long>::max();
  auto arg = fmt::internal::make_arg<fmt::format_context>(value);
  visit(fmt::internal::arg_converter<long long, fmt::format_context>(arg, 'd'),
        arg);
  EXPECT_EQ(value, visit(ValueExtractor<long long>(), arg));
}

TEST(FormatTest, FormatNegativeNaN) {
  double nan = std::numeric_limits<double>::quiet_NaN();
  if (fmt::internal::fputil::isnegative(-nan))
    EXPECT_EQ("-nan", fmt::format("{}", -nan));
  else
    fmt::print("Warning: compiler doesn't handle negative NaN correctly");
}

TEST(FormatTest, StrError) {
  char *message = nullptr;
  char buffer[BUFFER_SIZE];
  EXPECT_ASSERT(fmt::safe_strerror(EDOM, message = nullptr, 0), "invalid buffer");
  EXPECT_ASSERT(fmt::safe_strerror(EDOM, message = buffer, 0),
                "invalid buffer");
  buffer[0] = 'x';
#if defined(_GNU_SOURCE) && !defined(__COVERITY__)
  // Use invalid error code to make sure that safe_strerror returns an error
  // message in the buffer rather than a pointer to a static string.
  int error_code = -1;
#else
  int error_code = EDOM;
#endif

  int result = fmt::safe_strerror(error_code, message = buffer, BUFFER_SIZE);
  EXPECT_EQ(0, result);
  std::size_t message_size = std::strlen(message);
  EXPECT_GE(BUFFER_SIZE - 1u, message_size);
  EXPECT_EQ(get_system_error(error_code), message);

  // safe_strerror never uses buffer on MinGW.
#ifndef __MINGW32__
  result = fmt::safe_strerror(error_code, message = buffer, message_size);
  EXPECT_EQ(ERANGE, result);
  result = fmt::safe_strerror(error_code, message = buffer, 1);
  EXPECT_EQ(buffer, message);  // Message should point to buffer.
  EXPECT_EQ(ERANGE, result);
  EXPECT_STREQ("", message);
#endif
}

TEST(FormatTest, FormatErrorCode) {
  std::string msg = "error 42", sep = ": ";
  {
    fmt::memory_buffer buffer;
    format_to(buffer, "garbage");
    fmt::format_error_code(buffer, 42, "test");
    EXPECT_EQ("test: " + msg, to_string(buffer));
  }
  {
    fmt::memory_buffer buffer;
    std::string prefix(
        fmt::inline_buffer_size - msg.size() - sep.size() + 1, 'x');
    fmt::format_error_code(buffer, 42, prefix);
    EXPECT_EQ(msg, to_string(buffer));
  }
  int codes[] = {42, -1};
  for (std::size_t i = 0, n = sizeof(codes) / sizeof(*codes); i < n; ++i) {
    // Test maximum buffer size.
    msg = fmt::format("error {}", codes[i]);
    fmt::memory_buffer buffer;
    std::string prefix(
        fmt::inline_buffer_size - msg.size() - sep.size(), 'x');
    fmt::format_error_code(buffer, codes[i], prefix);
    EXPECT_EQ(prefix + sep + msg, to_string(buffer));
    std::size_t size = fmt::inline_buffer_size;
    EXPECT_EQ(size, buffer.size());
    buffer.resize(0);
    // Test with a message that doesn't fit into the buffer.
    prefix += 'x';
    fmt::format_error_code(buffer, codes[i], prefix);
    EXPECT_EQ(msg, to_string(buffer));
  }
}

TEST(FormatTest, CountCodePoints) {
  EXPECT_EQ(4, fmt::internal::count_code_points(fmt::u8string_view("ёжик")));
}

TEST(ColorsTest, Colors) {
  EXPECT_WRITE(stdout, fmt::print(fmt::rgb(255,20,30), "rgb(255,20,30)"),
               "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m");
  EXPECT_WRITE(stdout, fmt::print(fmt::color::blue, "blue"),
               "\x1b[38;2;000;000;255mblue\x1b[0m");
}
