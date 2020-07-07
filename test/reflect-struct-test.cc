// Formatting library for C++ - std::ostream support tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#define FMT_STRING_ALIAS 1
#include "fmt/format.h"

struct Inner {
  int x;
  double y;
  std::string z;
};

struct Outer {
  std::string a;
  std::string b;
  Inner inner;
};

// fixme: if this is enabled, then then fmt::format("{:e}", Outer{...}) fails,
//        because internally formatter<Inner> will be used to parse the "{:e}"
//        format string, which obviously is not a valid int format string.
//        This issue should be resolved by not using the NamedField formatter
//        specialization in structured.h
//// Test that there is no issues with specializations when fmt/ostream.h is
//// included after fmt/format.h.
// namespace fmt {
// template <> struct formatter<Inner> : formatter<int> {
//  template <typename FormatContext>
//  typename FormatContext::iterator format(const Inner&, FormatContext& ctx) {
//    return formatter<int>::format(42, ctx);
//  }
//};
//}  // namespace fmt

#include <tuple>

#include "fmt/structured.h"
#include "gmock.h"
#include "gtest-extra.h"

using fmt::format;
using fmt::format_error;

// provide explicit reflection for custom types
template <> struct fmt::reflection<Outer> {
  static constexpr bool available = true;
  static auto name() { return "Outer"; }
  static auto fields(const Outer& outer) {
    return std::make_tuple(NamedField<std::string>{"a", outer.a},
                           NamedField<std::string>{"b", outer.b},
                           NamedField<Inner>{"inner", outer.inner});
  }
  static auto values(const Outer& outer) {
    return std::make_tuple(outer.a, outer.b, outer.inner);
  }
};

template <> struct fmt::reflection<Inner> {
  static constexpr bool available = true;
  static auto name() { return "Inner"; }
  static auto fields(const Inner& inner) {
    return std::make_tuple(NamedField<int>{"x", inner.x},
                           NamedField<double>{"y", inner.y},
                           NamedField<std::string>{"z", inner.z});
  }

  static auto values(const Inner& inner) {
    return std::make_tuple(inner.x, inner.y, inner.z);
  }
};

using namespace std::string_literals;

TEST(StructuredTest, FallbackPickup) {
  Inner inner{1, 3.1, "hello"};
  Outer outer{"a", "b", inner};
  EXPECT_EQ("Outer{.a=a, .b=b, .inner=Inner{.x=1, .y=3.1, .z=hello}}",
            fmt::format("{:e}", outer));
}