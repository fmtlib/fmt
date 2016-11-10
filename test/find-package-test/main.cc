#include "fmt/printf.h"

int main(int argc, char** argv) {
  for(int i = 0; i < argc; ++i)
    fmt::print("{}: {}\n", i, argv[i]);
}
