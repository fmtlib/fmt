#include "fmt/backslash.h"
#include "gtest-extra.h"

namespace {
#if FMT_USE_CONSTEXPR
template <typename Char, size_t N, size_t M>
constexpr auto check_constexpr_iterators(const Char (&input)[N],
                                       const Char (&expected)[M]) -> bool {
  Char encoded[128] = {};
  const auto end = fmt::backslash_escape_to(
      encoded, fmt::basic_string_view<Char>{input, N - 1});
  if (end != encoded + M - 1) return false;
  for (size_t i = 0; i < M - 1; ++i)
    if (encoded[i] != expected[i]) return false;
  Char decoded[128] = {};
  const auto decoded_end = fmt::backslash_unescape_to(
      decoded, fmt::basic_string_view<Char>{expected, M - 1});
  if (decoded_end != decoded + N - 1) return false;
  for (size_t i = 0; i < N - 1; ++i)
    if (decoded[i] != input[i]) return false;
  return true;
}

static_assert(check_constexpr_iterators("\0\n\x1b\x7f\\\"", "\\x00\\n\\e\\x7f\\\\\\\""),
              "constexpr narrow escaping and unescaping");
static_assert(check_constexpr_iterators(L"\0\n\x1b\x7f\\\"", L"\\x00\\n\\e\\x7f\\\\\\\""),
              "constexpr wide escaping and unescaping");

constexpr auto check_constexpr_decoding() -> bool {
  char output[64] = {};
  const char input[] = "\\123\\x1b\\\'\\?";
  const auto end = fmt::backslash_unescape_to(
      output, fmt::string_view{input, sizeof(input) - 1});
  const char expected[] = "S\x1b\'?";
  if (end != output + sizeof(expected) - 1) return false;
  for (size_t i = 0; i < sizeof(expected) - 1; ++i)
    if (output[i] != expected[i]) return false;
  char empty[1] = {};
  return fmt::backslash_escape_to(empty, fmt::string_view{"", 0}) == empty &&
         fmt::backslash_unescape_to(empty, fmt::string_view{"", 0}) == empty;
}
static_assert(check_constexpr_decoding(),
              "constexpr octal, hex, quotes, question mark, and empty input");
#endif

#if FMT_USE_CONSTEXPR_STRING
constexpr auto check_constexpr_strings() -> bool {
  const auto encoded = fmt::backslash_escape("\x1b\n\x7f");
  const auto wide = fmt::backslash_escape(L"\x1b\n\x7f");
  if (encoded != "\\e\\n\\x7f" || wide != L"\\e\\n\\x7f") return false;
  if (fmt::backslash_unescape(encoded) != "\x1b\n\x7f" ||
      fmt::backslash_unescape(wide) != L"\x1b\n\x7f") return false;
  // Exercise allocation beyond small-string storage during constant evaluation.
  const auto input = std::string(100, '\x1b');
  const auto wide_input = std::wstring(100, L'\x1b');
  return fmt::backslash_unescape(fmt::backslash_escape(input)) == input &&
         fmt::backslash_unescape(fmt::backslash_escape(wide_input)) == wide_input;
}
static_assert(check_constexpr_strings(), "constexpr owning narrow/wide strings");
#endif

template <typename Char>
auto text(const char* s) -> std::basic_string<Char> {
  auto result = std::basic_string<Char>{};
  while (*s) result.push_back(static_cast<Char>(*s++));
  return result;
}

template <typename Char> void check_examples() {
  struct example { const char* escaped; const char* decoded; };
  const example cases[] = {
      {"\\a\\b\\f\\n\\r\\t\\v", "\a\b\f\n\r\t\v"},
      {"\\\\\\\"", "\\\""}, {"\\x1b[31m", "\x1b[31m"},
      {"\\e[31m", "\x1b[31m"}, {"\\ef", "\x1b" "f"},
      {"\\x7f", "\x7f"}, {"\\x1Bf", "\x1b" "f"},
      {"\\1Z", "\1Z"}, {"\\12Z", "\12Z"}, {"\\1234", "S4"},
      {"\\377", "\377"}, {"\\\\n", "\\n"},
      {"\\x5cn", "\\n"}, {"\"quoted\"", "\"quoted\""},
      {"\\'\\?", "'?"}};
  for (const auto& item : cases) {
    const auto input = text<Char>(item.escaped);
    auto expected = text<Char>(item.decoded);
    // Widen octal 377 as a code unit, rather than sign-extending a char.
    if (input == text<Char>("\\377")) expected.assign(1, Char(255));
    EXPECT_EQ(fmt::backslash_unescape<Char>({input.data(), input.size()}),
              expected);
  }
  for (const auto spelling : {"\\0", "\\00", "\\000", "\\x00"}) {
    const auto input = text<Char>(spelling);
    EXPECT_EQ(fmt::backslash_unescape<Char>({input.data(), input.size()}),
              std::basic_string<Char>(1, Char{}));
  }
  const auto controls = text<Char>("\a\b\f\n\r\t\v\x1b\x7f\\\"");
  EXPECT_EQ(fmt::backslash_escape<Char>({controls.data(), controls.size()}),
            text<Char>("\\a\\b\\f\\n\\r\\t\\v\\e\\x7f\\\\\\\""));
}
}  // namespace

TEST(backslash_test, examples) {
  check_examples<char>();
  check_examples<wchar_t>();
}

