// Copyright (c) 2020 Vladimir Solontsov
// SPDX-License-Identifier: MIT Licence

#include <fmt/core.h>

#include "gtest-extra.h"

TEST(FormatDynArgsTest, Basic) {
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  store.push_back(42);
  store.push_back("abc1");
  store.push_back(1.5f);

  std::string result = fmt::vformat("{} and {} and {}", store);

  EXPECT_EQ("42 and abc1 and 1.5", result);
}

TEST(FormatDynArgsTest, StringsAndRefs) {
  // Unfortunately the tests are compiled with old ABI
  // So strings use COW.
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  char str[]{"1234567890"};
  store.push_back(str);
  store.push_back(std::cref(str));
  store.push_back(fmt::string_view{str});
  str[0] = 'X';

  std::string result = fmt::vformat("{} and {} and {}", store);

  EXPECT_EQ("1234567890 and X234567890 and X234567890", result);
}

struct custom_type {
  int i{0};
};
FMT_BEGIN_NAMESPACE

template <> struct formatter<custom_type> {
  auto parse(format_parse_context& ctx) const -> decltype(ctx.begin()) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(const custom_type& p, FormatContext& ctx) -> decltype(format_to(
      ctx.out(), std::declval<typename FormatContext::char_type const*>())) {
    return format_to(ctx.out(), "cust={}", p.i);
  }
};
FMT_END_NAMESPACE

TEST(FormatDynArgsTest, CustomFormat) {
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  custom_type c{};
  store.push_back(c);
  ++c.i;
  store.push_back(c);
  ++c.i;
  store.push_back(std::cref(c));
  ++c.i;

  std::string result = fmt::vformat("{} and {} and {}", store);

  EXPECT_EQ("cust=0 and cust=1 and cust=3", result);
}

TEST(FormatDynArgsTest, NamedArgByRef) {
  fmt::dynamic_format_arg_store<fmt::format_context> store;

  // Note: fmt::arg() constructs an object which holds a reference
  // to its value. It's not an aggregate, so it doesn't extend the
  // reference lifetime. As a result, it's a very bad idea passing temporary
  // as a named argument value. Only GCC with optimization level >0
  // complains about this.
  //
  // A real life usecase is when you have both name and value alive
  // guarantee their lifetime and thus don't want them to be copied into
  // storages.
  int a1_val{42};
  auto a1 = fmt::arg("a1_", a1_val);
  store.push_back(std::cref(a1));

  std::string result = fmt::vformat("{a1_}",  // and {} and {}",
                                    store);

  EXPECT_EQ("42", result);
}
