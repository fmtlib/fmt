#include "fmt/strip_ansi.h"
#include "gtest-extra.h"

namespace {
template<typename Char>
auto chars(const char* input) -> std::basic_string<Char> {
  auto result = std::basic_string<Char>{};
  while (*input) result.push_back(static_cast<Char>(*input++));
  return result;
}
template<typename Char>
auto strip(const std::basic_string<Char>& input) -> std::basic_string<Char>
{ return fmt::strip_ansi<Char>({input.data(), input.size()}); }

template<typename Char>
void check_complete_sequences() {
  const char* controls[] = {
      "\x1b[m", "\x1b[1;31m", "\x1b[38:2::1:2:3m", "\x1b[38;5;999m",
      "\x1b[?25l", "\x1b[>0c", "\x1b[!p", "\x1b[2J", "\x1b[3M",
      "\x1b[999999999999999999999m", "\x1b[?42;88 $z",
      "\x1b" "c", "\x1b" "7", "\x1b" "8", "\x1b(B", "\x1b%G", "\x1b F", "\x1b\\",
      "\x1b]title\a", "\x1b]title\x1b\\", "\x1b]title\x1b[31mopaque\a",
      "\x1bPpayload\x1b[1m\x1b\\", "\x1bXpayload\x1b\\",
      "\x1b^payload\x1b\\", "\x1b_payload\x1b\\"};
  for (const auto control : controls) {
    const auto sequence = chars<Char>(control);
    EXPECT_EQ(strip(sequence), std::basic_string<Char>{});
    EXPECT_EQ(strip(chars<Char>("before") + sequence + chars<Char>("after")),
              chars<Char>("beforeafter"));
  }
  EXPECT_EQ(strip(chars<Char>("\x1b[31mred\x1b[0m")), chars<Char>("red"));
  EXPECT_EQ(strip(chars<Char>("\x1b]8;;https://example.com\x1b\\label\x1b]8;;\x1b\\")),
            chars<Char>("label"));
}

template<typename Char>
void check_preservation() {
  const char* malformed[] = {
      "", "plain", "\x1b", "\x1b[", "\x1b[31", "\x1b(",
      "\x1b[1 2m", "\x1b\n", "\x1b[31\n", "\x1b]unfinished\x1b[31mtext",
      "\x1bPunfinished\a", "\x1bXunfinished", "\x1b^unfinished", "\x1b_unfinished",
      "\x1b]cancel\x18text", "\x1bPcancel\x1atext"};
  for (const auto value : malformed) {
    const auto input = chars<Char>(value);
    EXPECT_EQ(strip(input), input);
  }
  // Ordinary controls are retained, including embedded NUL and C1 code units.
  auto controls = std::basic_string<Char>{};
  for (unsigned i = 0; i < 32; ++i)
    if (i != 27) controls.push_back(static_cast<Char>(i));
  for (unsigned i = 128; i < 160; ++i) controls.push_back(static_cast<Char>(i));
  EXPECT_EQ(strip(controls), controls);
  const auto interrupted = chars<Char>("\x1b[31\x1b[1mb");
  EXPECT_EQ(strip(interrupted), chars<Char>("\x1b[31b"));
  const auto input = std::basic_string<Char>{Char('a'), Char{}, Char('b')};
  EXPECT_EQ(strip(input + chars<Char>("\x1b[0m")), input);
}

template<typename Char>
void check_styled_text() {
  using text = fmt::basic_styled_text<Char>;
  const auto red = fmt::fg(fmt::term_color::red);
  const auto absolute = fmt::absolute(fmt::underline());
  const auto input = fmt::bold(
      text{chars<Char>("\x1b[32mred\x1b[0m"), red} +
      fmt::overlay(absolute,
          text{chars<Char>("\x1b]8;;url\a label\x1b]8;;\a")} +
          text{chars<Char>("\x1b[2Jtail"), fmt::italic()}));
  const auto expected = fmt::bold(
      text{chars<Char>("red"), red} +
      fmt::overlay(absolute,
          text{chars<Char>(" label")} +
          text{chars<Char>("tail"), fmt::italic()}));
  const auto original = input;
  auto result = fmt::strip_ansi(input);
  EXPECT_EQ(result, expected); // Equality checks exact structure and styles.
  EXPECT_EQ(input, original);
  EXPECT_EQ(fmt::strip_ansi(result), result);
  result += text{chars<Char>("changed")};
  EXPECT_EQ(input, original);
  EXPECT_EQ(fmt::strip_ansi(text{chars<Char>("\x1b[0m"), absolute}),
            (text{std::basic_string<Char>{}, absolute}));
  EXPECT_EQ(fmt::strip_ansi(text{}), text{});

  const auto only_controls = text{chars<Char>("\x1b[0m"), red} +
                             text{chars<Char>("\x1b[2J"), absolute};
  const auto empty_branch = fmt::strip_ansi(only_controls);
  EXPECT_EQ(empty_branch.plain_text(), std::basic_string<Char>{});
  EXPECT_NE(empty_branch, text{}); // Empty children are not collapsed.
  EXPECT_EQ(fmt::strip_ansi(empty_branch), empty_branch);

  // Incomplete CSI and control strings cannot consume text in another leaf.
  const auto split = text{chars<Char>("\x1b[3"), red} +
                     text{chars<Char>("1mtext"), absolute} +
                     text{chars<Char>("\x1b]title"), fmt::italic()} +
                     text{chars<Char>("\aend")};
  EXPECT_EQ(fmt::strip_ansi(split), split);
  const auto nul_text = std::basic_string<Char>{Char('a'), Char{}, Char('b')};
  EXPECT_EQ(fmt::strip_ansi(text{nul_text + chars<Char>("\x1b[m"), red}),
            (text{nul_text, red}));
  const auto malformed = text{chars<Char>("\x1b[1 2m\x1b]unfinished"), red};
  EXPECT_EQ(fmt::strip_ansi(malformed), malformed);
}
} // namespace

TEST(strip_ansi_test, complete_sequences) {
  check_complete_sequences<char>();
  check_complete_sequences<wchar_t>();
}
TEST(strip_ansi_test, preservation) {
  check_preservation<char>();
  check_preservation<wchar_t>();
}
TEST(strip_ansi_test, overloads_and_ownership) {
  EXPECT_EQ(fmt::strip_ansi("text"), "text");
  EXPECT_EQ(fmt::strip_ansi(L"text"), L"text");
  auto input = std::string{"\x1b[31mowned\x1b[0m"};
  const auto result = fmt::strip_ansi(input);
  input.assign(200, 'x');
  EXPECT_EQ(result, "owned");
  EXPECT_EQ(fmt::strip_ansi(std::wstring{L"wide"}), L"wide");
  const auto utf8 = std::string{"caf\xc3\xa9 \xe7\x95\x8c \xf0\x9f\x98\x80"};
  EXPECT_EQ(fmt::strip_ansi("\x1b[1m" + utf8 + "\x1b[m"), utf8);
}

TEST(strip_ansi_test, styled_text) {
  check_styled_text<char>();
  check_styled_text<wchar_t>();
}
