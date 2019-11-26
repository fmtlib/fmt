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
#include "fmt/printf.h"

#include <algorithm>
#include <cstring>

#include "gmock.h"
#include "gtest-extra.h"
#include "util.h"

#undef max

using fmt::internal::bigint;
using fmt::internal::fp;
using fmt::internal::max_value;

static_assert(!std::is_copy_constructible<bigint>::value, "");
static_assert(!std::is_copy_assignable<bigint>::value, "");

TEST(BigIntTest, Construct) {
  EXPECT_EQ("", fmt::format("{}", bigint()));
  EXPECT_EQ("42", fmt::format("{}", bigint(0x42)));
  EXPECT_EQ("123456789abcedf0", fmt::format("{}", bigint(0x123456789abcedf0)));
}

TEST(BigIntTest, Compare) {
  bigint n1(42);
  bigint n2(42);
  EXPECT_EQ(compare(n1, n2), 0);
  n2 <<= 32;
  EXPECT_LT(compare(n1, n2), 0);
  bigint n3(43);
  EXPECT_LT(compare(n1, n3), 0);
  EXPECT_GT(compare(n3, n1), 0);
  bigint n4(42 * 0x100000001);
  EXPECT_LT(compare(n2, n4), 0);
  EXPECT_GT(compare(n4, n2), 0);
}

TEST(BigIntTest, AddCompare) {
  EXPECT_LT(
      add_compare(bigint(0xffffffff), bigint(0xffffffff), bigint(1) <<= 64), 0);
  EXPECT_LT(add_compare(bigint(1) <<= 32, bigint(1), bigint(1) <<= 96), 0);
  EXPECT_GT(add_compare(bigint(1) <<= 32, bigint(0), bigint(0xffffffff)), 0);
  EXPECT_GT(add_compare(bigint(0), bigint(1) <<= 32, bigint(0xffffffff)), 0);
  EXPECT_GT(add_compare(bigint(42), bigint(1), bigint(42)), 0);
  EXPECT_GT(add_compare(bigint(0xffffffff), bigint(1), bigint(0xffffffff)), 0);
  EXPECT_LT(add_compare(bigint(10), bigint(10), bigint(22)), 0);
  EXPECT_LT(add_compare(bigint(0x100000010), bigint(0x100000010),
                        bigint(0x300000010)),
            0);
  EXPECT_GT(add_compare(bigint(0x1ffffffff), bigint(0x100000002),
                        bigint(0x300000000)),
            0);
  EXPECT_EQ(add_compare(bigint(0x1ffffffff), bigint(0x100000002),
                        bigint(0x300000001)),
            0);
  EXPECT_LT(add_compare(bigint(0x1ffffffff), bigint(0x100000002),
                        bigint(0x300000002)),
            0);
  EXPECT_LT(add_compare(bigint(0x1ffffffff), bigint(0x100000002),
                        bigint(0x300000003)),
            0);
}

TEST(BigIntTest, ShiftLeft) {
  bigint n(0x42);
  n <<= 0;
  EXPECT_EQ("42", fmt::format("{}", n));
  n <<= 1;
  EXPECT_EQ("84", fmt::format("{}", n));
  n <<= 25;
  EXPECT_EQ("108000000", fmt::format("{}", n));
}

TEST(BigIntTest, Multiply) {
  bigint n(0x42);
  EXPECT_THROW(n *= 0, assertion_failure);
  n *= 1;
  EXPECT_EQ("42", fmt::format("{}", n));
  n *= 2;
  EXPECT_EQ("84", fmt::format("{}", n));
  n *= 0x12345678;
  EXPECT_EQ("962fc95e0", fmt::format("{}", n));
  bigint bigmax(max_value<uint32_t>());
  bigmax *= max_value<uint32_t>();
  EXPECT_EQ("fffffffe00000001", fmt::format("{}", bigmax));
  bigmax.assign(max_value<uint64_t>());
  bigmax *= max_value<uint64_t>();
  EXPECT_EQ("fffffffffffffffe0000000000000001", fmt::format("{}", bigmax));
}

TEST(BigIntTest, Accumulator) {
  fmt::internal::accumulator acc;
  EXPECT_EQ(acc.lower, 0);
  EXPECT_EQ(acc.upper, 0);
  acc.upper = 12;
  acc.lower = 34;
  EXPECT_EQ(static_cast<uint32_t>(acc), 34);
  acc += 56;
  EXPECT_EQ(acc.lower, 90);
  acc += fmt::internal::max_value<uint64_t>();
  EXPECT_EQ(acc.upper, 13);
  EXPECT_EQ(acc.lower, 89);
  acc >>= 32;
  EXPECT_EQ(acc.upper, 0);
  EXPECT_EQ(acc.lower, 13 * 0x100000000);
}

