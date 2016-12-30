/*
 Custom argument formatter tests

 Copyright (c) 2016, Victor Zverovich
 All rights reserved.

 For the license information refer to format.h.
 */

#include "fmt/printf.h"
#include "gtest-extra.h"

using fmt::PrintfArgFormatter;

// A custom argument formatter that doesn't print `-` for floating-point values
// rounded to 0.
class CustomArgFormatter : public fmt::ArgFormatter<char> {
 public:
  CustomArgFormatter(fmt::writer &w, fmt::basic_format_context<char> &ctx,
                     fmt::FormatSpec &s)
  : fmt::ArgFormatter<char>(w, ctx, s) {}

  using fmt::ArgFormatter<char>::operator();

  void operator()(double value) {
    if (round(value * pow(10, spec().precision())) == 0)
      value = 0;
    fmt::ArgFormatter<char>::operator()(value);
  }
};

// A custom argument formatter that doesn't print `-` for floating-point values
// rounded to 0.
class CustomPrintfArgFormatter : public PrintfArgFormatter<char> {
 public:
  CustomPrintfArgFormatter(fmt::basic_writer<char> &w, fmt::FormatSpec &spec)
  : PrintfArgFormatter<char>(w, spec) {}

  using PrintfArgFormatter<char>::operator();

  void operator()(double value) {
    if (round(value * pow(10, spec().precision())) == 0)
      value = 0;
    PrintfArgFormatter<char>::operator()(value);
  }
};

std::string custom_vformat(fmt::CStringRef format_str, fmt::format_args args) {
  fmt::MemoryWriter writer;
  // Pass custom argument formatter as a template arg to vformat.
  fmt::vformat<CustomArgFormatter>(writer, format_str, args);
  return writer.str();
}

template <typename... Args>
std::string custom_format(const char *format_str, const Args & ... args) {
  auto va = fmt::make_format_args(args...);
  return custom_vformat(format_str, va);
}

typedef fmt::printf_context<char, CustomPrintfArgFormatter>
  CustomPrintfFormatter;

std::string custom_vsprintf(
    const char* format_str,
    fmt::basic_format_args<CustomPrintfFormatter> args) {
  fmt::MemoryWriter writer;
  CustomPrintfFormatter formatter(format_str, args);
  formatter.format(writer);
  return writer.str();
}

template <typename... Args>
std::string custom_sprintf(const char *format_str, const Args & ... args) {
  auto va = fmt::make_xformat_args<CustomPrintfFormatter>(args...);
  return custom_vsprintf(format_str, va);
}

TEST(CustomFormatterTest, Format) {
  EXPECT_EQ("0.00", custom_format("{:.2f}", -.00001));
  EXPECT_EQ("0.00", custom_sprintf("%.2f", -.00001));
}
