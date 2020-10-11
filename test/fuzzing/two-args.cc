// Copyright (c) 2019, Paul Dreik
// For the license information refer to format.h.

#include <cstdint>
#include <exception>
#include <string>
#include <fmt/format.h>

#include "fuzzer-common.h"

template <typename Item1, typename Item2>
void invoke_fmt(const uint8_t* data, size_t size) {
  using fmt_fuzzer::nfixed;
  static_assert(sizeof(Item1) <= nfixed, "size1 exceeded");
  static_assert(sizeof(Item2) <= nfixed, "size2 exceeded");
  if (size <= nfixed + nfixed) return;

  const Item1 item1 = fmt_fuzzer::assignFromBuf<Item1>(data);
  data += nfixed;
  size -= nfixed;

  const Item2 item2 = fmt_fuzzer::assignFromBuf<Item2>(data);
  data += nfixed;
  size -= nfixed;

  auto format_str = fmt::string_view(fmt_fuzzer::as_chars(data), size);

#if FMT_FUZZ_FORMAT_TO_STRING
  std::string message = fmt::format(format_str, item1, item2);
#else
  fmt::memory_buffer message;
  fmt::format_to(message, format_str, item1, item2);
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
  case 13:
    using ptr = void*;
    callback(ptr());
    break;
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size <= 3) return 0;

  // Switch types depending on the first byte of the input.
  const auto first = data[0] & 0x0F;
  const auto second = (data[0] & 0xF0) >> 4;
  data++;
  size--;
  try {
    invoke(first, [=](auto param1) {
      invoke(second, [=](auto param2) {
        invoke_fmt<decltype(param1), decltype(param2)>(data, size);
      });
    });
  } catch (std::exception&) {
  }
  return 0;
}
