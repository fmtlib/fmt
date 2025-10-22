#include <iostream>
#include "format.h"

int main() {
  // Test short string formatting
  std::string short_result = fmt::format_short("Hello, {}!", "World");
  std::cout << "Short string test: " << short_result << std::endl;

  // Test long string (trigger dynamic allocation)
  std::string long_str(300, 'a');
  std::string long_result = fmt::format_short("{}", long_str);
  std::cout << "Long string test: " << (long_result == long_str ? "Success" : "Failed") << std::endl;

  return 0;
}