TEST(BigIntTest, Square) {
  bigint n0(0);
  n0.square();
  EXPECT_EQ("0", fmt::format("{}", n0));
  bigint n1(0x100);
  n1.square();
  EXPECT_EQ("10000", fmt::format("{}", n1));
  bigint n2(0xfffffffff);
  n2.square();
  EXPECT_EQ("ffffffffe000000001", fmt::format("{}", n2));
  bigint n3(max_value<uint64_t>());
  n3.square();
  EXPECT_EQ("fffffffffffffffe0000000000000001", fmt::format("{}", n3));
  bigint n4;
  n4.assign_pow10(10);
  EXPECT_EQ("2540be400", fmt::format("{}", n4));
}

TEST(BigIntTest, DivModAssignZeroDivisor) {
  bigint zero(0);
  EXPECT_THROW(bigint(0).divmod_assign(zero), assertion_failure);
  EXPECT_THROW(bigint(42).divmod_assign(zero), assertion_failure);
}

TEST(BigIntTest, DivModAssignSelf) {
  bigint n(100);
  EXPECT_THROW(n.divmod_assign(n), assertion_failure);
}

TEST(BigIntTest, DivModAssignUnaligned) {
  // (42 << 340) / pow(10, 100):
  bigint n1(42);
  n1 <<= 340;
  bigint n2;
  n2.assign_pow10(100);
  int result = n1.divmod_assign(n2);
  EXPECT_EQ(result, 9406);
  EXPECT_EQ("10f8353019583bfc29ffc8f564e1b9f9d819dbb4cf783e4507eca1539220p96",
            fmt::format("{}", n1));
}

TEST(BigIntTest, DivModAssign) {
  // 100 / 10:
  bigint n1(100);
  int result = n1.divmod_assign(bigint(10));
  EXPECT_EQ(result, 10);
  EXPECT_EQ("0", fmt::format("{}", n1));
  // pow(10, 100) / (42 << 320):
  n1.assign_pow10(100);
  result = n1.divmod_assign(bigint(42) <<= 320);
  EXPECT_EQ(result, 111);
  EXPECT_EQ("13ad2594c37ceb0b2784c4ce0bf38ace408e211a7caab24308a82e8f10p96",
            fmt::format("{}", n1));
  // 42 / 100:
  bigint n2(42);
  n1.assign_pow10(2);
  result = n2.divmod_assign(n1);
  EXPECT_EQ(result, 0);
  EXPECT_EQ("2a", fmt::format("{}", n2));
}

template <bool is_iec559> void run_double_tests() {
  fmt::print("warning: double is not IEC559, skipping FP tests\n");
}

template <> void run_double_tests<true>() {
  // Construct from double.
  EXPECT_EQ(fp(1.23), fp(0x13ae147ae147aeu, -52));

  // Compute boundaries:
  fp value;
  // Normalized & not power of 2 - equidistant boundaries:
  auto b = value.assign_with_boundaries(1.23);
  EXPECT_EQ(value, fp(0x0013ae147ae147ae, -52));
  EXPECT_EQ(b.lower, 0x9d70a3d70a3d6c00);
  EXPECT_EQ(b.upper, 0x9d70a3d70a3d7400);
  // Normalized power of 2 - lower boundary is closer:
  b = value.assign_with_boundaries(1.9807040628566084e+28);  // 2**94
  EXPECT_EQ(value, fp(0x0010000000000000, 42));
  EXPECT_EQ(b.lower, 0x7ffffffffffffe00);
  EXPECT_EQ(b.upper, 0x8000000000000400);
  // Smallest normalized double - equidistant boundaries:
  b = value.assign_with_boundaries(2.2250738585072014e-308);
  EXPECT_EQ(value, fp(0x0010000000000000, -1074));
  EXPECT_EQ(b.lower, 0x7ffffffffffffc00);
  EXPECT_EQ(b.upper, 0x8000000000000400);
  // Subnormal - equidistant boundaries:
  b = value.assign_with_boundaries(4.9406564584124654e-324);
  EXPECT_EQ(value, fp(0x0000000000000001, -1074));
  EXPECT_EQ(b.lower, 0x4000000000000000);
  EXPECT_EQ(b.upper, 0xc000000000000000);
}

TEST(FPTest, DoubleTests) {
  run_double_tests<std::numeric_limits<double>::is_iec559>();
}

