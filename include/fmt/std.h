// Formatting library for C++ - formatters for standard library types
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#ifndef FMT_STD_H_
#define FMT_STD_H_

#include <thread>

#include "ostream.h"

#if FMT_HAS_INCLUDE(<version>)
#  include <version>
#endif

#ifdef __cpp_lib_filesystem
#  include <filesystem>

FMT_BEGIN_NAMESPACE

namespace detail {

template <typename Char>
void write_escaped_path(basic_memory_buffer<Char>& quoted,
                        const std::filesystem::path& p) {
  write_escaped_string<Char>(std::back_inserter(quoted), p.string<Char>());
}
#  ifdef _WIN32
template <>
inline void write_escaped_path<char>(basic_memory_buffer<char>& quoted,
                                     const std::filesystem::path& p) {
  auto s = p.u8string();
  write_escaped_string<char>(
      std::back_inserter(quoted),
      string_view(reinterpret_cast<const char*>(s.c_str()), s.size()));
}
#  endif
template <>
inline void write_escaped_path<std::filesystem::path::value_type>(
    basic_memory_buffer<std::filesystem::path::value_type>& quoted,
    const std::filesystem::path& p) {
  write_escaped_string<std::filesystem::path::value_type>(
      std::back_inserter(quoted), p.native());
}

}  // namespace detail

template <typename Char>
struct formatter<std::filesystem::path, Char>
    : formatter<basic_string_view<Char>> {
  template <typename FormatContext>
  auto format(const std::filesystem::path& p, FormatContext& ctx) const ->
      typename FormatContext::iterator {
    basic_memory_buffer<Char> quoted;
    detail::write_escaped_path(quoted, p);
    return formatter<basic_string_view<Char>>::format(
        basic_string_view<Char>(quoted.data(), quoted.size()), ctx);
  }
};
FMT_END_NAMESPACE
#endif

FMT_BEGIN_NAMESPACE
template <typename Char>
struct formatter<std::thread::id, Char> : basic_ostream_formatter<Char> {};
FMT_END_NAMESPACE

#endif  // FMT_STD_H_
