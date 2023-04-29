// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/compile.h"
#include "gmock/gmock.h"

#if defined(__cpp_lib_bit_cast) && __cpp_lib_bit_cast >= 201806 && \
    defined(__cpp_constexpr) && __cpp_constexpr >= 201907 &&       \
    defined(__cpp_constexpr_dynamic_alloc) &&                      \
    __cpp_constexpr_dynamic_alloc >= 201907 && FMT_CPLUSPLUS >= 202002L
template <size_t max_string_length, typename Char = char> struct test_string {
  template <typename T> constexpr bool operator==(const T& rhs) const noexcept {
    return fmt::basic_string_view<Char>(rhs).compare(buffer) == 0;
  }
  Char buffer[max_string_length]{};
};

template <size_t max_string_length, typename Char = char, typename... Args>
consteval auto test_format(auto format, const Args&... args) {
  test_string<max_string_length, Char> string{};
  fmt::format_to(string.buffer, format, args...);
  return string;
}

#  if FMT_USE_CONSTEXPR_STR
#    define TEST_FORMAT(expected, len, str, ...)                               \
      do {                                                                     \
        EXPECT_EQ(expected, test_format<len>(FMT_COMPILE(str), __VA_ARGS__));  \
        static_assert(fmt::format(FMT_COMPILE(str), __VA_ARGS__) == expected); \
      } while (false)
#  else
#    define TEST_FORMAT(expected, len, str, ...) \
      EXPECT_EQ(expected, test_format<len>(FMT_COMPILE(str), __VA_ARGS__))
#  endif

TEST(compile_time_formatting_test, floating_point) {
  TEST_FORMAT("0", 2, "{}", 0.0f);
  TEST_FORMAT("392.500000", 11, "{0:f}", 392.5f);

  TEST_FORMAT("0", 2, "{:}", 0.0);
  TEST_FORMAT("0.000000", 9, "{:f}", 0.0);
  TEST_FORMAT("0", 2, "{:g}", 0.0);
  TEST_FORMAT("392.65", 7, "{:}", 392.65);
  TEST_FORMAT("392.65", 7, "{:g}", 392.65);
  TEST_FORMAT("392.65", 7, "{:G}", 392.65);
  TEST_FORMAT("4.9014e+06", 11, "{:g}", 4.9014e6);
  TEST_FORMAT("-392.650000", 12, "{:f}", -392.65);
  TEST_FORMAT("-392.650000", 12, "{:F}", -392.65);

  TEST_FORMAT("3.926500e+02", 13, "{0:e}", 392.65);
  TEST_FORMAT("3.926500E+02", 13, "{0:E}", 392.65);
  TEST_FORMAT("+0000392.6", 11, "{0:+010.4g}", 392.65);
  TEST_FORMAT("9223372036854775808.000000", 27, "{:f}", 9223372036854775807.0);

  constexpr double nan = std::numeric_limits<double>::quiet_NaN();
  TEST_FORMAT("nan", 4, "{}", nan);
  TEST_FORMAT("+nan", 5, "{:+}", nan);
  if (std::signbit(-nan))
    TEST_FORMAT("-nan", 5, "{}", -nan);
  else
    fmt::print("Warning: compiler doesn't handle negative NaN correctly");

  constexpr double inf = std::numeric_limits<double>::infinity();
  TEST_FORMAT("inf", 4, "{}", inf);
  TEST_FORMAT("+inf", 5, "{:+}", inf);
  TEST_FORMAT("-inf", 5, "{}", -inf);
}
#endif
