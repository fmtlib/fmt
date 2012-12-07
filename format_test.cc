/*
 Formatting library tests.
 Author: Victor Zverovich
 */

#include <cfloat>
#include <climits>
#include <cstring>
#include <gtest/gtest.h>
#include "format.h"

using std::size_t;
using std::sprintf;

using fmt::Formatter;
using fmt::Format;

TEST(FormatterTest, FormatNoArgs) {
  Formatter format;
  format("test");
  EXPECT_STREQ("test", format.c_str());
}

TEST(FormatterTest, FormatComplex) {
  EXPECT_STREQ("1.2340000000:0042:+3.13:str:0x3e8:X:%",
      c_str(Format("{0:0.10f}:{1:04}:{2:+g}:{3}:{4}:{5}:%")
          << 1.234 << 42 << 3.13 << "str" << reinterpret_cast<void*>(1000)
          << 'X'));
  printf("%0.*f:%04d:%+g:%s:%p:%c:%%\n",
                  10, 1.234, 42, 3.13, "str", (void*)1000, (int)'X');
  printf("%0.10f:%04d:%+g:%s:%p:%c:%%\n",
                  1.234, 42, 3.13, "str", (void*)1000, (int)'X');
}

TEST(FormatterTest, FormatInt) {
  EXPECT_STREQ("42", c_str(Format("{0}") << 42));
  EXPECT_STREQ("before 42 after", c_str(Format("before {0} after") << 42));
  printf("%0.10f:%04d:%+g:%s:%p:%c:%%\n",
                  1.234, 42, 3.13, "str", (void*)1000, (int)'X');
}

// TODO
