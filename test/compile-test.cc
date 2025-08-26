// Tests for FMT_COMPILE and named arguments
#include <fmt/compile.h>
#include <gtest/gtest.h>

TEST(CompileTest, MissingNamedArgs) {
  using namespace fmt::literals;
  // These should cause compilation errors when using FMT_COMPILE
  // fmt::format(FMT_COMPILE("{x}")); // 1 - Missing argument
  // fmt::format(FMT_COMPILE("{x}"), "x"_a); // 2 - Named argument with no value
}