TEST(FPTest, Normalize) {
  const auto v = fp(0xbeef, 42);
  auto normalized = normalize(v);
  EXPECT_EQ(0xbeef000000000000, normalized.f);
  EXPECT_EQ(-6, normalized.e);
}

TEST(FPTest, ComputeFloatBoundaries) {
  struct {
    double x, lower, upper;
  } tests[] = {
      // regular
      {1.5f, 1.4999999403953552, 1.5000000596046448},
      // boundary
      {1.0f, 0.9999999701976776, 1.0000000596046448},
      // min normal
      {1.1754944e-38f, 1.1754942807573643e-38, 1.1754944208872107e-38},
      // max subnormal
      {1.1754942e-38f, 1.1754941406275179e-38, 1.1754942807573643e-38},
      // min subnormal
      {1e-45f, 7.006492321624085e-46, 2.1019476964872256e-45},
  };
  for (auto test : tests) {
    fp vlower = normalize(fp(test.lower));
    fp vupper = normalize(fp(test.upper));
    vlower.f >>= vupper.e - vlower.e;
    vlower.e = vupper.e;
    fp value;
    auto b = value.assign_float_with_boundaries(test.x);
    EXPECT_EQ(vlower.f, b.lower);
    EXPECT_EQ(vupper.f, b.upper);
  }
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
    EXPECT_DOUBLE_EQ(pow(10, dec_exp), ldexp(static_cast<double>(fp.f), fp.e));
  }
}

TEST(FPTest, GetRoundDirection) {
  using fmt::internal::get_round_direction;
  EXPECT_EQ(fmt::internal::down, get_round_direction(100, 50, 0));
  EXPECT_EQ(fmt::internal::up, get_round_direction(100, 51, 0));
  EXPECT_EQ(fmt::internal::down, get_round_direction(100, 40, 10));
  EXPECT_EQ(fmt::internal::up, get_round_direction(100, 60, 10));
  for (int i = 41; i < 60; ++i)
    EXPECT_EQ(fmt::internal::unknown, get_round_direction(100, i, 10));
  uint64_t max = max_value<uint64_t>();
  EXPECT_THROW(get_round_direction(100, 100, 0), assertion_failure);
  EXPECT_THROW(get_round_direction(100, 0, 100), assertion_failure);
  EXPECT_THROW(get_round_direction(100, 0, 50), assertion_failure);
  // Check that remainder + error doesn't overflow.
  EXPECT_EQ(fmt::internal::up, get_round_direction(max, max - 1, 2));
  // Check that 2 * (remainder + error) doesn't overflow.
  EXPECT_EQ(fmt::internal::unknown,
            get_round_direction(max, max / 2 + 1, max / 2));
  // Check that remainder - error doesn't overflow.
  EXPECT_EQ(fmt::internal::unknown, get_round_direction(100, 40, 41));
  // Check that 2 * (remainder - error) doesn't overflow.
  EXPECT_EQ(fmt::internal::up, get_round_direction(max, max - 1, 1));
}

TEST(FPTest, FixedHandler) {
  struct handler : fmt::internal::fixed_handler {
    char buffer[10];
    handler(int prec = 0) : fmt::internal::fixed_handler() {
      buf = buffer;
      precision = prec;
    }
  };
  int exp = 0;
  handler().on_digit('0', 100, 99, 0, exp, false);
  EXPECT_THROW(handler().on_digit('0', 100, 100, 0, exp, false),
               assertion_failure);
  namespace digits = fmt::internal::digits;
  EXPECT_EQ(handler(1).on_digit('0', 100, 10, 10, exp, false), digits::done);
  // Check that divisor - error doesn't overflow.
  EXPECT_EQ(handler(1).on_digit('0', 100, 10, 101, exp, false), digits::error);
  // Check that 2 * error doesn't overflow.
  uint64_t max = max_value<uint64_t>();
  EXPECT_EQ(handler(1).on_digit('0', max, 10, max - 1, exp, false),
            digits::error);
}

TEST(FPTest, GrisuFormatCompilesWithNonIEEEDouble) {
  fmt::memory_buffer buf;
  format_float(0.42, -1, fmt::internal::float_specs(), buf);
}

template <typename T> struct value_extractor {
  T operator()(T value) { return value; }

  template <typename U> FMT_NORETURN T operator()(U) {
    throw std::runtime_error(fmt::format("invalid type {}", typeid(U).name()));
  }

#ifdef __apple_build_version__
  // Apple Clang does not define typeid for __int128_t and __uint128_t.
  FMT_NORETURN T operator()(__int128_t) {
    throw std::runtime_error(fmt::format("invalid type {}", "__int128_t"));
  }

  FMT_NORETURN T operator()(__uint128_t) {
    throw std::runtime_error(fmt::format("invalid type {}", "__uint128_t"));
  }
#endif
};

