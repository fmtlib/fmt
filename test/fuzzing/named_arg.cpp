// Copyright (c) 2019, Paul Dreik
// For the license information refer to format.h.

#include <cstdint>
#include <type_traits>
#include <vector>
#include <fmt/chrono.h>

#include "fuzzer_common.h"

template <typename Item1>
void invoke_fmt(const uint8_t* data, size_t size, unsigned int argsize) {
  static_assert(sizeof(Item1) <= fmt_fuzzer::nfixed, "nfixed too small");
  if (size <= fmt_fuzzer::nfixed) return;
  const Item1 item1 = fmt_fuzzer::assignFromBuf<Item1>(data);

  data += fmt_fuzzer::nfixed;
  size -= fmt_fuzzer::nfixed;

  // How many chars should be used for the argument name?
  if (argsize <= 0 || argsize >= size) return;

#if FMT_FUZZ_SEPARATE_ALLOCATION
  std::vector<char> argnamebuffer(argsize + 1);
  std::memcpy(argnamebuffer.data(), data, argsize);
  auto argname = argnamebuffer.data();
#else
  auto argname = fmt_fuzzer::as_chars(data);
#endif
  data += argsize;
  size -= argsize;

#if FMT_FUZZ_SEPARATE_ALLOCATION
  std::vector<char> fmtstringbuffer(size);
  std::memcpy(fmtstringbuffer.data(), data, size);
  auto format_str = fmt::string_view(fmtstringbuffer.data(), size);
#else
  auto format_str = fmt::string_view(fmt_fuzzer::as_chars(data), size);
#endif

#if FMT_FUZZ_FORMAT_TO_STRING
  std::string message = fmt::format(format_str, fmt::arg(argname, item1));
#else
  fmt::memory_buffer out;
  fmt::format_to(out, format_str, fmt::arg(argname, item1));
#endif
}

// For dynamic dispatching to an explicit instantiation.
template <typename Callback> void invoke(int index, Callback callback) {
  switch (index) {
  case 0:
    callback(bool());
    break;
  case 1:
    callback(char());
    break;
  case 2:
    using sc = signed char;
    callback(sc());
    break;
  case 3:
    using uc = unsigned char;
    callback(uc());
    break;
  case 4:
    callback(short());
    break;
  case 5:
    using us = unsigned short;
    callback(us());
    break;
  case 6:
    callback(int());
    break;
  case 7:
    callback(unsigned());
    break;
  case 8:
    callback(long());
    break;
  case 9:
    using ul = unsigned long;
    callback(ul());
    break;
  case 10:
    callback(float());
    break;
  case 11:
    callback(double());
    break;
  case 12:
    using LD = long double;
    callback(LD());
    break;
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size <= 3) return 0;

  // Switch types depending on the first byte of the input.
  const auto first = data[0] & 0x0F;
  const unsigned second = (data[0] & 0xF0) >> 4;
  data++;
  size--;

  try {
    invoke(first, [=](auto param1) {
      invoke_fmt<decltype(param1)>(data, size, second);
    });
  } catch (std::exception&) {
  }
  return 0;
}
