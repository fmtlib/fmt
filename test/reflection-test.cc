// Formatting library for C++ - formatting library tests
//
// Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

#include <meta>

#include "fmt/compile.h"
#include "fmt/std.h"

template <> struct fmt::formatter<std::meta::info> {
  consteval auto parse(auto& ctx) { return ctx.begin(); }
  consteval auto format(const std::meta::info&, auto& ctx) const {
    return fmt::formatter<std::string_view>{}.format("magic_type", ctx);
  }
};

// just so we can reflect it
struct magic_type {};

static_assert("[magic_type]" == fmt::format(FMT_COMPILE("[{}]"), ^^magic_type));
static_assert("magic_type" == fmt::format(FMT_COMPILE("{}"), ^^magic_type));
