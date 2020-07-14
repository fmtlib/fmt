#ifndef _CRT_SECURE_NO_WARNINGS
#  define _CRT_SECURE_NO_WARNINGS
#endif

#include <algorithm>

#include "fmt/format.h"
#include "gtest-extra.h"

enum struct color { red, green, blue };

template <> struct fmt::formatter<color> {
  constexpr auto parse(format_parse_context& ctx) {
    auto it = ctx.begin();
    if (it == ctx.end()) throw fmt::format_error("incomplete format string");
    if (*it != '}') throw fmt::format_error("unknown format specs");
    return it;
  }
  template <typename FormatContext> auto format(color c, FormatContext& ctx) {
    static const char red_s[] = "red", green_s[] = "green", blue_s[] = "blue";
    const char *begin_p, *end_p;
    switch (c) {
    case color::red:
      begin_p = std::begin(red_s);
      end_p = std::end(red_s);
      break;
    case color::green:
      begin_p = std::begin(green_s);
      end_p = std::end(green_s);
      break;
    case color::blue:
      begin_p = std::begin(blue_s);
      end_p = std::end(blue_s);
      break;
    }
    return std::copy(begin_p, end_p - 1, ctx.out());
  }
};

TEST(UserDefinedTypeTest, Format) {
  EXPECT_EQ("red green blue",
            fmt::format("{} {} {}", color::red, color::green, color::blue));
}