#if FMT_USE_EXCEPTIONS
namespace {
template <typename Char> void check_invalid_escapes() {
  const char* malformed[] = {
      "\\", "\\q", "\\x", "\\x1", "\\xG1", "\\x1G", "\\x\\n",
      "\\400", "\\777", "\\8", "\\9", "\\u001b", "\\U0000001b",
      "\\x{1b}", "\\o{33}", "\\\n"};
  for (auto spelling : malformed) {
    const auto input = text<Char>(spelling);
    EXPECT_THROW((void)fmt::backslash_unescape(input), fmt::format_error);
    auto output = text<Char>("prefix:");
    const auto prefixed = text<Char>("ok\\n") + input;
    EXPECT_THROW(fmt::backslash_unescape_to(std::back_inserter(output), prefixed),
                 fmt::format_error);
    EXPECT_EQ(output, text<Char>("prefix:ok\n"));
    Char buffer[8] = {};
    EXPECT_THROW(fmt::backslash_unescape_to(buffer, input), fmt::format_error);
    EXPECT_EQ(buffer[0], Char{}); // The invalid escape writes nothing.
  }
  const auto nul_escape = std::basic_string<Char>{Char('\\'), Char{}};
  EXPECT_THROW((void)fmt::backslash_unescape(nul_escape), fmt::format_error);
  const auto nul_suffix = text<Char>("\\0") + Char('8');
  EXPECT_EQ(fmt::backslash_unescape(nul_suffix),
            (std::basic_string<Char>{Char{}, Char('8')}));
}
}  // namespace

TEST(backslash_test, invalid_escapes) {
  check_invalid_escapes<char>();
  check_invalid_escapes<wchar_t>();
}
#endif

TEST(backslash_test, all_bytes) {
  auto bytes = std::string{};
  for (int i = 0; i < 256; ++i) bytes.push_back(static_cast<char>(i));
  const auto escaped = fmt::backslash_escape(bytes);
  EXPECT_EQ(fmt::backslash_unescape(escaped), bytes);
  for (auto c : escaped) {
    const auto value = static_cast<unsigned char>(c);
    EXPECT_FALSE(value < 32 || value == 127);
  }
  // High bytes are retained, so UTF-8 is not interpreted as C1 controls.
  const auto utf8 = std::string{"caf\xc3\xa9 \xe7\x95\x8c \xf0\x9f\x98\x80"};
  EXPECT_EQ(fmt::backslash_escape(utf8), utf8);
  auto wide = std::wstring{};
  for (int i = 0; i < 256; ++i) wide.push_back(static_cast<wchar_t>(i));
  wide += L"\u754c\U0001f600";
  EXPECT_EQ(fmt::backslash_unescape(fmt::backslash_escape(wide)), wide);
}

TEST(backslash_test, overloads_and_ownership) {
  EXPECT_EQ(fmt::backslash_escape(""), "");
  EXPECT_EQ(fmt::backslash_unescape(L""), L"");
  EXPECT_EQ(fmt::backslash_escape("\n"), "\\n");
  EXPECT_EQ(fmt::backslash_escape("'?"), "'?");
  EXPECT_EQ(fmt::backslash_unescape(L"\\t"), L"\t");
  auto input = std::string{"a\0b", 3};
  auto escaped = fmt::backslash_escape(input);
  EXPECT_EQ(escaped, "a\\x00b");
  const auto decoded = fmt::backslash_unescape(escaped);
  input.assign(100, 'x');
  escaped.assign(100, 'x');
  EXPECT_EQ(decoded, (std::string{"a\0b", 3}));
  const auto wide = std::wstring{L"a\0b", 3};
  EXPECT_EQ(fmt::backslash_unescape(wide), wide);
}

TEST(backslash_test, output_iterators) {
  auto appended = std::string{"prefix:"};
  auto out = fmt::backslash_escape_to(std::back_inserter(appended), "\x1b\n");
  *out++ = '!';
  EXPECT_EQ(appended, "prefix:\\e\\n!");
  const auto escaped = std::string{"\\e\\x00\\377\\\'\\?"};
  fmt::backslash_unescape_to(std::back_inserter(appended), escaped);
  // Check the embedded NUL explicitly rather than treating output as a C string.
  const auto prefix = std::string{"prefix:\\e\\n!"};
  EXPECT_EQ(appended.substr(0, prefix.size()), prefix);
  EXPECT_EQ(appended.substr(prefix.size()),
            (std::string{"\x1b\0\377", 3} + "\'?"));

  char buffer[32] = {};
  const auto end = fmt::backslash_escape_to(buffer, std::string{"\x1b\x7f"});
  EXPECT_EQ(end, buffer + 6);
  EXPECT_EQ(std::string(buffer, end), "\\e\\x7f");
  const auto decoded_end = fmt::backslash_unescape_to(buffer, "\\e\\x7f");
  EXPECT_EQ(decoded_end, buffer + 2);
  EXPECT_EQ(std::string(buffer, decoded_end), "\x1b\x7f");
  EXPECT_EQ(fmt::backslash_escape_to(buffer, ""), buffer);
  EXPECT_EQ(fmt::backslash_unescape_to(buffer, ""), buffer);

  auto wide = std::wstring{L"prefix:"};
  fmt::backslash_escape_to(std::back_inserter(wide), std::wstring{L"\x1b\n"});
  EXPECT_EQ(wide, L"prefix:\\e\\n");
  wchar_t wide_buffer[8] = {};
  auto wide_end = fmt::backslash_unescape_to(wide_buffer, L"\\e\\n");
  EXPECT_EQ(wide_end, wide_buffer + 2);
  EXPECT_EQ(std::wstring(wide_buffer, wide_end), L"\x1b\n");
}
