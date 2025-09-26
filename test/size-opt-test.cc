#include "fmt/format.h"
#include "gtest/gtest.h"

TEST(SizeOptTest, BasicFormatting) {
  // Test basic formatting under size-optimized, header-only configuration
  auto s = fmt::format("size-optimized {}", 42);
  ASSERT_EQ(s, "size-optimized 42");
}
