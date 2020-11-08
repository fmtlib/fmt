#include <array>
#include <string>

#define FMT_ENABLE_COMPILE_TIME_FORMATTING 1
#include "fmt/format.h"
#include "gmock.h"
#include "gtest-extra.h"

template <size_t max_string_length> struct constexpr_buffer_helper {
  template <typename Func>
  constexpr constexpr_buffer_helper& modify(Func func) {
    func(buffer);
    return *this;
  }

  template <typename T> constexpr bool operator==(const T& rhs) const noexcept {
    return (std::string_view(rhs).compare(buffer.data()) == 0);
  }

  std::array<char, max_string_length> buffer{};
};
