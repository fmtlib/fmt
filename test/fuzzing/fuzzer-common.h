// Copyright (c) 2019, Paul Dreik
// For the license information refer to format.h.

#ifndef FUZZER_COMMON_H
#define FUZZER_COMMON_H

#include <cstdint>      // std::uint8_t
#include <cstring>      // memcpy

// One can format to either a string, or a buffer. The latter is faster, but
// one may be interested in formatting to a string instead to verify it works
// as intended. To avoid a combinatoric explosion, select this at compile time
// instead of dynamically from the fuzz data.
#define FMT_FUZZ_FORMAT_TO_STRING 0

// If {fmt} is given a buffer that is separately allocated, chances that address
// sanitizer detects out of bound reads is much higher. However, it slows down
// the fuzzing.
#define FMT_FUZZ_SEPARATE_ALLOCATION 1

namespace fmt_fuzzer {

// The size of the largest possible type in use.
// To let the the fuzzer mutation be efficient at cross pollinating between
// different types, use a fixed size format. The same bit pattern, interpreted
// as another type, is likely interesting.
constexpr auto nfixed = 16;

// Casts data to a char pointer.
template <typename T> inline const char* as_chars(const T* data) {
  return reinterpret_cast<const char*>(data);
}

// Casts data to a byte pointer.
template <typename T> inline const std::uint8_t* as_bytes(const T* data) {
  return reinterpret_cast<const std::uint8_t*>(data);
}

// Blits bytes from data to form an (assumed trivially constructible) object
// of type Item.
template <class Item> inline Item assignFromBuf(const std::uint8_t* data) {
  auto item = Item();
  std::memcpy(&item, data, sizeof(Item));
  return item;
}

// Reads a boolean value by looking at the first byte from data.
template <> inline bool assignFromBuf<bool>(const std::uint8_t* data) {
  return *data != 0;
}

}  // namespace fmt_fuzzer

#endif  // FUZZER_COMMON_H
