/*
 printf tests.

 Copyright (c) 2012-2014, Victor Zverovich
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "fmt/format.h"
#include "fmt/printf.h"
#include "gtest-extra.h"

// A custom argument formatter that doesn't print `-` for floating-point values
// rounded to 0.
class CustomArgFormatter :
  public fmt::BasicArgFormatter<CustomArgFormatter, char>  {
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
class CustomPAF : public fmt::internal::PrintfArgFormatter<CustomPAF, char>
{
public:
  CustomPAF(fmt::BasicWriter<char> &writer, fmt::FormatSpec &spec):
	  fmt::internal::PrintfArgFormatter<CustomPAF, char>(writer, spec) {}

  void visit_double(double value) {
    if (round(value * pow(10, spec().precision())) == 0)
      value = 0;
    fmt::internal::PrintfArgFormatter< CustomPAF, char>::visit_double(value);
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


std::string printfer(const char* fstr, fmt::ArgList args){
  fmt::MemoryWriter writer;
  fmt::internal::PrintfFormatter< char, CustomPAF > pfer( args);
  pfer.format(writer, fstr);
   return writer.str();
}
FMT_VARIADIC(std::string, printfer, const char*);


TEST(custom, foo){
	EXPECT_EQ("0.00", custom_format("{:.2f}", -.00001));
	EXPECT_EQ("0.00", printfer("%.2f", -.00001));
}
