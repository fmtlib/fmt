#include <fmt/core.h>

int main()
{
   fmt::print("Hello, {}!", "world");  // uses Python-like format string syntax

   fmt::print("The answer is {}\n", 42);

   return 0;
}
