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

#ifdef __cpp_lib_filesystem
TEST(std_test, path) {
  EXPECT_EQ(fmt::format("{:8}", std::filesystem::path("foo")), "\"foo\"   ");
  EXPECT_EQ(fmt::format("{}", std::filesystem::path("foo\"bar.txt")),
            "\"foo\\\"bar.txt\"");
  EXPECT_EQ(fmt::format("{:?}", std::filesystem::path("foo\"bar.txt")),
            "\"foo\\\"bar.txt\"");

#  ifdef _WIN32
  EXPECT_EQ(fmt::format("{}", std::filesystem::path(
                                  L"\x0428\x0447\x0443\x0447\x044B\x043D\x0448"
                                  L"\x0447\x044B\x043D\x0430")),
            "\"Шчучыншчына\"");
  EXPECT_EQ(fmt::format("{}", std::filesystem::path(L"\xd800")), "\"\\ud800\"");
#  endif
}

// Test ambiguity problem described in #2954.
TEST(ranges_std_test, format_vector_path) {
  auto p = std::filesystem::path("foo/bar.txt");
  auto c = std::vector<std::string>{"abc", "def"};
  EXPECT_EQ(fmt::format("path={}, range={}", p, c),
            "path=\"foo/bar.txt\", range=[\"abc\", \"def\"]");
}

// Test that path is not escaped twice in the debug mode.
TEST(ranges_std_test, format_quote_path) {
  auto vec =
      std::vector<std::filesystem::path>{"path1/file1.txt", "path2/file2.txt"};
  EXPECT_EQ(fmt::format("{}", vec),
            "[\"path1/file1.txt\", \"path2/file2.txt\"]");
#  ifdef __cpp_lib_optional
  auto o = std::optional<std::filesystem::path>("path/file.txt");
  EXPECT_EQ(fmt::format("{}", o), "optional(\"path/file.txt\")");
  EXPECT_EQ(fmt::format("{:?}", o), "optional(\"path/file.txt\")");
#  endif
}
#endif

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

struct throws_on_move {
  throws_on_move() = default;

  [[noreturn]] throws_on_move(throws_on_move&&) {
    throw std::runtime_error("Thrown by throws_on_move");
  }

  throws_on_move(const throws_on_move&) = default;
};

namespace fmt {
template <> struct formatter<throws_on_move> : formatter<string_view> {
  auto format(const throws_on_move&, format_context& ctx) const
      -> decltype(ctx.out()) {
    string_view str("<throws_on_move>");
    return formatter<string_view>::format(str, ctx);
  }
};
}  // namespace fmt

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

  std::variant<std::monostate, throws_on_move> v6;

  try {
    throws_on_move thrower;
    v6.emplace<throws_on_move>(std::move(thrower));
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
  // Tests that the inline namespace is stripped out, e.g.
  // std::filesystem::__cxx11::* -> std::filesystem::*.
  try {
    throw std::filesystem::filesystem_error("message", std::error_code());
  } catch (const std::filesystem::filesystem_error& ex) {
    EXPECT_THAT(fmt::format("{:t}", ex),
                StartsWith("std::filesystem::filesystem_error: "));
  }
#endif
}
