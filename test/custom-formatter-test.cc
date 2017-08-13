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
class CustomArgFormatter : public fmt::arg_formatter<char> {
 public:
  CustomArgFormatter(fmt::buffer &buf, fmt::basic_context<char> &ctx,
                     fmt::format_specs &s)
  : fmt::arg_formatter<char>(buf, ctx, s) {}

  using fmt::arg_formatter<char>::operator();

  void operator()(double value) {
    if (round(value * pow(10, spec().precision())) == 0)
      value = 0;
    fmt::arg_formatter<char>::operator()(value);
  }
};

// A custom argument formatter that doesn't print `-` for floating-point values
// rounded to 0.
class CustomPrintfArgFormatter : public printf_arg_formatter<char> {
 public:
  CustomPrintfArgFormatter(fmt::buffer &buf, fmt::format_specs &spec)
  : printf_arg_formatter<char>(buf, spec) {}

  using printf_arg_formatter<char>::operator();

  void operator()(double value) {
    if (round(value * pow(10, spec().precision())) == 0)
      value = 0;
    printf_arg_formatter<char>::operator()(value);
  }
};

std::string custom_vformat(fmt::string_view format_str, fmt::args args) {
  fmt::memory_buffer buffer;
  // Pass custom argument formatter as a template arg to vwrite.
  fmt::vformat_to<CustomArgFormatter>(buffer, format_str, args);
  return std::string(buffer.data(), buffer.size());
}

template <typename... Args>
std::string custom_format(const char *format_str, const Args & ... args) {
  auto va = fmt::make_args(args...);
  return custom_vformat(format_str, va);
}

typedef fmt::printf_context<char, CustomPrintfArgFormatter>
  CustomPrintfFormatter;

std::string custom_vsprintf(
    const char* format_str,
    fmt::basic_args<CustomPrintfFormatter> args) {
  fmt::memory_buffer buffer;
  CustomPrintfFormatter formatter(args);
  formatter.format(format_str, buffer);
  return std::string(buffer.data(), buffer.size());
}

template <typename... Args>
std::string custom_sprintf(const char *format_str, const Args & ... args) {
  auto va = fmt::make_args<CustomPrintfFormatter>(args...);
  return custom_vsprintf(format_str, va);
}

TEST(CustomFormatterTest, Format) {
  EXPECT_EQ("0.00", custom_format("{:.2f}", -.00001));
  EXPECT_EQ("0.00", custom_sprintf("%.2f", -.00001));
}
