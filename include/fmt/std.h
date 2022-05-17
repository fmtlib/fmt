// Formatting library for C++ - formatters for standard library types
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_STD_H_
#define FMT_STD_H_

#include "format.h"

#if FMT_HAS_INCLUDE(<version>)
#  include <version>
#endif

#ifdef __cpp_lib_filesystem
#  include <filesystem>

template <>
struct fmt::formatter<std::filesystem::path> : formatter<string_view> {
  template <typename FormatContext>
  auto format(const std::filesystem::path& p, FormatContext& ctx) const ->
      typename FormatContext::iterator {
    return formatter<string_view>::format(p.string(), ctx);
  }
};
#endif

#endif  // FMT_STD_H_
