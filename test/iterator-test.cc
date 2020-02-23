// Copyright (c) Google LLC.
// SPDX-License-Identifier: MIT Licence
//
#include "fmt/iterator.h"

#include <algorithm>
#include <deque>
#include <iterator>
#include <vector>

#include "fmt/ranges.h"
#include "gtest-extra.h"
#include "gtest/gtest.h"

namespace {
#if defined(__cpp_lib_ranges) and __cpp_lib_ranges >= 201911
template <typename I> void check_iterator_conformance() {
  static_assert(not std::readable<I>);
  static_assert(std::output_iterator<I, int>);
  static_assert(std::output_iterator<I, double>);
  static_assert(std::output_iterator<I, std::string>);
  static_assert(not std::output_iterator<I, std::vector<int>>);
}

template [[maybe_unused]] void
check_iterator_conformance<fmt::print_iterator>();
template [[maybe_unused]] void
check_iterator_conformance<fmt::wprint_iterator>();
template [[maybe_unused]] void
check_iterator_conformance<fmt::u8print_iterator>();
template [[maybe_unused]] void
check_iterator_conformance<fmt::u16print_iterator>();
template [[maybe_unused]] void
check_iterator_conformance<fmt::u32print_iterator>();
#endif  // __cpp_lib_ranges

template <typename I> void check_default_operations() {
  auto i = I{};
  {
    auto& prefix = ++i;
    EXPECT_EQ(std::addressof(i), std::addressof(prefix));
  }
  {
    auto& postfix = i++;
    EXPECT_EQ(std::addressof(i), std::addressof(postfix));
  }
  {
    auto& deref = *i;
    EXPECT_EQ(std::addressof(i), std::addressof(deref));
  }
}

TEST(PrintIterator, DefaultOperators) {
  check_default_operations<fmt::print_iterator>();
  check_default_operations<fmt::wprint_iterator>();
  check_default_operations<fmt::u16print_iterator>();
  check_default_operations<fmt::u32print_iterator>();
#ifdef __cpp_char8_t
  check_default_operations<fmt::u8print_iterator>();
#endif  // __cpp_char8_t
}

enum class decorator { off, on };

template <decorator is_decorated>
void check_assignment(fmt::print_iterator simple_writer, std::FILE* out);

template <>
void check_assignment<decorator::off>(fmt::print_iterator simple_writer,
                                      std::FILE* out) {
  EXPECT_WRITE(out, simple_writer = 0, "0");
  EXPECT_WRITE(out, simple_writer = 0.0, "0.0");
  EXPECT_WRITE(out, simple_writer = "hello there", "hello there");

  auto v = std::vector<std::vector<int>>{{0, 1, 2}, {3, 4, 5}, {6, 7, 8, 9}};
  EXPECT_WRITE(out, simple_writer = v, "{{0, 1, 2}, {3, 4, 5}, {6, 7, 8, 9}}");

  // GCC 4.8 silencing
  (void)simple_writer, (void)out;
}

template <>
void check_assignment<decorator::on>(fmt::print_iterator decorated_writer,
                                     std::FILE* out) {
  auto d = std::deque<int>{16, 8, 4, 2, 1};
  EXPECT_WRITE(out, std::copy(begin(d), end(d), decorated_writer),
               "-16!-8!-4!-2!-1!");
  EXPECT_WRITE(out, decorated_writer = 0.0, "-0.0!");
  EXPECT_WRITE(out, decorated_writer = "Pusheen the Cat", "-Pusheen the Cat!");

  // GCC 4.8 silencing
  (void)decorated_writer, (void)out;
}

TEST(PrintIterator, DefaultDestinationAssignment) {
  check_assignment<decorator::off>(fmt::print_iterator("{}"), stdout);
  check_assignment<decorator::on>(fmt::print_iterator("-{}!"), stdout);

  check_assignment<decorator::off>(fmt::print_iterator(stderr, "{}"), stderr);
  check_assignment<decorator::on>(fmt::print_iterator(stderr, "-{}!"), stderr);
}
}  // namespace
