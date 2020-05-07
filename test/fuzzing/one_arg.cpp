// Copyright (c) 2019, Paul Dreik
// License: see LICENSE.rst in the fmt root directory

#include <fmt/core.h>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <fmt/chrono.h>
#include "fuzzer_common.h"

using fmt_fuzzer::Nfixed;

template <typename Item>
void invoke_fmt(const uint8_t* Data, size_t Size) {
  constexpr auto N = sizeof(Item);
  static_assert(N <= Nfixed, "Nfixed is too small");
  if (Size <= Nfixed) {
    return;
  }
  const Item item = fmt_fuzzer::assignFromBuf<Item>(Data);
  Data += Nfixed;
  Size -= Nfixed;

#if FMT_FUZZ_SEPARATE_ALLOCATION
  // allocates as tight as possible, making it easier to catch buffer overruns.
  std::vector<char> fmtstringbuffer(Size);
  std::memcpy(fmtstringbuffer.data(), Data, Size);
  auto fmtstring = fmt::string_view(fmtstringbuffer.data(), Size);
#else
  auto fmtstring = fmt::string_view(fmt_fuzzer::as_chars(Data), Size);
#endif

#if FMT_FUZZ_FORMAT_TO_STRING
  std::string message = fmt::format(fmtstring, item);
#else
  fmt::memory_buffer message;
  fmt::format_to(message, fmtstring, item);
#endif
}

void invoke_fmt_time(const uint8_t* Data, size_t Size) {
  using Item = std::time_t;
  constexpr auto N = sizeof(Item);
  static_assert(N <= Nfixed, "Nfixed too small");
  if (Size <= Nfixed) {
    return;
  }
  const Item item = fmt_fuzzer::assignFromBuf<Item>(Data);
  Data += Nfixed;
  Size -= Nfixed;
#if FMT_FUZZ_SEPARATE_ALLOCATION
  // allocates as tight as possible, making it easier to catch buffer overruns.
  std::vector<char> fmtstringbuffer(Size);
  std::memcpy(fmtstringbuffer.data(), Data, Size);
  auto fmtstring = fmt::string_view(fmtstringbuffer.data(), Size);
#else
  auto fmtstring = fmt::string_view(fmt_fuzzer::as_chars(Data), Size);
#endif
  auto* b = std::localtime(&item);
  if (b) {
#if FMT_FUZZ_FORMAT_TO_STRING
    std::string message = fmt::format(fmtstring, *b);
#else
    fmt::memory_buffer message;
    fmt::format_to(message, fmtstring, *b);
#endif
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
  if (Size <= 3) {
    return 0;
  }

  const auto first = Data[0];
  Data++;
  Size--;

  try {
    switch (first) {
    case 0:
      invoke_fmt<bool>(Data, Size);
      break;
    case 1:
      invoke_fmt<char>(Data, Size);
      break;
    case 2:
      invoke_fmt<unsigned char>(Data, Size);
      break;
    case 3:
      invoke_fmt<signed char>(Data, Size);
      break;
    case 4:
      invoke_fmt<short>(Data, Size);
      break;
    case 5:
      invoke_fmt<unsigned short>(Data, Size);
      break;
    case 6:
      invoke_fmt<int>(Data, Size);
      break;
    case 7:
      invoke_fmt<unsigned int>(Data, Size);
      break;
    case 8:
      invoke_fmt<long>(Data, Size);
      break;
    case 9:
      invoke_fmt<unsigned long>(Data, Size);
      break;
    case 10:
      invoke_fmt<float>(Data, Size);
      break;
    case 11:
      invoke_fmt<double>(Data, Size);
      break;
    case 12:
      invoke_fmt<long double>(Data, Size);
      break;
    case 13:
      invoke_fmt_time(Data, Size);
      break;
    default:
      break;
    }
  } catch (std::exception& /*e*/) {
  }
  return 0;
}
