// Formatting library for C++ - color tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/color.h"
#include "gtest-extra.h"

TEST(ColorsTest, ColorsPrint) {
  EXPECT_WRITE(stdout, fmt::print(fg(fmt::rgb(255, 20, 30)), "rgb(255,20,30)"),
               "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m");
  EXPECT_WRITE(stdout, fmt::print(fg(fmt::color::blue), "blue"),
               "\x1b[38;2;000;000;255mblue\x1b[0m");
  EXPECT_WRITE(
      stdout,
      fmt::print(fg(fmt::color::blue) | bg(fmt::color::red), "two color"),
      "\x1b[38;2;000;000;255m\x1b[48;2;255;000;000mtwo color\x1b[0m");
  EXPECT_WRITE(stdout, fmt::print(fmt::emphasis::bold, "bold"),
               "\x1b[1mbold\x1b[0m");
  EXPECT_WRITE(stdout, fmt::print(fmt::emphasis::italic, "italic"),
               "\x1b[3mitalic\x1b[0m");
  EXPECT_WRITE(stdout, fmt::print(fmt::emphasis::underline, "underline"),
               "\x1b[4munderline\x1b[0m");
  EXPECT_WRITE(stdout,
               fmt::print(fmt::emphasis::strikethrough, "strikethrough"),
               "\x1b[9mstrikethrough\x1b[0m");
  EXPECT_WRITE(
      stdout,
      fmt::print(fg(fmt::color::blue) | fmt::emphasis::bold, "blue/bold"),
      "\x1b[1m\x1b[38;2;000;000;255mblue/bold\x1b[0m");
  EXPECT_WRITE(stderr, fmt::print(stderr, fmt::emphasis::bold, "bold error"),
               "\x1b[1mbold error\x1b[0m");
  EXPECT_WRITE(stderr, fmt::print(stderr, fg(fmt::color::blue), "blue log"),
               "\x1b[38;2;000;000;255mblue log\x1b[0m");
  EXPECT_WRITE(stdout, fmt::print(fmt::text_style(), "hi"), "hi");
  EXPECT_WRITE(stdout, fmt::print(fg(fmt::terminal_color::red), "tred"),
               "\x1b[31mtred\x1b[0m");
  EXPECT_WRITE(stdout, fmt::print(bg(fmt::terminal_color::cyan), "tcyan"),
               "\x1b[46mtcyan\x1b[0m");
  EXPECT_WRITE(stdout,
               fmt::print(fg(fmt::terminal_color::bright_green), "tbgreen"),
               "\x1b[92mtbgreen\x1b[0m");
  EXPECT_WRITE(stdout,
               fmt::print(bg(fmt::terminal_color::bright_magenta), "tbmagenta"),
               "\x1b[105mtbmagenta\x1b[0m");
}

TEST(ColorsTest, Format) {
  EXPECT_EQ(fmt::format(fg(fmt::rgb(255, 20, 30)), "rgb(255,20,30)"),
            "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m");
  EXPECT_EQ(fmt::format(fg(fmt::color::blue), "blue"),
            "\x1b[38;2;000;000;255mblue\x1b[0m");
  EXPECT_EQ(
      fmt::format(fg(fmt::color::blue) | bg(fmt::color::red), "two color"),
      "\x1b[38;2;000;000;255m\x1b[48;2;255;000;000mtwo color\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::bold, "bold"), "\x1b[1mbold\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::italic, "italic"),
            "\x1b[3mitalic\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::underline, "underline"),
            "\x1b[4munderline\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::strikethrough, "strikethrough"),
            "\x1b[9mstrikethrough\x1b[0m");
  EXPECT_EQ(
      fmt::format(fg(fmt::color::blue) | fmt::emphasis::bold, "blue/bold"),
      "\x1b[1m\x1b[38;2;000;000;255mblue/bold\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::bold, "bold error"),
            "\x1b[1mbold error\x1b[0m");
  EXPECT_EQ(fmt::format(fg(fmt::color::blue), "blue log"),
            "\x1b[38;2;000;000;255mblue log\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::text_style(), "hi"), "hi");
  EXPECT_EQ(fmt::format(fg(fmt::terminal_color::red), "tred"),
            "\x1b[31mtred\x1b[0m");
  EXPECT_EQ(fmt::format(bg(fmt::terminal_color::cyan), "tcyan"),
            "\x1b[46mtcyan\x1b[0m");
  EXPECT_EQ(fmt::format(fg(fmt::terminal_color::bright_green), "tbgreen"),
            "\x1b[92mtbgreen\x1b[0m");
  EXPECT_EQ(fmt::format(bg(fmt::terminal_color::bright_magenta), "tbmagenta"),
            "\x1b[105mtbmagenta\x1b[0m");
  EXPECT_EQ(fmt::format(fg(fmt::terminal_color::red), "{}", "foo"),
            "\x1b[31mfoo\x1b[0m");
}
