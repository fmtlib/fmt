// Formatting library for C++ - tests of formatters for standard library types
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/std.h"

#include <stdexcept>
#include <string>
#include <vector>

#include "fmt/os.h"  // fmt::system_category
#include "fmt/ranges.h"
#include "gtest-extra.h"  // StartsWith

using testing::StartsWith;

TEST(std_test, path) {
#ifdef __cpp_lib_filesystem
  EXPECT_EQ(fmt::format("{:8}", std::filesystem::path("foo")), "\"foo\"   ");
  EXPECT_EQ(fmt::format("{}", std::filesystem::path("foo\"bar.txt")),
            "\"foo\\\"bar.txt\"");
  EXPECT_EQ(fmt::format("{:?}", std::filesystem::path("foo\"bar.txt")),
            "\"foo\\\"bar.txt\"");

#  ifdef _WIN32
  // File.txt in Russian.
  const wchar_t unicode_path[] = {0x424, 0x430, 0x439, 0x43b, 0x2e,
                                  0x74,  0x78,  0x74,  0};
  const char unicode_u8path[] = {'"',        char(0xd0), char(0xa4), char(0xd0),
                                 char(0xb0), char(0xd0), char(0xb9), char(0xd0),
                                 char(0xbb), '.',        't',        'x',
                                 't',        '"',        '\0'};
  EXPECT_EQ(fmt::format("{}", std::filesystem::path(unicode_path)),
            unicode_u8path);
#  endif
#endif
}

TEST(ranges_std_test, format_vector_path) {
// Test ambiguity problem described in #2954.
#ifdef __cpp_lib_filesystem
  auto p = std::filesystem::path("foo/bar.txt");
  auto c = std::vector<std::string>{"abc", "def"};
  EXPECT_EQ(fmt::format("path={}, range={}", p, c),
            "path=\"foo/bar.txt\", range=[\"abc\", \"def\"]");
#endif
}

TEST(ranges_std_test, format_quote_path) {
  // Test that path is not escaped twice in the debug mode.
#ifdef __cpp_lib_filesystem
  auto vec =
      std::vector<std::filesystem::path>{"path1/file1.txt", "path2/file2.txt"};
  EXPECT_EQ(fmt::format("{}", vec),
            "[\"path1/file1.txt\", \"path2/file2.txt\"]");
#  ifdef __cpp_lib_optional
  auto o = std::optional<std::filesystem::path>("path/file.txt");
  EXPECT_EQ(fmt::format("{}", o), "optional(\"path/file.txt\")");
  EXPECT_EQ(fmt::format("{:?}", o), "optional(\"path/file.txt\")");
#  endif
#endif
}

TEST(std_test, thread_id) {
  EXPECT_FALSE(fmt::format("{}", std::this_thread::get_id()).empty());
}

TEST(std_test, optional) {
#ifdef __cpp_lib_optional
  EXPECT_EQ(fmt::format("{}", std::optional<int>{}), "none");
  EXPECT_EQ(fmt::format("{}", std::pair{1, "second"}), "(1, \"second\")");
  EXPECT_EQ(fmt::format("{}", std::vector{std::optional{1}, std::optional{2},
                                          std::optional{3}}),
            "[optional(1), optional(2), optional(3)]");
  EXPECT_EQ(
      fmt::format("{}", std::optional<std::optional<const char*>>{{"nested"}}),
      "optional(optional(\"nested\"))");
  EXPECT_EQ(
      fmt::format("{:<{}}", std::optional{std::string{"left aligned"}}, 30),
      "optional(\"left aligned\"                )");
  EXPECT_EQ(
      fmt::format("{::d}", std::optional{std::vector{'h', 'e', 'l', 'l', 'o'}}),
      "optional([104, 101, 108, 108, 111])");
  EXPECT_EQ(fmt::format("{}", std::optional{std::string{"string"}}),
            "optional(\"string\")");
  EXPECT_EQ(fmt::format("{}", std::optional{'C'}), "optional(\'C\')");
  EXPECT_EQ(fmt::format("{:.{}f}", std::optional{3.14}, 1), "optional(3.1)");

  struct unformattable {};
  EXPECT_FALSE((fmt::is_formattable<unformattable>::value));
  EXPECT_FALSE((fmt::is_formattable<std::optional<unformattable>>::value));
  EXPECT_TRUE((fmt::is_formattable<std::optional<int>>::value));
#endif
}

// this struct exists only to force a variant to be
// valueless by exception
// NOLINTNEXTLINE
struct throws_on_move_t {
  throws_on_move_t() = default;

  // NOLINTNEXTLINE
  [[noreturn]] throws_on_move_t(throws_on_move_t&&) {
    throw std::runtime_error{"Thrown by throws_on_move_t"};
  }

