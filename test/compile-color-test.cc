// Formatting library for C++ - color tests with compiled format strings
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/compile-color.h"

#include <iterator>  // std::back_inserter

#include "gtest-extra.h"  // EXPECT_WRITE

TEST(compile_color_test, format) {
  EXPECT_EQ(fmt::format(fmt::text_style(), FMT_COMPILE("hi")), "hi");
  EXPECT_EQ(fmt::format(fmt::text_style(), FMT_COMPILE("{} {}"), "hi", "there"),
            "hi there");

  EXPECT_EQ(fmt::format(fg(fmt::terminal_color::red), FMT_COMPILE("{}"), "foo"),
            "\x1b[31mfoo\x1b[0m");
  EXPECT_EQ(fmt::format(fg(fmt::terminal_color::red), FMT_COMPILE("{} {}"),
                        "foo", 42),
            "\x1b[31mfoo 42\x1b[0m");
}

TEST(compile_color_test, format_to) {
  auto out = std::string();
  fmt::format_to(std::back_inserter(out), fg(fmt::rgb(255, 20, 30)),
                 FMT_COMPILE("rgb(255,20,30){}{}{}"), 1, 2, 3);
  EXPECT_EQ(fmt::to_string(out),
            "\x1b[38;2;255;020;030mrgb(255,20,30)123\x1b[0m");
}

TEST(color_test, print) {
  EXPECT_WRITE(stdout, fmt::print(fg(fmt::rgb(255, 20, 30)), "rgb(255,20,30)"),
               "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m");
}
