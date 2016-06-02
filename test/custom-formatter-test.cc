#include "fmt/format.h"
#include "fmt/printf.h"

#include <iostream>

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
class CustomPAF : public fmt::internal::ArgFormatterBase<CustomPAF, char>
{
public:
  CustomPAF(fmt::BasicWriter<char> &w, fmt::FormatSpec &s):
    fmt::internal::ArgFormatterBase<CustomPAF, char>(w, s) {}

  void visit_double(double value) {
    if (round(value * pow(10, spec().precision())) == 0)
      value = 0;
    fmt::internal::ArgFormatterBase<CustomPAF, char>::visit_double(value);
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
  fmt::internal::PrintfFormatter<char, CustomPAF> pfer(args);
  pfer.format(writer,fstr);
  return writer.str();
}

FMT_VARIADIC(std::string, printfer, const char*);

int main() {
  std::cout << custom_format("custom: {:.2f}", -0.000001) << std::endl;
  std::cout << printfer("printf: %.2f", -0.0001) << std::endl;

}
