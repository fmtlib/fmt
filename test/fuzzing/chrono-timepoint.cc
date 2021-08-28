// Copyright (c) 2021, Paul Dreik
// For license information refer to format.h.
#include <fmt/chrono.h>

#include "fuzzer-common.h"

/*
 * a fuzzer for the chrono timepoints formatters
 * C is a clock (std::chrono::system_clock etc)
 */
template <typename C> void doit(const uint8_t* data, size_t size) {
  using D = typename C::duration;
  using TP = typename C::time_point;
  using Rep = typename TP::rep;
  constexpr auto N = sizeof(Rep);
  if (size < N) return;

  const auto x = assign_from_buf<Rep>(data);
  D dur{x};
  TP timepoint{dur};
  data += N;
  size -= N;
  data_to_string format_str(data, size);

  std::string message = fmt::format(format_str.get(), timepoint);
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size < 1) return 0;
  const auto action = data[0] & 0b1;
  data += 1;
  size -= 1;

  try {
    switch (action) {
    case 0:
      doit<std::chrono::system_clock>(data, size);
      break;
    case 1:
        // may be the same as system_clock
        doit<std::chrono::high_resolution_clock>(data, size);
      break;
    case 2:
      // won't compile
      // doit<std::chrono::steady_clock>(data,size);
      break;
    }
  } catch (...) {
  }
  return 0;
}
