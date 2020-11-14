#include <array>
#include <string>

#include "fmt/compile.h"
#include "fmt/core.h"
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

TEST(CompileTimeFormattingTest, OneInteger) {
  constexpr auto result42 =
      constexpr_buffer_helper<3>{}.modify([](auto& buffer) {
        fmt::format_to(buffer.data(), FMT_COMPILE("{}"), 42);
      });
  EXPECT_EQ(result42, "42");
  constexpr auto result420 =
      constexpr_buffer_helper<3>{}.modify([](auto& buffer) {
        fmt::format_to(buffer.data(), FMT_COMPILE("{}"), 420);
      });
  EXPECT_EQ(result420, "420");
}

TEST(CompileTimeFormattingTest, TwoIntegers) {
  constexpr auto result = constexpr_buffer_helper<6>{}.modify([](auto& buffer) {
    fmt::format_to(buffer.data(), FMT_COMPILE("{} {}"), 41, 43);
  });
  EXPECT_EQ(result, "41 43");
}

TEST(CompileTimeFormattingTest, OneString) {
  constexpr auto result = constexpr_buffer_helper<3>{}.modify([](auto& buffer) {
    fmt::format_to(buffer.data(), FMT_COMPILE("{}"), "42");
  });
  EXPECT_EQ(result, "42");
}

TEST(CompileTimeFormattingTest, TwoStrings) {
  constexpr auto result =
      constexpr_buffer_helper<17>{}.modify([](auto& buffer) {
        fmt::format_to(buffer.data(), FMT_COMPILE("{} is {}"), "The answer",
                       "42");
      });
  EXPECT_EQ(result, "The answer is 42");
}

TEST(CompileTimeFormattingTest, StringAndInteger) {
  constexpr auto result =
      constexpr_buffer_helper<17>{}.modify([](auto& buffer) {
        fmt::format_to(buffer.data(), FMT_COMPILE("{} is {}"), "The answer",
                       42);
      });
  EXPECT_EQ(result, "The answer is 42");
}
