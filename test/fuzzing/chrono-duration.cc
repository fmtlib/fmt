// Copyright (c) 2019, Paul Dreik
// For the license information refer to format.h.

#include <cstdint>
#include <fmt/chrono.h>

#include "fuzzer-common.h"

template <typename Item, typename Ratio>
void invoke_inner(fmt::string_view format_str, Item item) {
  auto value = std::chrono::duration<Item, Ratio>(item);
  try {
#if FMT_FUZZ_FORMAT_TO_STRING
    std::string message = fmt::format(format_str, value);
#else
    fmt::memory_buffer buf;
    fmt::format_to(buf, format_str, value);
#endif
  } catch (std::exception&) {
  }
}

// Item is the underlying type for duration (int, long, etc.)
template <typename Item>
void invoke_outer(const uint8_t* data, size_t size, int scaling) {
  // Always use a fixed location of the data.
  using fmt_fuzzer::nfixed;

  static_assert(sizeof(Item) <= nfixed, "fixed size is too small");
  if (size <= nfixed + 1) return;

  const Item item = fmt_fuzzer::assignFromBuf<Item>(data);

  // Fast forward.
  data += nfixed;
  size -= nfixed;

  // data is already allocated separately in libFuzzer so reading past the end 
  // will most likely be detected anyway.
  const auto format_str = fmt::string_view(fmt_fuzzer::as_chars(data), size);

  // yocto, zepto, zetta and yotta are not handled.
  switch (scaling) {
  case 1:
    invoke_inner<Item, std::atto>(format_str, item);
    break;
  case 2:
    invoke_inner<Item, std::femto>(format_str, item);
    break;
  case 3:
    invoke_inner<Item, std::pico>(format_str, item);
    break;
  case 4:
    invoke_inner<Item, std::nano>(format_str, item);
    break;
  case 5:
    invoke_inner<Item, std::micro>(format_str, item);
    break;
  case 6:
    invoke_inner<Item, std::milli>(format_str, item);
    break;
  case 7:
    invoke_inner<Item, std::centi>(format_str, item);
    break;
  case 8:
    invoke_inner<Item, std::deci>(format_str, item);
    break;
  case 9:
    invoke_inner<Item, std::deca>(format_str, item);
    break;
  case 10:
    invoke_inner<Item, std::kilo>(format_str, item);
    break;
  case 11:
    invoke_inner<Item, std::mega>(format_str, item);
    break;
  case 12:
    invoke_inner<Item, std::giga>(format_str, item);
    break;
  case 13:
    invoke_inner<Item, std::tera>(format_str, item);
    break;
  case 14:
    invoke_inner<Item, std::peta>(format_str, item);
    break;
  case 15:
    invoke_inner<Item, std::exa>(format_str, item);
  }
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size <= 4)  return 0;

  const auto representation = data[0];
  const auto scaling = data[1];
  data += 2;
  size -= 2;

  switch (representation) {
  case 1:
    invoke_outer<char>(data, size, scaling);
    break;
  case 2:
    invoke_outer<signed char>(data, size, scaling);
    break;
  case 3:
    invoke_outer<unsigned char>(data, size, scaling);
    break;
  case 4:
    invoke_outer<short>(data, size, scaling);
    break;
  case 5:
    invoke_outer<unsigned short>(data, size, scaling);
    break;
  case 6:
    invoke_outer<int>(data, size, scaling);
    break;
  case 7:
    invoke_outer<unsigned int>(data, size, scaling);
    break;
  case 8:
    invoke_outer<long>(data, size, scaling);
    break;
  case 9:
    invoke_outer<unsigned long>(data, size, scaling);
    break;
  case 10:
    invoke_outer<float>(data, size, scaling);
    break;
  case 11:
    invoke_outer<double>(data, size, scaling);
    break;
  case 12:
    invoke_outer<long double>(data, size, scaling);
    break;
  default:
    break;
  }
  return 0;
}
