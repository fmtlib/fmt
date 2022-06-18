// Formatting library for C++ - experimental variant API
//
// {fmt} support for variant interface.

#include "fmt/variant.h"

#include <string>

#include "gtest/gtest.h"

#if FMT_HAS_VARIANT

TEST(variant_test, format_monostate) {
  EXPECT_EQ(fmt::format("{}", std::monostate{}), " ");
}
TEST(variant_test, format_variant) {
  using V0 = std::variant<int, float, std::string, char>;
  V0 v0(42);
  V0 v1(1.5f);
  V0 v2("hello");
  V0 v3('i');
  EXPECT_EQ(fmt::format("{}", v0), "<42>");
  EXPECT_EQ(fmt::format("{}", v1), "<1.5>");
  EXPECT_EQ(fmt::format("{}", v2), "<\"hello\">");
  EXPECT_EQ(fmt::format("{}", v3), "<'i'>");

  struct unformattable{};
  EXPECT_FALSE((fmt::is_formattable<unformattable>::value));
  EXPECT_FALSE((fmt::is_formattable<std::variant<unformattable>>::value));
  EXPECT_FALSE((fmt::is_formattable<std::variant<unformattable,int>>::value));
  EXPECT_FALSE((fmt::is_formattable<std::variant<int,unformattable>>::value));
  EXPECT_FALSE((fmt::is_formattable<std::variant<unformattable,unformattable>>::value));
  EXPECT_TRUE((fmt::is_formattable<std::variant<int,float>>::value));

  using V1 = std::variant<std::monostate, std::string, std::string>;
  V1 v4{};
  V1 v5{std::in_place_index<1>,"yes, this is variant"};

  EXPECT_EQ(fmt::format("{}", v4), "< >");
  EXPECT_EQ(fmt::format("{}", v5), "<\"yes, this is variant\">");
}

#endif // FMT_HAS_VARIANT
