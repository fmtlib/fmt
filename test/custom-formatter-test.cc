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
  CustomArgFormatter(fmt::BasicFormatter<char, CustomArgFormatter> &f,
                     fmt::FormatSpec &s, const char *fmt)
  : fmt::BasicArgFormatter<CustomArgFormatter, char>(f, s, fmt) {}

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

std::string custom_format(const char *format_str, fmt::ArgList args) {
  fmt::MemoryWriter writer;
  // Pass custom argument formatter as a template arg to BasicFormatter.
  fmt::BasicFormatter<char, CustomArgFormatter> formatter(args, writer);
  formatter.format(format_str);
  return writer.str();
}
FMT_VARIADIC(std::string, custom_format, const char *)

std::string custom_sprintf(const char* format_str, fmt::ArgList args){
  fmt::MemoryWriter writer;
  fmt::PrintfFormatter<char, CustomPrintfArgFormatter> formatter(args, writer);
  formatter.format(format_str);
  return writer.str();
}
FMT_VARIADIC(std::string, custom_sprintf, const char*);

TEST(CustomFormatterTest, Format) {
  EXPECT_EQ("0.00", custom_format("{:.2f}", -.00001));
  EXPECT_EQ("0.00", custom_sprintf("%.2f", -.00001));
}
