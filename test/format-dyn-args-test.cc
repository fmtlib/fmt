// Copyright (c) 2020 Vladimir Solontsov
// SPDX-License-Identifier: MIT Licence

#include <fmt/dyn-args.h>

#include "gtest-extra.h"


TEST(FormatDynArgsTest, Basic) {
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  store.push_back(42);
  store.push_back("abc1");
  store.push_back(1.5f);

  std::string result = fmt::vformat(
      "{} and {} and {}",
      store);

  EXPECT_EQ("42 and abc1 and 1.5", result);
}

TEST(FormatDynArgsTest, StringsAndRefs) {
  // Unfortunately the tests are compiled with old ABI
  // So strings use COW.
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  char str[]{"1234567890"};
  store.push_back(str);
  store.push_back(std::cref(str));
  store.push_back(std::string_view{str});
  str[0] = 'X';

  std::string result = fmt::vformat(
      "{} and {} and {}",
      store);

  EXPECT_EQ("1234567890 and X234567890 and X234567890", result);
}

struct Custom{ int i{0}; };
template <>
struct fmt::formatter<Custom> {
  constexpr auto parse(format_parse_context& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
    auto format(const Custom& p, FormatContext& ctx) {
      // ctx.out() is an output iterator to write to.
      return format_to(
          ctx.out(),
          "cust={}", p.i);
    }
};

#ifdef FMT_HAS_VARIANT

TEST(FormatDynArgsTest, CustomFormat) {
  fmt::dynamic_format_arg_store<fmt::format_context, Custom> store;
  Custom c{};
  store.push_back(c);
  ++c.i;
  store.push_back(c);
  ++c.i;
  store.push_back(std::cref(c));
  ++c.i;

  std::string result = fmt::vformat(
      "{} and {} and {}",
      store);

  EXPECT_EQ("cust=0 and cust=1 and cust=3", result);
}

#endif // FMT_HAS_VARIANT

TEST(FormatDynArgsTest, NamedArgByRef) {
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  auto a1 = fmt::arg("a1_", 42);
  store.push_back(std::cref(a1));

  std::string result = fmt::vformat(
      "{a1_}", // and {} and {}",
      store);

  EXPECT_EQ("42", result);
}

TEST(FormatDynArgsTest, NamedInt) {
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  store.push_back(fmt::arg("a1", 42));
  std::string result = fmt::vformat("{a1}", store);
  EXPECT_EQ("42", result);
}

TEST(FormatDynArgsTest, NamedStrings) {
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  char str[]{"1234567890"};
  store.push_back(fmt::arg("a1", str));
  store.push_back(fmt::arg("a2", std::cref(str)));
  str[0] = 'X';

  std::string result = fmt::vformat(
      "{a1} and {a2}",
      store);

  EXPECT_EQ("1234567890 and X234567890", result);
}

TEST(FormatDynArgsTest, NamedCustomFormat) {
  fmt::dynamic_format_arg_store<fmt::format_context, Custom> store;
  Custom c{};
  store.push_back(fmt::arg("a1", c));
  ++c.i;
  store.push_back(fmt::arg("a2", c));
  ++c.i;
  store.push_back(fmt::arg("a3", std::cref(c)));
  ++c.i;

  std::string result = fmt::vformat(
      "{a1} and {a2} and {a3}",
      store);

  EXPECT_EQ("cust=0 and cust=1 and cust=3", result);
}
