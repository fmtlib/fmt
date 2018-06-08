// Formatting library for C++ - the core API
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.
//
// Copyright (c) 2018 - present, Remotion (Igor Schulz)
// All Rights Reserved
// {fmt} support for rgb color output.

#include "gtest.h"
#include "gtest-extra.h"

#include "fmt/colors.h"
#include "fmt/printf.h"

#include <vector>
#include <array>
#include <map>
#include <string>

TEST(ColorsTest, RgbTest) {
  fmt::print(fmt::rgb(10,20,30), "rgb(10,20,30) \n");    // \x1b[38;2;010;020;030mrgb(10,20,30) \n\x1b[0m
  fmt::print(fmt::rgb(255,20,30), "rgb(255,20,30) \n");  // \x1b[38;2;255;020;030mrgb(255,20,30) \n\x1b[0m
  fmt::print(fmt::rgb(30,255,30), "rgb(30,255,30) \n");  // \x1b[38;2;030;255;030mrgb(30,255,30) \n\x1b[0m
  fmt::print(fmt::rgb(30,30,255), "rgb(30,30,255) \n");  // \x1b[38;2;030;030;255mrgb(30,30,255) \n\x1b[0m

  EXPECT_WRITE(stdout, fmt::print(fmt::rgb(255,20,30), "rgb(255,20,30)"), "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m");
}

TEST(ColorsTest, Colors) {
  fmt::print(fmt::colors::blue,"blue \n");               // \x1b[38;2;000;000;255mblue \n\x1b[0m
  fmt::print(fmt::colors::medium_spring_green,"medium_spring_green \n"); // \x1b[38;2;000;250;154mmedium_spring_green \n\x1b[0m
  fmt::print(fmt::colors::light_golden_rod_yellow,"light_golden_rod_yellow \n"); // \x1b[38;2;250;250;210mlight_golden_rod_yellow \n\x1b[0m

  EXPECT_WRITE(stdout, fmt::print(fmt::colors::blue,"blue"), "\x1b[38;2;000;000;255mblue\x1b[0m");
}
