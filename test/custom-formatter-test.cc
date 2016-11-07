/*
 Custom argument formatter tests

 Copyright (c) 2016, Victor Zverovich
 All rights reserved.

 For the license information refer to format.h.
 */

#include "fmt/printf.h"
#include "gtest-extra.h"

using fmt::BasicPrintfArgFormatter;

// A custom argument formatter that doesn't print `-` for floating-point values
// rounded to 0.
class CustomArgFormatter
  : public fmt::BasicArgFormatter<CustomArgFormatter, char> {
 public:
  CustomArgFormatter(fmt::Writer &w, fmt::basic_format_context<char> &ctx,
                     fmt::FormatSpec &s)
  : fmt::BasicArgFormatter<CustomArgFormatter, char>(w, ctx, s) {}

  void visit_double(double value) {
    if (round(value * pow(10, spec().precision())) == 0)
      value = 0;
    fmt::BasicArgFormatter<CustomArgFormatter, char>::visit_double(value);
  }
};

// A custom argument formatter that doesn't print `-` for floating-point values
// rounded to 0.
class CustomPrintfArgFormatter :
    public BasicPrintfArgFormatter<CustomPrintfArgFormatter, char> {
 public:
  typedef BasicPrintfArgFormatter<CustomPrintfArgFormatter, char> Base;

  CustomPrintfArgFormatter(fmt::BasicWriter<char> &w, fmt::FormatSpec &spec)
  : Base(w, spec) {}

  void visit_double(double value) {
    if (round(value * pow(10, spec().precision())) == 0)
      value = 0;
    Base::visit_double(value);
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
  auto va = fmt::make_format_args<fmt::format_context>(args...);
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
  auto va = fmt::make_format_args<CustomPrintfFormatter>(args...);
  return custom_vsprintf(format_str, va);
}

TEST(CustomFormatterTest, Format) {
  EXPECT_EQ("0.00", custom_format("{:.2f}", -.00001));
  EXPECT_EQ("0.00", custom_sprintf("%.2f", -.00001));
}
