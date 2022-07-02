// Formatting library for C++ - tests for ranges and std combination
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.
//
// Copyright (c) 2022 - present, Dani-Hub (Daniel Kruegler)
// All rights reserved

#include "fmt/ranges.h"
#include "fmt/std.h"

#include <string>
#include <vector>

#include "gtest/gtest.h"

TEST(ranges_std_test, format_vector_path) {
#ifdef __cpp_lib_filesystem
  auto p = std::filesystem::path("foo/bar.txt");
  auto c = std::vector<std::string>{"abc", "def"};
  EXPECT_EQ(fmt::format("path={}, range={}", p, c),
            "path=\"foo/bar.txt\", range=[\"abc\", \"def\"]");
#endif
}
