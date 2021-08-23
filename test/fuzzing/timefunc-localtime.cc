// Copyright (c) 2021, Paul Dreik
// For license information refer to format.h.
#include <fmt/chrono.h>

#include "fuzzer-common.h"

/*
 * a fuzzer for fmt::localtime and fmt::gmtime
 */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  constexpr auto N = sizeof(std::time_t);
  if (size != N + 1) return 0;

  const auto action = data[0] & 0x1;
  const std::time_t x = assign_from_buf<std::time_t>(data + 1);

  try {
    switch (action) {
    case 0: {
      auto ignored = fmt::localtime(x);
    } break;
    case 1: {
      auto ignored = fmt::gmtime(x);
    } break;
    }

  } catch (...) {
  }
  return 0;
}
