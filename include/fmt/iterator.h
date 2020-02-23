// Copyright (c) Google LLC.
// SPDX-License-Identifier: MIT Licence
//

#ifndef FMT_ITERATOR_H_
#define FMT_ITERATOR_H_

#include <cstdio>

#include "core.h"
#include "format.h"

FMT_BEGIN_NAMESPACE

namespace internal {
#if defined(__cpp_concepts) and __cpp_concepts >= 201907
template <typename T> concept formattable = requires(T&& t) {
  ::fmt::format("{}", std::forward<T>(t));
};
#endif // __cpp_concepts
}  // namespace internal

// Iterator that allows for writing to file via fmt::print.
//
template <typename CharT> class basic_print_iterator {
 public:
  using iterator_category = std::output_iterator_tag;
  using value_type = void;
  using difference_type = std::ptrdiff_t;
  using pointer = void;
  using reference = void;
  using char_type = CharT;

  basic_print_iterator() = default;

  // Constructs a print-iterator that writes to stdout.
  explicit basic_print_iterator(basic_string_view<CharT> const format_specifier)
      : file_(stdout), format_specifier_(format_specifier) {}

  // Constructs a print-iterator that writes to a designated file.
  explicit basic_print_iterator(
      std::FILE* file, basic_string_view<CharT> const format_specifier = "{}")
      : file_(file), format_specifier_(format_specifier) {}

  basic_print_iterator& operator++() noexcept { return *this; }
  basic_print_iterator& operator++(int) noexcept { return *this; }
  basic_print_iterator& operator*() noexcept { return *this; }

#if defined(__cpp_concepts) and __cpp_concepts >= 201907
  template <internal::formattable T>
#else
  template <typename T>
#endif  // __cpp_concepts
  void operator=(T&& t)
  {
    ::fmt::print(file_, format_specifier_, std::forward<T>(t));
  }

 private:
  std::FILE* file_ = nullptr;
  basic_string_view<CharT> format_specifier_;
};

#if __cplusplus >= 201703
template <typename CharT>
basic_print_iterator(basic_string_view<CharT>) -> basic_print_iterator<CharT>;

template <typename CharT>
basic_print_iterator(std::FILE*, basic_string_view<CharT>)
    -> basic_print_iterator<CharT>;
#endif  // __cplusplus

using print_iterator = basic_print_iterator<char>;
using wprint_iterator = basic_print_iterator<wchar_t>;
using u8print_iterator = basic_print_iterator<char8_t>;
using u16print_iterator = basic_print_iterator<char16_t>;
using u32print_iterator = basic_print_iterator<char32_t>;

FMT_END_NAMESPACE

#endif  // FMT_ITERATOR_H_
