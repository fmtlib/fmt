// Copyright (c) 2020 Vladimir Solontsov
// SPDX-License-Identifier: MIT Licence

#include <fmt/core.h>

#include "gtest-extra.h"

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
  store.push_back("abc");
  store.push_back(1.5f);
  store.push_back(std::cref(a1));

  std::string result = fmt::vformat(
      "{a1_} and {} and {} and {}",
      store);

  EXPECT_EQ("42 and abc and 1.5 and 42", result);
}

