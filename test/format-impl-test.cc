/*
 Formatting library implementation tests.

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

// Include format.cc instead of format.h to test implementation-specific stuff.
#include "format.cc"
#include "gtest-extra.h"

#undef max

TEST(FormatTest, ArgConverter) {
  using fmt::internal::Arg;
  Arg arg = Arg();
  arg.type = Arg::LONG_LONG;
  arg.long_long_value = std::numeric_limits<fmt::LongLong>::max();
  ArgConverter<fmt::LongLong>(arg, 'd').visit(arg);
  EXPECT_EQ(Arg::LONG_LONG, arg.type);
}

TEST(FormatTest, FormatNegativeNaN) {
  double nan = std::numeric_limits<double>::quiet_NaN();
  if (getsign(-nan))
    EXPECT_EQ("-nan", fmt::format("{}", -nan));
  else
    fmt::print("Warning: compiler doesn't handle negative NaN correctly");
}

TEST(FormatTest, FormatErrorCode) {
  std::string msg = "error 42", sep = ": ";
  {
    fmt::Writer w;
    w << "garbage";
    format_error_code(w, 42, "test");
    EXPECT_EQ("test: " + msg, w.str());
  }
  {
    fmt::Writer w;
    std::string prefix(
        fmt::internal::INLINE_BUFFER_SIZE - msg.size() - sep.size() + 1, 'x');
    format_error_code(w, 42, prefix);
    EXPECT_EQ(msg, w.str());
  }
  {
    fmt::Writer w;
    std::string prefix(
        fmt::internal::INLINE_BUFFER_SIZE - msg.size() - sep.size(), 'x');
    format_error_code(w, 42, prefix);
    EXPECT_EQ(prefix + sep + msg, w.str());
    EXPECT_EQ(fmt::internal::INLINE_BUFFER_SIZE, w.size());
  }
}
