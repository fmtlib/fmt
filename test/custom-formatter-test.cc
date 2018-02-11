/*
 Custom argument formatter tests

 Copyright (c) 2016, Victor Zverovich
 All rights reserved.

 For the license information refer to format.h.
 */

#include "fmt/printf.h"
#include "gtest-extra.h"

using fmt::printf_arg_formatter;

// A custom argument formatter that doesn't print `-` for floating-point values
// rounded to 0.
class CustomArgFormatter :
    public fmt::arg_formatter<fmt::back_insert_range<fmt::internal::buffer>> {
 public:
  typedef fmt::back_insert_range<fmt::internal::buffer> range;
  typedef decltype(std::declval<range>().begin()) iterator;
  typedef fmt::arg_formatter<range> base;

  CustomArgFormatter(fmt::basic_context<iterator, char> &ctx,
                     fmt::format_specs &s)
  : base(ctx, s) {}

  using base::operator();

  void operator()(double value) {
    if (round(value * pow(10, spec().precision())) == 0)
      value = 0;
    base::operator()(value);
  }
};

std::string custom_vformat(fmt::string_view format_str, fmt::format_args args) {
  fmt::memory_buffer buffer;
  // Pass custom argument formatter as a template arg to vwrite.
  fmt::do_vformat_to<CustomArgFormatter>(buffer, format_str, args);
  return std::string(buffer.data(), buffer.size());
}

template <typename... Args>
std::string custom_format(const char *format_str, const Args & ... args) {
  auto va = fmt::make_args(args...);
  return custom_vformat(format_str, va);
}

TEST(CustomFormatterTest, Format) {
  EXPECT_EQ("0.00", custom_format("{:.2f}", -.00001));
}
