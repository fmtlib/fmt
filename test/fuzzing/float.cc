// A fuzzer for floating-point formatter.
// For the license information refer to format.h.

#include <cstdint>
#include <cstdlib>
#include <stdexcept>
#include <limits>
#include <fmt/format.h>

#include "fuzzer-common.h"

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size <= sizeof(double) || !std::numeric_limits<double>::is_iec559)
    return 0;

  auto value = assign_from_buf<double>(data);
  auto buffer = fmt::memory_buffer();
  fmt::format_to(buffer, "{}", value);

  // Check a round trip.
  if (std::isnan(value)) {
    auto nan = std::signbit(value) ? "-nan" : "nan";
    if (fmt::string_view(buffer.data(), buffer.size()) != nan)
      throw std::runtime_error("round trip failure");
    return 0;
  }
  buffer.push_back('\0');
  char* ptr = nullptr;
  if (std::strtod(buffer.data(), &ptr) != value)
    throw std::runtime_error("round trip failure");
  if (ptr + 1 != buffer.end())
    throw std::runtime_error("unparsed output");
  return 0;
}
