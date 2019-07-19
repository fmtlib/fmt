#include <format>
#include "gtest.h"

TEST(StdFormatTest, Escaping) {
  using namespace std;
  string s = format("{0}-{{", 8);  // s == "8-{"
  EXPECT_EQ(s, "8-{");
}

TEST(StdFormatTest, Indexing) {
  using namespace std;
  string s0 = format("{} to {}", "a", "b");    // OK: automatic indexing
  string s1 = format("{1} to {0}", "a", "b");  // OK: manual indexing
  EXPECT_EQ(s0, "a to b");
  EXPECT_EQ(s1, "b to a");
  // Error: mixing automatic and manual indexing
  EXPECT_THROW(string s2 = format("{0} to {}", "a", "b"), std::format_error);
  // Error: mixing automatic and manual indexing
  EXPECT_THROW(string s3 = format("{} to {1}", "a", "b"), std::format_error);
}

TEST(StdFormatTest, Alignment) {
  using namespace std;
  char c = 120;
  string s0 = format("{:6}", 42);     // s0 == "    42"
  string s1 = format("{:6}", 'x');    // s1 == "x     "
  string s2 = format("{:*<6}", 'x');  // s2 == "x*****"
  string s3 = format("{:*>6}", 'x');  // s3 == "*****x"
  string s4 = format("{:*^6}", 'x');  // s4 == "**x***"
  // Error: '=' with charT and no integer presentation type
  EXPECT_THROW(string s5 = format("{:=6}", 'x'), std::format_error);
  string s6 = format("{:6d}", c);       // s6 == "   120"
  string s7 = format("{:=+06d}", c);    // s7 == "+00120"
  string s8 = format("{:0=#6x}", 0xa);  // s8 == "0x000a"
  string s9 = format("{:6}", true);     // s9 == "true  "
  EXPECT_EQ(s0, "    42");
  EXPECT_EQ(s1, "x     ");
  EXPECT_EQ(s2, "x*****");
  EXPECT_EQ(s3, "*****x");
  EXPECT_EQ(s4, "**x***");
  EXPECT_EQ(s6, "   120");
  EXPECT_EQ(s7, "+00120");
  EXPECT_EQ(s8, "0x000a");
  EXPECT_EQ(s9, "true  ");
}

TEST(StdFormatTest, Float) {
  using namespace std;
  double inf = numeric_limits<double>::infinity();
  double nan = numeric_limits<double>::quiet_NaN();
  string s0 = format("{0:} {0:+} {0:-} {0: }", 1);   // s0 == "1 +1 1  1"
  string s1 = format("{0:} {0:+} {0:-} {0: }", -1);  // s1 == "-1 -1 -1 -1"
  string s2 =
      format("{0:} {0:+} {0:-} {0: }", inf);  // s2 == "inf +inf inf  inf"
  string s3 =
      format("{0:} {0:+} {0:-} {0: }", nan);  // s3 == "nan +nan nan  nan"
  EXPECT_EQ(s0, "1 +1 1  1");
  EXPECT_EQ(s1, "-1 -1 -1 -1");
  EXPECT_EQ(s2, "inf +inf inf  inf");
  EXPECT_EQ(s3, "nan +nan nan  nan");
}

TEST(StdFormatTest, Int) {
  using namespace std;
  string s0 = format("{}", 42);                       // s0 == "42"
  string s1 = format("{0:b} {0:d} {0:o} {0:x}", 42);  // s1 == "101010 42 52 2a"
  string s2 = format("{0:#x} {0:#X}", 42);            // s2 == "0x2a 0X2A"
  string s3 = format("{:n}", 1234);  // s3 == "1,234" (depends on the locale)
  EXPECT_EQ(s0, "42");
  EXPECT_EQ(s1, "101010 42 52 2a");
  EXPECT_EQ(s2, "0x2a 0X2A");
  EXPECT_EQ(s3, "1,234");
}

#include <format>

enum color { red, green, blue };

const char* color_names[] = {"red", "green", "blue"};

template <> struct std::formatter<color> : std::formatter<const char*> {
  auto format(color c, format_context& ctx) {
    return formatter<const char*>::format(color_names[c], ctx);
  }
};

struct err {};

TEST(StdFormatTest, Formatter) {
  std::string s0 = std::format("{}", 42);  // OK: library-provided formatter
  // std::string s1 = std::format("{}", L"foo"); // Ill-formed: disabled
  // formatter
  std::string s2 = std::format("{}", red);  // OK: user-provided formatter
  // std::string s3 = std::format("{}", err{});  // Ill-formed: disabled
  // formatter
  EXPECT_EQ(s0, "42");
  EXPECT_EQ(s2, "red");
}

struct S {
  int value;
};

template <> struct std::formatter<S> {
  size_t width_arg_id = 0;

  // Parses a width argument id in the format { <digit> }.
  constexpr auto parse(format_parse_context& ctx) {
    auto iter = ctx.begin();
    // auto get_char = [&]() { return iter != ctx.end() ? *iter : 0; };
    auto get_char = [&]() { return iter != ctx.end() ? *iter : '\0'; };
    if (get_char() != '{') return iter;
    ++iter;
    char c = get_char();
    if (!isdigit(c) || (++iter, get_char()) != '}')
      throw format_error("invalid format");
    width_arg_id = c - '0';
    ctx.check_arg_id(width_arg_id);
    return ++iter;
  }

  // Formats S with width given by the argument width_arg_id.
  auto format(S s, format_context& ctx) {
    int width = visit_format_arg(
        [](auto value) -> int {
          if constexpr (!is_integral_v<decltype(value)>)
            throw format_error("width is not integral");
          // else if (value < 0 || value > numeric_limits<int>::max())
          else if (fmt::internal::is_negative(value) ||
                   value > numeric_limits<int>::max())
            throw format_error("invalid width");
          else
            return static_cast<int>(value);
        },
        ctx.arg(width_arg_id));
    return format_to(ctx.out(), "{0:{1}}", s.value, width);
  }
};

TEST(StdFormatTest, Parsing) {
  std::string s = std::format("{0:{1}}", S{42}, 10);  // s == "        42"
  EXPECT_EQ(s, "        42");
}

#if FMT_USE_INT128
template <> struct std::formatter<__int128_t> : std::formatter<long long> {
  auto format(__int128_t n, format_context& ctx) {
    // Format as a long long since we only want to check if it is possible to
    // specialize formatter for __int128_t.
    return formatter<long long>::format(n, ctx);
  }
};

TEST(StdFormatTest, Int128) {
  __int128_t n = 42;
  auto s = std::format("{}", n);
  EXPECT_EQ(s, "42");
}
#endif  // FMT_USE_INT128
