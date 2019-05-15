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

namespace fmt {

struct scan_arg {
  int& value;
  // TODO: more types
};

struct scan_args {
  int size;
  const scan_arg* data;

  template <size_t N>
  scan_args(const std::array<scan_arg, N>& store)
      : size(N), data(store.data()) {
    static_assert(N < INT_MAX, "too many arguments");
  }
};

template <typename Handler>
void parse_scan_format(string_view format_str, Handler& handler) {
  const char* p = format_str.data();
  const char* end = p + format_str.size();
  while (p != end) {
    char c = *p++;
    if (c != '{' || p == end || *p++ != '}')
      throw format_error("invalid format string");
    handler.on_arg();
  }
}

void vscan(string_view input, string_view format_str, scan_args args) {
  struct handler {
    const char* begin;
    const char* end;
    scan_args args;
    int next_arg_id;

    handler(string_view input, scan_args args)
        : begin(input.data()),
          end(begin + input.size()),
          args(args),
          next_arg_id(0) {}

    void on_arg() {
      int value = 0;
      while (begin != end) {
        char c = *begin++;
        if (c < '0' || c > '9') throw format_error("invalid input");
        value = value * 10 + (c - '0');
      }
      if (next_arg_id >= args.size)
        throw format_error("argument index out of range");
      args.data[0].value = value;
    }
  } h(input, args);
  parse_scan_format(format_str, h);
}

template <typename... Args>
std::array<scan_arg, sizeof...(Args)> make_scan_args(Args&... args) {
  return {args...};
}

template <typename... Args>
void scan(string_view input, string_view format_str, Args&... args) {
  vscan(input, format_str, make_scan_args(args...));
}
}  // namespace fmt

TEST(ScanTest, ReadInt) {
  int n = 0;
  fmt::scan("42", "{}", n);
  EXPECT_EQ(n, 42);
}