TEST(FormatTest, ArgConverter) {
  long long value = max_value<long long>();
  auto arg = fmt::internal::make_arg<fmt::format_context>(value);
  fmt::visit_format_arg(
      fmt::internal::arg_converter<long long, fmt::format_context>(arg, 'd'),
      arg);
  EXPECT_EQ(value, fmt::visit_format_arg(value_extractor<long long>(), arg));
}

TEST(FormatTest, FormatNegativeNaN) {
  double nan = std::numeric_limits<double>::quiet_NaN();
  if (std::signbit(-nan))
    EXPECT_EQ("-nan", fmt::format("{}", -nan));
  else
    fmt::print("Warning: compiler doesn't handle negative NaN correctly");
}

TEST(FormatTest, StrError) {
  char* message = nullptr;
  char buffer[BUFFER_SIZE];
  EXPECT_ASSERT(fmt::internal::safe_strerror(EDOM, message = nullptr, 0),
                "invalid buffer");
  EXPECT_ASSERT(fmt::internal::safe_strerror(EDOM, message = buffer, 0),
                "invalid buffer");
  buffer[0] = 'x';
#if defined(_GNU_SOURCE) && !defined(__COVERITY__)
  // Use invalid error code to make sure that safe_strerror returns an error
  // message in the buffer rather than a pointer to a static string.
  int error_code = -1;
#else
  int error_code = EDOM;
#endif

  int result =
      fmt::internal::safe_strerror(error_code, message = buffer, BUFFER_SIZE);
  EXPECT_EQ(result, 0);
  std::size_t message_size = std::strlen(message);
  EXPECT_GE(BUFFER_SIZE - 1u, message_size);
  EXPECT_EQ(get_system_error(error_code), message);

  // safe_strerror never uses buffer on MinGW.
#if !defined(__MINGW32__) && !defined(__sun)
  result =
      fmt::internal::safe_strerror(error_code, message = buffer, message_size);
  EXPECT_EQ(ERANGE, result);
  result = fmt::internal::safe_strerror(error_code, message = buffer, 1);
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
    fmt::internal::format_error_code(buffer, 42, "test");
    EXPECT_EQ("test: " + msg, to_string(buffer));
  }
  {
    fmt::memory_buffer buffer;
    std::string prefix(fmt::inline_buffer_size - msg.size() - sep.size() + 1,
                       'x');
    fmt::internal::format_error_code(buffer, 42, prefix);
    EXPECT_EQ(msg, to_string(buffer));
  }
  int codes[] = {42, -1};
  for (std::size_t i = 0, n = sizeof(codes) / sizeof(*codes); i < n; ++i) {
    // Test maximum buffer size.
    msg = fmt::format("error {}", codes[i]);
    fmt::memory_buffer buffer;
    std::string prefix(fmt::inline_buffer_size - msg.size() - sep.size(), 'x');
    fmt::internal::format_error_code(buffer, codes[i], prefix);
    EXPECT_EQ(prefix + sep + msg, to_string(buffer));
    std::size_t size = fmt::inline_buffer_size;
    EXPECT_EQ(size, buffer.size());
    buffer.resize(0);
    // Test with a message that doesn't fit into the buffer.
    prefix += 'x';
    fmt::internal::format_error_code(buffer, codes[i], prefix);
    EXPECT_EQ(msg, to_string(buffer));
  }
}

TEST(FormatTest, CountCodePoints) {
  EXPECT_EQ(4, fmt::internal::count_code_points(fmt::u8string_view("ёжик")));
}

// Tests fmt::internal::count_digits for integer type Int.
template <typename Int> void test_count_digits() {
  for (Int i = 0; i < 10; ++i) EXPECT_EQ(1u, fmt::internal::count_digits(i));
  for (Int i = 1, n = 1, end = max_value<Int>() / 10; n <= end; ++i) {
    n *= 10;
    EXPECT_EQ(i, fmt::internal::count_digits(n - 1));
    EXPECT_EQ(i + 1, fmt::internal::count_digits(n));
  }
}

TEST(UtilTest, CountDigits) {
  test_count_digits<uint32_t>();
  test_count_digits<uint64_t>();
}

TEST(UtilTest, WriteUIntPtr) {
  fmt::memory_buffer buf;
  fmt::internal::writer writer(buf);
  writer.write_pointer(fmt::internal::bit_cast<fmt::internal::fallback_uintptr>(
                           reinterpret_cast<void*>(0xface)),
                       nullptr);
  EXPECT_EQ("0xface", to_string(buf));
}
