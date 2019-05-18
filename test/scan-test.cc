// Formatting library for C++ - scanning API proof of concept
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include <array>
#include <climits>

#include "fmt/format.h"
#include "gmock.h"
#include "gtest-extra.h"

FMT_BEGIN_NAMESPACE
namespace internal {
struct scan_arg {
  int* value;
  // TODO: more types
};
}  // namespace internal

struct scan_args {
  int size;
  const internal::scan_arg* data;

  template <size_t N>
  scan_args(const std::array<internal::scan_arg, N>& store)
      : size(N), data(store.data()) {
    static_assert(N < INT_MAX, "too many arguments");
  }
};

namespace internal {
struct scan_handler : error_handler {
  const char* begin;
  const char* end;
  scan_args args;
  int next_arg_id;

  scan_handler(string_view input, scan_args args)
      : begin(input.data()),
        end(begin + input.size()),
        args(args),
        next_arg_id(0) {}

  void on_text(const char*, const char*) { on_error("invalid format"); }

  void on_arg_id() {
    if (next_arg_id >= args.size) on_error("argument index out of range");
  }
  void on_arg_id(unsigned) { on_error("invalid format"); }
  void on_arg_id(string_view) { on_error("invalid format"); }

  void on_replacement_field(const char*) {
    int value = 0;
    while (begin != end) {
      char c = *begin++;
      if (c < '0' || c > '9') on_error("invalid input");
      value = value * 10 + (c - '0');
    }
    *args.data[0].value = value;
  }

  const char* on_format_specs(const char* begin, const char*) {
    return begin;
  }
};
}  // namespace internal

void vscan(string_view input, string_view format_str, scan_args args) {
  internal::parse_format_string<false>(format_str,
                                       internal::scan_handler(input, args));
}

template <typename... Args>
std::array<internal::scan_arg, sizeof...(Args)> make_scan_args(Args&... args) {
  return std::array<internal::scan_arg, sizeof...(Args)>{&args...};
}

template <typename... Args>
void scan(string_view input, string_view format_str, Args&... args) {
  vscan(input, format_str, make_scan_args(args...));
}
FMT_END_NAMESPACE

TEST(ScanTest, ReadInt) {
  int n = 0;
  fmt::scan("42", "{}", n);
  EXPECT_EQ(n, 42);
}

TEST(ScanTest, InvalidFormat) {
  EXPECT_THROW_MSG(fmt::scan("", "{}"), fmt::format_error,
                   "argument index out of range");
  EXPECT_THROW_MSG(fmt::scan("", "{"), fmt::format_error,
                   "invalid format string");
}