  throws_on_move_t(const throws_on_move_t&) = default;
};

template <> struct fmt::formatter<throws_on_move_t> : formatter<string_view> {
  template <typename FormatContext>
  auto format(const throws_on_move_t&, FormatContext& ctx) const {
    string_view str{"<throws_on_move_t>"};
    return formatter<string_view>::format(str, ctx);
  }
};

TEST(std_test, variant) {
#ifdef __cpp_lib_variant
  EXPECT_EQ(fmt::format("{}", std::monostate{}), "monostate");
  using V0 = std::variant<int, float, std::string, char>;
  V0 v0(42);
  V0 v1(1.5f);
  V0 v2("hello");
  V0 v3('i');
  EXPECT_EQ(fmt::format("{}", v0), "variant(42)");
  EXPECT_EQ(fmt::format("{}", v1), "variant(1.5)");
  EXPECT_EQ(fmt::format("{}", v2), "variant(\"hello\")");
  EXPECT_EQ(fmt::format("{}", v3), "variant('i')");

  struct unformattable {};
  EXPECT_FALSE((fmt::is_formattable<unformattable>::value));
  EXPECT_FALSE((fmt::is_formattable<std::variant<unformattable>>::value));
  EXPECT_FALSE((fmt::is_formattable<std::variant<unformattable, int>>::value));
  EXPECT_FALSE((fmt::is_formattable<std::variant<int, unformattable>>::value));
  EXPECT_FALSE(
      (fmt::is_formattable<std::variant<unformattable, unformattable>>::value));
  EXPECT_TRUE((fmt::is_formattable<std::variant<int, float>>::value));

  using V1 = std::variant<std::monostate, std::string, std::string>;
  V1 v4{};
  V1 v5{std::in_place_index<1>, "yes, this is variant"};

  EXPECT_EQ(fmt::format("{}", v4), "variant(monostate)");
  EXPECT_EQ(fmt::format("{}", v5), "variant(\"yes, this is variant\")");

  volatile int i = 42;  // Test compile error before GCC 11 described in #3068.
  EXPECT_EQ(fmt::format("{}", i), "42");

  using V2 = std::variant<std::monostate, throws_on_move_t>;

  V2 v6{};

  try {
    throws_on_move_t thrower{};
    v6.emplace<throws_on_move_t>(std::move(thrower));
  } catch (const std::runtime_error&) {
  }
  // v6 is now valueless by exception

  EXPECT_EQ(fmt::format("{}", v6), "variant(valueless by exception)");

#endif
}

TEST(std_test, error_code) {
  EXPECT_EQ("generic:42",
            fmt::format(FMT_STRING("{0}"),
                        std::error_code(42, std::generic_category())));
  EXPECT_EQ("system:42",
            fmt::format(FMT_STRING("{0}"),
                        std::error_code(42, fmt::system_category())));
  EXPECT_EQ("system:-42",
            fmt::format(FMT_STRING("{0}"),
                        std::error_code(-42, fmt::system_category())));
}

template <typename Catch> void exception_test() {
  try {
    throw std::runtime_error("Test Exception");
  } catch (const Catch& ex) {
    EXPECT_EQ("Test Exception", fmt::format("{}", ex));
    EXPECT_EQ("std::runtime_error: Test Exception", fmt::format("{:t}", ex));
  }
}

namespace my_ns1 {
namespace my_ns2 {
struct my_exception : public std::exception {
 private:
  std::string msg;

 public:
  my_exception(const std::string& s) : msg(s) {}
  const char* what() const noexcept override;
};
const char* my_exception::what() const noexcept { return msg.c_str(); }
}  // namespace my_ns2
}  // namespace my_ns1

TEST(std_test, exception) {
  exception_test<std::exception>();
  exception_test<std::runtime_error>();

  try {
    using namespace my_ns1::my_ns2;
    throw my_exception("My Exception");
  } catch (const std::exception& ex) {
    EXPECT_EQ("my_ns1::my_ns2::my_exception: My Exception",
              fmt::format("{:t}", ex));
    EXPECT_EQ("My Exception", fmt::format("{:}", ex));
  }

  try {
    throw std::system_error(std::error_code(), "message");
  } catch (const std::system_error& ex) {
    EXPECT_THAT(fmt::format("{:t}", ex), StartsWith("std::system_error: "));
  }

#ifdef __cpp_lib_filesystem
  try {
    throw std::filesystem::filesystem_error("message", std::error_code());
  } catch (const std::filesystem::filesystem_error& ex) {
    EXPECT_THAT(fmt::format("{:t}", ex),
                StartsWith("std::filesystem::filesystem_error: "));
  }
#endif
}
