#ifdef FMT_IMPORT_STD
import std;
import fmt;
#else
#  include "fmt/format.h"
#endif

int main(int argc, char** argv) {
  for (int i = 0; i < argc; ++i) fmt::print("{}: {}\n", i, argv[i]);
}
