// Formatting library for C++ - the core API
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.
//
// Copyright (c) 2018 - present, Remotion (Igor Schulz)
// All Rights Reserved
// {fmt} support for ranges, containers and types tuple interface.

#ifdef WIN32
#define _CRT_SECURE_NO_WARNINGS
#endif

#define FMT_HEADER_ONLY   1
#include "fmt/ranges.h"

#include "gtest/gtest.h"


#include <vector>
#include <array>
#include <map>
#include <string_view>

TEST(RangesTest, FormatVector) {
  std::vector<int32_t> iv{ 1,2,3,5,7,11 };
  auto ivf = fmt::format("{}", iv);
  EXPECT_EQ("{ 1, 2, 3, 5, 7, 11 }", ivf);
}

TEST(RangesTest, FormatVector2) {
  std::vector<std::vector<int32_t>> ivv{ {1,2},{3,5},{7,11} };
  auto ivf = fmt::format("{}", ivv);
  EXPECT_EQ("{ { 1, 2 }, { 3, 5 }, { 7, 11 } }", ivf);
}

TEST(RangesTest, FormatMap) {
  std::map<std::string, int32_t>  simap{ {"one",1}, {"two",2} };
  EXPECT_EQ("{ [ one, 1 ], [ two, 2 ] }", fmt::format("{}", simap));
}

TEST(RangesTest, FormatPair) {
  std::pair<int64_t, float> pa1{42, 3.14159265358979f};
  EXPECT_EQ("[ 42, 3.14159 ]", fmt::format("{}", pa1));
}

TEST(RangesTest, FormatTuple) {
  std::tuple<int64_t, float, std::string> tu1{42, 3.14159265358979f, "this is tuple"};
  EXPECT_EQ("[ 42, 3.14159, this is tuple ]", fmt::format("{}", tu1));
}

