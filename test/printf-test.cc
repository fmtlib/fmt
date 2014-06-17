/*
 printf tests.

 Copyright (c) 2012-2014, Victor Zverovich
 All rights reserved.

 Redistribution and use in source and binary forms, with or without
 modification, are permitted provided that the following conditions are met:

 1. Redistributions of source code must retain the above copyright notice, this
    list of conditions and the following disclaimer.
 2. Redistributions in binary form must reproduce the above copyright notice,
    this list of conditions and the following disclaimer in the documentation
    and/or other materials provided with the distribution.

 THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <climits>
#include <cstring>

#include "format.h"
#include "gtest-extra.h"
#include "util.h"

#if FMT_USE_VARIADIC_TEMPLATES && FMT_USE_RVALUE_REFERENCES

using fmt::Format;
using fmt::FormatError;

// Returns a number UINT_MAX + 1 formatted as a string.
std::string GetBigNumber() {
  char format[BUFFER_SIZE];
  sprintf(format, "%u", UINT_MAX);
  Increment(format + 1);
  return format;
}

const unsigned BIG_NUM = INT_MAX + 1u;

// Makes format string argument positional.
std::string MakePositional(fmt::StringRef format) {
  std::string s(format);
  s.replace(s.find('%'), 1, "%1$");
  return s;
}

#define EXPECT_PRINTF(expected_output, format, arg) \
  EXPECT_EQ(expected_output, str(fmt::sprintf(format, arg))); \
  EXPECT_EQ(expected_output, str(fmt::sprintf(MakePositional(format), arg)))

TEST(PrintfTest, NoArgs) {
  EXPECT_EQ("test", str(fmt::sprintf("test")));
}

TEST(PrintfTest, Escape) {
  EXPECT_EQ("%", str(fmt::sprintf("%%")));
  EXPECT_EQ("before %", str(fmt::sprintf("before %%")));
  EXPECT_EQ("% after", str(fmt::sprintf("%% after")));
  EXPECT_EQ("before % after", str(fmt::sprintf("before %% after")));
  EXPECT_EQ("%s", str(fmt::sprintf("%%s")));
}

TEST(PrintfTest, PositionalArgs) {
  EXPECT_EQ("42", str(fmt::sprintf("%1$d", 42)));
  EXPECT_EQ("before 42", str(fmt::sprintf("before %1$d", 42)));
  EXPECT_EQ("42 after", str(fmt::sprintf("%1$d after",42)));
  EXPECT_EQ("before 42 after", str(fmt::sprintf("before %1$d after", 42)));
  EXPECT_EQ("answer = 42", str(fmt::sprintf("%1$s = %2$d", "answer", 42)));
  EXPECT_EQ("42 is the answer",
      str(fmt::sprintf("%2$d is the %1$s", "answer", 42)));
  EXPECT_EQ("abracadabra", str(fmt::sprintf("%1$s%2$s%1$s", "abra", "cad")));
}

TEST(PrintfTest, AutomaticArgIndexing) {
  EXPECT_EQ("abc", str(fmt::sprintf("%c%c%c", 'a', 'b', 'c')));
}

TEST(PrintfTest, NumberIsTooBigInArgIndex) {
  EXPECT_THROW_MSG(fmt::sprintf(str(Format("%{}$", BIG_NUM))),
      FormatError, "invalid format string");
  EXPECT_THROW_MSG(fmt::sprintf(str(Format("%{}$d", BIG_NUM))),
      FormatError, "number is too big in format");
}

TEST(PrintfTest, SwitchArgIndexing) {
  EXPECT_THROW_MSG(fmt::sprintf("%1$d%", 1, 2),
      FormatError, "invalid format string");
  EXPECT_THROW_MSG(fmt::sprintf(str(Format("%1$d%{}d", BIG_NUM)), 1, 2),
      FormatError, "cannot switch from manual to automatic argument indexing");
  EXPECT_THROW_MSG(fmt::sprintf("%1$d%d", 1, 2),
      FormatError, "cannot switch from manual to automatic argument indexing");

  EXPECT_THROW_MSG(fmt::sprintf("%d%1$", 1, 2),
      FormatError, "invalid format string");
  EXPECT_THROW_MSG(fmt::sprintf(str(Format("%d%{}$d", BIG_NUM)), 1, 2),
      FormatError, "number is too big in format");
  EXPECT_THROW_MSG(fmt::sprintf("%d%1$d", 1, 2),
      FormatError, "cannot switch from automatic to manual argument indexing");

  // Indexing errors override width errors.
  EXPECT_THROW_MSG(fmt::sprintf(str(Format("%d%1${}d", BIG_NUM)), 1, 2),
      FormatError, "cannot switch from automatic to manual argument indexing");
  EXPECT_THROW_MSG(fmt::sprintf(str(Format("%1$d%{}d", BIG_NUM)), 1, 2),
      FormatError, "cannot switch from manual to automatic argument indexing");
}

TEST(PrintfTest, InvalidArgIndex) {
  EXPECT_THROW_MSG(fmt::sprintf("%0$d", 42), FormatError,
      "argument index is out of range in format");
  EXPECT_THROW_MSG(fmt::sprintf("%2$d", 42), FormatError,
      "argument index is out of range in format");
  EXPECT_THROW_MSG(fmt::sprintf(str(Format("%{}$d", INT_MAX)), 42),
      FormatError, "argument index is out of range in format");

  EXPECT_THROW_MSG(fmt::sprintf("%2$", 42),
      FormatError, "invalid format string");
  EXPECT_THROW_MSG(fmt::sprintf(str(Format("%{}$d", BIG_NUM)), 42),
      FormatError, "number is too big in format");
}

TEST(PrintfTest, DefaultAlignRight) {
  EXPECT_PRINTF("   42", "%5d", 42);
  EXPECT_PRINTF("  abc", "%5s", "abc");
}

TEST(PrintfTest, Width) {
  EXPECT_PRINTF("  abc", "%5s", "abc");
  EXPECT_EQ("   42", str(fmt::sprintf("%*d", 5, 42)));

  // Width cannot be specified twice.
  EXPECT_THROW_MSG(fmt::sprintf("%5-5d", 42), FormatError,
      "unknown format code '-' for integer");

  EXPECT_THROW_MSG(fmt::sprintf(str(Format("%{}d", BIG_NUM)), 42),
      FormatError, "number is too big in format");
  EXPECT_THROW_MSG(fmt::sprintf(str(Format("%1${}d", BIG_NUM)), 42),
      FormatError, "number is too big in format");
}

TEST(PrintfTest, ZeroFlag) {
  EXPECT_PRINTF("00042", "%05d", 42);
  EXPECT_PRINTF("-0042", "%05d", -42);

  EXPECT_PRINTF("00042", "%05d", 42);
  EXPECT_PRINTF("-0042", "%05d", -42);
  EXPECT_PRINTF("-004.2", "%06g", -4.2);

  EXPECT_PRINTF("+00042", "%00+6d", 42);

  // '0' flag is ignored for non-numeric types.
  EXPECT_PRINTF("    x", "%05c", 'x');
}

TEST(PrintfTest, PlusFlag) {
  EXPECT_PRINTF("+42", "%+d", 42);
  EXPECT_PRINTF("-42", "%+d", -42);
  EXPECT_PRINTF("+0042", "%+05d", 42);
  EXPECT_PRINTF("+0042", "%0++5d", 42);

  // '+' flag is ignored for non-numeric types.
  EXPECT_PRINTF("x", "%+c", 'x');
}

TEST(PrintfTest, MinusFlag) {
  EXPECT_PRINTF("abc  ", "%-5s", "abc");
  EXPECT_PRINTF("abc  ", "%0--5s", "abc");
}

TEST(PrintfTest, SpaceFlag) {
  EXPECT_PRINTF(" 42", "% d", 42);
  EXPECT_PRINTF("-42", "% d", -42);
  EXPECT_PRINTF(" 0042", "% 05d", 42);
  EXPECT_PRINTF(" 0042", "%0  5d", 42);

  // ' ' flag is ignored for non-numeric types.
  EXPECT_PRINTF("x", "% c", 'x');
}

TEST(PrintfTest, HashFlag) {
  EXPECT_PRINTF("042", "%#o", 042);
  EXPECT_PRINTF("-042", "%#o", -042);
  EXPECT_PRINTF("0", "%#o", 0);

  EXPECT_PRINTF("0x42", "%#x", 0x42);
  EXPECT_PRINTF("0X42", "%#X", 0x42);
  EXPECT_PRINTF("-0x42", "%#x", -0x42);
  EXPECT_PRINTF("0", "%#x", 0);

  EXPECT_PRINTF("0x0042", "%#06x", 0x42);
  EXPECT_PRINTF("0x0042", "%0##6x", 0x42);

  EXPECT_PRINTF("-42.000000", "%#f", -42.0);
  EXPECT_PRINTF("-42.000000", "%#F", -42.0);

  char buffer[BUFFER_SIZE];
  SPrintf(buffer, "%#e", -42.0);
  EXPECT_PRINTF(buffer, "%#e", -42.0);
  SPrintf(buffer, "%#E", -42.0);
  EXPECT_PRINTF(buffer, "%#E", -42.0);

  EXPECT_PRINTF("-42.0000", "%#g", -42.0);
  EXPECT_PRINTF("-42.0000", "%#G", -42.0);

  SPrintf(buffer, "%#a", 16.0);
  EXPECT_PRINTF(buffer, "%#a", 16.0);
  SPrintf(buffer, "%#A", 16.0);
  EXPECT_PRINTF(buffer, "%#A", 16.0);

  // '#' flag is ignored for non-numeric types.
  EXPECT_PRINTF("x", "%#c", 'x');
}

// TODO: test '*' width, precision, length and type specifier

#endif
