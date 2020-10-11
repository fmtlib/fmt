// Copyright (c) 2019, Paul Dreik
// For the license information refer to format.h.

#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include <fmt/chrono.h>

#include "fuzzer_common.h"

using fmt_fuzzer::nfixed;

template <typename Item>
void invoke_fmt(const uint8_t* data, size_t size) {
  constexpr auto N = sizeof(Item);
  static_assert(N <= nfixed, "Nfixed is too small");
  if (size <= nfixed) {
    return;
  }
  const Item item = fmt_fuzzer::assignFromBuf<Item>(data);
  data += nfixed;
  size -= nfixed;

#if FMT_FUZZ_SEPARATE_ALLOCATION
  std::vector<char> fmtstringbuffer(size);
  std::memcpy(fmtstringbuffer.data(), data, size);
  auto format_str = fmt::string_view(fmtstringbuffer.data(), size);
#else
  auto format_str = fmt::string_view(fmt_fuzzer::as_chars(data), size);
#endif

#if FMT_FUZZ_FORMAT_TO_STRING
  std::string message = fmt::format(format_str, item);
#else
  fmt::memory_buffer message;
  fmt::format_to(message, format_str, item);
#endif
}

void invoke_fmt_time(const uint8_t* data, size_t size) {
  using Item = std::time_t;
  static_assert(sizeof(Item) <= nfixed, "Nfixed too small");
  if (size <= nfixed) return;
  const Item item = fmt_fuzzer::assignFromBuf<Item>(data);
  data += nfixed;
  size -= nfixed;
#if FMT_FUZZ_SEPARATE_ALLOCATION
  std::vector<char> fmtstringbuffer(size);
  std::memcpy(fmtstringbuffer.data(), data, size);
  auto format_str = fmt::string_view(fmtstringbuffer.data(), size);
#else
  auto format_str = fmt::string_view(fmt_fuzzer::as_chars(data), size);
#endif
  auto* b = std::localtime(&item);
  if (b) {
#if FMT_FUZZ_FORMAT_TO_STRING
    std::string message = fmt::format(format_str, *b);
#else
    fmt::memory_buffer message;
    fmt::format_to(message, format_str, *b);
#endif
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size <= 3) return 0;

  const auto first = data[0];
  data++;
  size--;

  try {
    switch (first) {
    case 0:
      invoke_fmt<bool>(data, size);
      break;
    case 1:
      invoke_fmt<char>(data, size);
      break;
    case 2:
      invoke_fmt<unsigned char>(data, size);
      break;
    case 3:
      invoke_fmt<signed char>(data, size);
      break;
    case 4:
      invoke_fmt<short>(data, size);
      break;
    case 5:
      invoke_fmt<unsigned short>(data, size);
      break;
    case 6:
      invoke_fmt<int>(data, size);
      break;
    case 7:
      invoke_fmt<unsigned int>(data, size);
      break;
    case 8:
      invoke_fmt<long>(data, size);
      break;
    case 9:
      invoke_fmt<unsigned long>(data, size);
      break;
    case 10:
      invoke_fmt<float>(data, size);
      break;
    case 11:
      invoke_fmt<double>(data, size);
      break;
    case 12:
      invoke_fmt<long double>(data, size);
      break;
    case 13:
      invoke_fmt_time(data, size);
      break;
    default:
      break;
    }
  } catch (std::exception&) {
  }
  return 0;
}
