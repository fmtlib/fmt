// Formatting library for C++ - custom argument formatter tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/format.h"
#include "gtest-extra.h"

// MSVC 2013 is known to be broken.
#if !FMT_MSC_VER || FMT_MSC_VER > 1800

// A custom argument formatter that doesn't print `-` for floating-point values
// rounded to 0.
class custom_arg_formatter :
    public fmt::arg_formatter<fmt::back_insert_range<fmt::internal::buffer>> {
 public:
  typedef fmt::back_insert_range<fmt::internal::buffer> range;
  typedef fmt::arg_formatter<range> base;

  custom_arg_formatter(fmt::format_context &ctx, fmt::format_specs &s)
  : base(ctx, s) {}

  using base::operator();

  iterator operator()(double value) {
    // Comparing a float to 0.0 is safe
    if (round(value * pow(10, spec().precision())) == 0.0)
      value = 0;
    return base::operator()(value);
  }
};

std::string custom_vformat(fmt::string_view format_str, fmt::format_args args) {
  fmt::memory_buffer buffer;
  // Pass custom argument formatter as a template arg to vwrite.
  fmt::vformat_to<custom_arg_formatter>(buffer, format_str, args);
  return std::string(buffer.data(), buffer.size());
}

template <typename... Args>
std::string custom_format(const char *format_str, const Args & ... args) {
  auto va = fmt::make_format_args(args...);
  return custom_vformat(format_str, va);
}

TEST(CustomFormatterTest, Format) {
  EXPECT_EQ("0.00", custom_format("{:.2f}", -.00001));
}
#endif
