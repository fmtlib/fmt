#ifndef FUZZER_COMMON_H
#define FUZZER_COMMON_H

// Copyright (c) 2019, Paul Dreik
// License: see LICENSE.rst in the fmt root directory

#include <cstdint>      // std::uint8_t
#include <cstring>      // memcpy
#include <type_traits>  // trivially copyable

// one can format to either a string, or a buf. buf is faster,
// but one may be interested in formatting to a string instead to
// verify it works as intended. to avoid a combinatoric explosion,
// select this at compile time instead of dynamically from the fuzz data
#define FMT_FUZZ_FORMAT_TO_STRING 0

// if fmt is given a buffer that is separately allocated,
// chances that address sanitizer detects out of bound reads is
// much higher. However, it slows down the fuzzing.
#define FMT_FUZZ_SEPARATE_ALLOCATION 1

// To let the the fuzzer mutation be efficient at cross pollinating
// between different types, use a fixed size format.
// The same bit pattern, interpreted as another type,
// is likely interesting.
// For this, we must know the size of the largest possible type in use.

// There are some problems on travis, claiming Nfixed is not a constant
// expression which seems to be an issue with older versions of libstdc++
#if _GLIBCXX_RELEASE >= 7
#  include <algorithm>
namespace fmt_fuzzer {
constexpr auto Nfixed = std::max(sizeof(long double), sizeof(std::intmax_t));
}
#else
namespace fmt_fuzzer {
constexpr auto Nfixed = 16;
}
#endif

namespace fmt_fuzzer {
// view data as a c char pointer.
template <typename T> inline const char* as_chars(const T* data) {
  return static_cast<const char*>(static_cast<const void*>(data));
}

// view data as a byte pointer
template <typename T> inline const std::uint8_t* as_bytes(const T* data) {
  return static_cast<const std::uint8_t*>(static_cast<const void*>(data));
}

// blits bytes from Data to form an (assumed trivially constructible) object
// of type Item
template <class Item> inline Item assignFromBuf(const std::uint8_t* Data) {
  Item item{};
  std::memcpy(&item, Data, sizeof(Item));
  return item;
}

// reads a boolean value by looking at the first byte from Data
template <> inline bool assignFromBuf<bool>(const std::uint8_t* Data) {
  return !!Data[0];
}

}  // namespace fmt_fuzzer

#endif  // FUZZER_COMMON_H
