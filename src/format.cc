// Formatting library for C++
//
// Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/format-inl.h"

FMT_BEGIN_NAMESPACE

#if FMT_USE_LOCALE
template FMT_API locale_ref::locale_ref(const std::locale& loc);  // DEPRECATED!
template FMT_API auto locale_ref::get<std::locale>() const -> std::locale;
#endif

template FMT_API auto zmij::detail::write(char* buffer, float value) noexcept
    -> char*;
template FMT_API auto zmij::detail::write(char* buffer, double value) noexcept
    -> char*;

namespace detail {

template FMT_API auto to_decimal(float x) noexcept -> decimal_fp<float>;
template FMT_API auto to_decimal(double x) noexcept -> decimal_fp<double>;

// The Dragonbox API called from the headers of earlier 12.x versions.
namespace dragonbox {
template <typename T> struct decimal_fp {
  typename zmij::detail::float_traits<T>::sig_type significand;
  int exponent;
};

template <typename T> auto to_decimal(T x) noexcept -> decimal_fp<T> {
  auto dec = detail::to_decimal(x);
  return {dec.significand, dec.exponent};
}
template FMT_API auto to_decimal(float x) noexcept -> decimal_fp<float>;
template FMT_API auto to_decimal(double x) noexcept -> decimal_fp<double>;

FMT_API auto get_cached_power(int k) noexcept -> uint128 {
  // zmij's powers of 10 are rounded down and these are rounded up. They agree
  // on 10^0 to 10^55, whose significands are exact in 128 bits.
  auto p = zmij::static_data.pow10_significands[k];
  uint64_t lo = p.lo + static_cast<uint64_t>(k < 0 || k > 55);
  return {p.hi + static_cast<uint64_t>(lo < p.lo), lo};
}
}  // namespace dragonbox

// Explicit instantiations for char.

template FMT_API auto thousands_sep_impl(locale_ref)
    -> thousands_sep_result<char>;
template FMT_API auto decimal_point_impl(locale_ref) -> char;

// DEPRECATED!
template FMT_API void buffer<char>::append(const char*, const char*);

// Explicit instantiations for wchar_t.

template FMT_API auto thousands_sep_impl(locale_ref)
    -> thousands_sep_result<wchar_t>;
template FMT_API auto decimal_point_impl(locale_ref) -> wchar_t;

// DEPRECATED!
template FMT_API void buffer<wchar_t>::append(const wchar_t*, const wchar_t*);

}  // namespace detail
FMT_END_NAMESPACE
