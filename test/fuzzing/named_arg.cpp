// Copyright (c) 2019, Paul Dreik
// License: see LICENSE.rst in the fmt root directory

#include <fmt/chrono.h>
#include <fmt/core.h>
#include <cstdint>
#include <stdexcept>
#include <type_traits>
#include <vector>
#include "fuzzer_common.h"

template <typename Item1>
void invoke_fmt(const uint8_t* Data, size_t Size, unsigned int argsize) {
  constexpr auto N1 = sizeof(Item1);
  static_assert(N1 <= fmt_fuzzer::Nfixed, "Nfixed too small");
  if (Size <= fmt_fuzzer::Nfixed) {
    return;
  }
  const Item1 item1 = fmt_fuzzer::assignFromBuf<Item1>(Data);

  Data += fmt_fuzzer::Nfixed;
  Size -= fmt_fuzzer::Nfixed;

  // how many chars should be used for the argument name?
  if (argsize <= 0 || argsize >= Size) {
    return;
  }

  // allocating buffers separately is slower, but increases chances
  // of detecting memory errors
#if FMT_FUZZ_SEPARATE_ALLOCATION
  std::vector<char> argnamebuffer(argsize + 1);
  std::memcpy(argnamebuffer.data(), Data, argsize);
  auto argname = argnamebuffer.data();
#else
  auto argname = fmt_fuzzer::as_chars(Data);
#endif
  Data += argsize;
  Size -= argsize;

#if FMT_FUZZ_SEPARATE_ALLOCATION
  // allocates as tight as possible, making it easier to catch buffer overruns.
  std::vector<char> fmtstringbuffer(Size);
  std::memcpy(fmtstringbuffer.data(), Data, Size);
  auto fmtstring = fmt::string_view(fmtstringbuffer.data(), Size);
#else
  auto fmtstring = fmt::string_view(fmt_fuzzer::as_chars(Data), Size);
#endif

#if FMT_FUZZ_FORMAT_TO_STRING
  std::string message = fmt::format(fmtstring, fmt::arg(argname, item1));
#else
  fmt::memory_buffer outbuf;
  fmt::format_to(outbuf, fmtstring, fmt::arg(argname, item1));
#endif
}

// for dynamic dispatching to an explicit instantiation
template <typename Callback> void invoke(int index, Callback callback) {
  switch (index) {
  case 0:
    callback(bool{});
    break;
  case 1:
    callback(char{});
    break;
  case 2:
    using sc = signed char;
    callback(sc{});
    break;
  case 3:
    using uc = unsigned char;
    callback(uc{});
    break;
  case 4:
    callback(short{});
    break;
  case 5:
    using us = unsigned short;
    callback(us{});
    break;
  case 6:
    callback(int{});
    break;
  case 7:
    callback(unsigned{});
    break;
  case 8:
    callback(long{});
    break;
  case 9:
    using ul = unsigned long;
    callback(ul{});
    break;
  case 10:
    callback(float{});
    break;
  case 11:
    callback(double{});
    break;
  case 12:
    using LD = long double;
    callback(LD{});
    break;
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* Data, size_t Size) {
  if (Size <= 3) {
    return 0;
  }

  // switch types depending on the first byte of the input
  const auto first = Data[0] & 0x0F;
  const unsigned int second = (Data[0] & 0xF0) >> 4;
  Data++;
  Size--;

  auto outerfcn = [=](auto param1) {
    invoke_fmt<decltype(param1)>(Data, Size, second);
  };

  try {
    invoke(first, outerfcn);
  } catch (std::exception& /*e*/) {
  }
  return 0;
}
