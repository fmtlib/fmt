#include "fmt/styled_text.h"
#include "fmt/compile.h"
#include "fmt/xchar.h"
#include "fmt/color-ostream.h"
#include "gtest-extra.h"

namespace {
auto render(const fmt::styled_text& value) -> std::string {
  return fmt::format(FMT_STRING("{}"), value);
}
auto render(const fmt::wstyled_text& value) -> std::wstring {
  return fmt::format(FMT_STRING(L"{}"), value);
}
template<typename Char>
auto chars(const char* value) -> std::basic_string<Char> {
  auto result = std::basic_string<Char>{};
  for (; *value; ++value) result.push_back(static_cast<Char>(*value));
  return result;
}

// A small ANSI interpreter checks the state of each output code unit, without
// using overlay or to_sgr to construct the expected terminal state.
template<typename Char>
void check_states(const std::basic_string<Char>& output,
                  const std::basic_string<Char>& text,
                  const std::vector<fmt::text_style>& expected) {
  ASSERT_EQ(text.size(), expected.size());
  auto state = fmt::text_style{};
  size_t position = 0;
  for (size_t i = 0; i < output.size();) {
    if (output[i] != Char('\x1b')) {
      ASSERT_LT(position, text.size());
      EXPECT_EQ(output[i++], text[position]);
      EXPECT_EQ(state, expected[position]);
      ++position;
      continue;
    }
    ASSERT_LT(i + 1, output.size());
    ASSERT_EQ(output[i + 1], Char('['));
    i += 2;
    auto parameters = std::vector<unsigned>{};
    unsigned number = 0;
    for (;;) {
      ASSERT_LT(i, output.size());
      auto c = output[i++];
      if (c >= Char('0') && c <= Char('9')) {
        number = number * 10 + static_cast<unsigned>(c - Char('0'));
        continue;
      }
      ASSERT_TRUE(c == Char(';') || c == Char('m'));
      parameters.push_back(number);
      number = 0;
      if (c == Char('m')) break;
    }
    for (size_t p = 0; p < parameters.size(); ++p) {
      const auto code = parameters[p];
      switch (code) {
        case 0: state.clear(); break;
        case 1: state.bold(); break;
        case 2: state.faint(); break;
        case 3: state.italic(); break;
        case 4: state.underline(); break;
        case 5: state.blink(); break;
        case 7: state.reverse(); break;
        case 8: state.conceal(); break;
        case 9: state.strike(); break;
        case 22: state.bold(false).faint(false); break;
        case 23: state.italic(false); break;
        case 24: state.underline(false); break;
        case 25: state.blink(false); break;
        case 27: state.reverse(false); break;
        case 28: state.conceal(false); break;
        case 29: state.strike(false); break;
        case 39: state.fg(fmt::no_color); break;
        case 49: state.bg(fmt::no_color); break;
        case 38: case 48: {
          ASSERT_LT(p + 1, parameters.size());
          const auto mode = parameters[++p];
          auto color = fmt::color_t{};
          if (mode == 2) {
            ASSERT_LT(p + 3, parameters.size());
            color = fmt::rgb{static_cast<int>(parameters[p + 1]),
                             static_cast<int>(parameters[p + 2]),
                             static_cast<int>(parameters[p + 3])};
            p += 3;
          } else {
            ASSERT_EQ(mode, 5u);
            ASSERT_LT(p + 1, parameters.size());
            const auto index = parameters[++p];
            if (index >= 232)
              color = fmt::gray_color{static_cast<int>(index - 232)};
            else {
              ASSERT_GE(index, 16u);
              color = fmt::cube_color{index - 16};
            }
          }
          if (code == 38) state.fg(color);
          else state.bg(color);
          break;
        }
        default: {
          const bool background = (code >= 40 && code <= 47) ||
                                  (code >= 100 && code <= 107);
          const auto base = background ? (code >= 100 ? 92u : 40u)
                                       : (code >= 90 ? 82u : 30u);
          ASSERT_GE(code, base);
          const auto index = code - base;
          ASSERT_LT(index, 16u);
          if (background) state.bg(static_cast<fmt::term_color>(index));
          else state.fg(static_cast<fmt::term_color>(index));
        }
      }
    }
  }
  EXPECT_EQ(position, text.size());
  EXPECT_EQ(state, fmt::text_style{});
}

template<typename Char>
void check_nested_formatting() {
  using node = fmt::basic_styled_text<Char>;
  const auto outer = fmt::text_style{fmt::rgb{1, 2, 3}, fmt::gray_color{12},
                                     fmt::emphasis::bold};
  auto inner_style = fmt::text_style{fmt::term_color::green, fmt::no_color,
                                    fmt::emphasis::italic};
  auto inner = node(chars<Char>("b")) +
               node(chars<Char>("c"), fmt::underline());
  inner.style(inner_style);
  auto tree = node(chars<Char>("a")) + inner + node(chars<Char>("d"));
  tree.style(outer);
  const auto saved = tree;
  auto b = outer;
  b.fg(fmt::term_color::green).italic();
  auto c = b;
  c.underline();
  check_states(render(tree), chars<Char>("abcd"), {outer, b, c, outer});
  EXPECT_EQ(tree, saved);

  // An absolute group drops outer colors/emphasis, its relative children
  // inherit from that group, and a later sibling resumes the outer state.
  inner_style = fmt::absolute(inner_style);
  inner = fmt::absolute(inner);
  tree = node(chars<Char>("a")) + inner + node(chars<Char>("d"));
  tree.style(outer);
  b = fmt::text_style{fmt::term_color::green, fmt::no_color,
                       fmt::emphasis::italic};
  c = b;
  c.underline();
  check_states(render(tree), chars<Char>("abcd"), {outer, b, c, outer});

  auto plain_absolute = fmt::text_style{};
  plain_absolute.absolute();
  tree = node(chars<Char>("a")) + node(chars<Char>("b"), plain_absolute) +
         node(chars<Char>("c"));
  tree.style(outer);
  check_states(render(tree), chars<Char>("abc"), {outer, {}, outer});

  // An absolute outer group does not force a reset at each relative child.
  tree = node(chars<Char>("a")) + node(chars<Char>("b"));
  auto absolute_outer = outer;
  absolute_outer.absolute();
  tree.style(absolute_outer);
  check_states(render(tree), chars<Char>("ab"), {outer, outer});
  // The absolute root resets even from assumed defaults, in both modes.
  const auto expected = fmt::to_sgr<Char>(absolute_outer) + chars<Char>("ab") +
                        fmt::to_sgr<Char>(fmt::text_style{}, outer);
  EXPECT_EQ(render(tree), expected);

  // Equal complete states still honor an explicit absolute child boundary.
  tree = node(chars<Char>("a")) + node(chars<Char>("b"), absolute_outer);
  tree.style(outer);
  const auto boundary = fmt::to_sgr<Char>(outer, fmt::text_style{}) +
      chars<Char>("a") + fmt::to_sgr<Char>(absolute_outer) + chars<Char>("b") +
      fmt::to_sgr<Char>(fmt::text_style{}, outer);
  EXPECT_EQ(render(tree), boundary);
}
} // namespace

TEST(styled_text_format_test, plain_empty_and_output_iterators) {
  EXPECT_EQ(render(fmt::styled_text{}), "");
  EXPECT_EQ(render(fmt::styled_text{"", fmt::bold()}), "");
  EXPECT_EQ(render(fmt::styled_text{"literal {}"}), "literal {}");
  EXPECT_EQ(fmt::format(FMT_STRING("{:}"), fmt::styled_text{"plain"}), "plain");
  EXPECT_EQ(render(fmt::wstyled_text{L"wide"}), L"wide");
  EXPECT_EQ(fmt::format(FMT_STRING("before{}after{}"), fmt::bold("x"), 42),
            "before\x1b[1mx\x1b[0mafter42");
  auto output = std::vector<char>{'p'};
  fmt::format_to(std::back_inserter(output), FMT_STRING("{}!"), fmt::bold("x"));
  EXPECT_EQ((std::string{output.begin(), output.end()}), "p\x1b[1mx\x1b[0m!");
  const auto binary = std::string{"a\0{}b", 5};
  EXPECT_EQ(render(fmt::styled_text{binary}), binary);
  const auto long_text = std::string(2000, 'x');
  EXPECT_EQ(render(fmt::bold(long_text)), "\x1b[1m" + long_text + "\x1b[0m");
  EXPECT_EQ(fmt::format(FMT_COMPILE("before{}after"), fmt::bold("x")),
            "before\x1b[1mx\x1b[0mafter");
  const auto raw = std::string{"\x1b[4mraw\x1b[24m"};
  EXPECT_EQ(render(fmt::styled_text{raw}), raw);
}

TEST(styled_text_format_test, nested_inheritance_and_absolute_boundaries) {
  check_nested_formatting<char>();
  check_nested_formatting<wchar_t>();
}

namespace {
template<typename Char, typename... T>
auto format_in_style(fmt::text_style style, const char* pattern, const T&... args)
    -> std::basic_string<Char> {
  auto buffer = fmt::basic_memory_buffer<Char>{};
  const auto text = chars<Char>(pattern);
  fmt::detail::vformat_to(buffer, style, fmt::basic_string_view<Char>{text},
      fmt::make_format_args<fmt::buffered_context<Char>>(args...));
  return {buffer.data(), buffer.size()};
}

template<typename Char>
void check_context_styles() {
  const auto outer = fmt::underline(fmt::bg(fmt::gray_color{12}));
  const auto red = fmt::fg(fmt::term_color::bright_red);
  auto inherited = outer;
  inherited.fg(fmt::term_color::bright_red);
  const auto number = 42;
  const auto value = fmt::styled(number, red);
  // Both the exact-"{}" shortcut and replacement fields without specs create
  // contexts for custom formatters. Explicit specs use the handler's context.
  check_states(format_in_style<Char>(outer, "{}", value), chars<Char>("42"),
               {inherited, inherited});
  check_states(format_in_style<Char>(outer, "{:}", value), chars<Char>("42"),
               {inherited, inherited});
  check_states(format_in_style<Char>(outer, "a{}z{}", value, 7),
               chars<Char>("a42z7"), {outer, inherited, inherited, outer, outer});
  check_states(format_in_style<Char>(outer, "a{:>{}}z", value, 4),
               chars<Char>("a  42z"),
               {outer, inherited, inherited, inherited, inherited, outer});

  const auto absolute = fmt::styled(number, fmt::absolute(red));
  check_states(format_in_style<Char>(outer, "a{}z", absolute),
               chars<Char>("a42z"), {outer, red, red, outer});
  const auto clear = fmt::styled(number, fmt::absolute(fmt::text_style{}));
  check_states(format_in_style<Char>(outer, "a{}z", clear),
               chars<Char>("a42z"), {outer, {}, {}, outer});
  check_states(format_in_style<Char>(outer, "a{}z", fmt::styled(42, {})),
               chars<Char>("a42z"), {outer, outer, outer, outer});

  const auto nested = fmt::styled(value, fmt::bold());
  const auto bold_red = fmt::bold(inherited);
  check_states(format_in_style<Char>(outer, "a{}z", nested),
               chars<Char>("a42z"), {outer, bold_red, bold_red, outer});

  using node = fmt::basic_styled_text<Char>;
  auto tree = fmt::overlay(red, node{chars<Char>("x")}) +
              fmt::absolute(fmt::bold(node{chars<Char>("y")}));
  check_states(format_in_style<Char>(outer, "a{}z{}", tree, 7),
               chars<Char>("axyz7"), {outer, inherited, fmt::bold(), outer, outer});
  check_states(format_in_style<Char>(outer, "a{:>4.1}z", tree),
               chars<Char>("a   xz"),
               {outer, outer, outer, outer, inherited, outer});
  check_states(format_in_style<Char>(outer, "a{:>2.0}z", tree),
               chars<Char>("a  z"), {outer, outer, outer, outer});
  check_states(format_in_style<Char>(outer, "a{}z", node{}),
               chars<Char>("az"), {outer, outer});
  check_states(format_in_style<Char>(fmt::absolute(outer), "a{}z", tree),
               chars<Char>("axyz"), {outer, inherited, fmt::bold(), outer});

  const auto wrapped = fmt::styled(tree, fmt::italic());
  check_states(format_in_style<Char>(outer, "a{}z", wrapped),
               chars<Char>("axyz"),
               {outer, fmt::italic(inherited), fmt::bold(), outer});
}
} // namespace

struct color_context_nested {};
struct color_context_throwing {};

FMT_BEGIN_NAMESPACE
template<> struct formatter<color_context_nested> : nested_formatter<int> {
  template<typename Context>
  auto format(color_context_nested, Context& ctx) const
      -> decltype(ctx.out()) {
    return write(ctx, "a", fmt::styled(42, fmt::fg(fmt::term_color::red)), "z");
  }
};
template<> struct formatter<color_context_throwing> : formatter<int> {
  template<typename Context>
  auto format(color_context_throwing, Context& ctx) const -> decltype(ctx.out()) {
    report_error("color context test");
    return ctx.out();
  }
};
FMT_END_NAMESPACE

TEST(styled_text_format_test, nested_buffer_context_inherits_style) {
  const auto outer = fmt::underline();
  const auto red = fmt::underline(fmt::fg(fmt::term_color::red));
  check_states(fmt::format(outer, FMT_STRING("{}"), color_context_nested{}),
               std::string{"a42z"}, {outer, red, red, outer});
  // Nonzero outer width creates a separate buffer context. Its custom child
  // must still inherit underline, even though the complete text exceeds width.
  check_states(fmt::format(outer, FMT_STRING("{:>1}"), color_context_nested{}),
               std::string{"a42z"}, {outer, red, red, outer});
  const auto nested = color_context_nested{};
  const auto wrapped = fmt::styled(nested, outer);
  check_states(fmt::format(FMT_COMPILE("{:>1}"), wrapped),
               std::string{"a42z"}, {outer, red, red, outer});
}

TEST(styled_text_format_test, context_state_is_local_and_exception_safe) {
  auto buffer = fmt::memory_buffer{};
  auto ctx = fmt::format_context{fmt::appender{buffer}, {}};
  EXPECT_EQ(fmt::detail::color_context::get(ctx), uint64_t{0});
  const auto outer = fmt::underline(fmt::bg(fmt::term_color::blue));
  fmt::detail::style_context::set(ctx, outer);
  const auto number = 42;
  auto inner = fmt::styled(number, fmt::fg(fmt::term_color::red));
  auto formatter = fmt::formatter<decltype(inner)>{};
  auto parse_inner = fmt::parse_context<char>{fmt::string_view{}};
  formatter.parse(parse_inner);
  ctx.advance_to(formatter.format(inner, ctx));
  auto ordinary = fmt::format_context{fmt::appender{buffer}, {}};
  EXPECT_EQ(fmt::detail::color_context::get(ordinary), uint64_t{0});
  EXPECT_EQ(fmt::detail::style_context::get(ctx), outer);
#if FMT_USE_EXCEPTIONS
  const auto value = color_context_throwing{};
  auto styled = fmt::styled(value, fmt::bold());
  auto throwing = fmt::formatter<decltype(styled)>{};
  auto parse = fmt::parse_context<char>{fmt::string_view{}};
  throwing.parse(parse);
  EXPECT_THROW((void)throwing.format(styled, ctx), fmt::format_error);
  EXPECT_EQ(fmt::detail::style_context::get(ctx), outer);
#endif
}

TEST(styled_text_format_test, enclosing_format_context) {
  check_context_styles<char>();
  check_context_styles<wchar_t>();
  const auto outer = fmt::underline();
  const auto red = fmt::fg(fmt::term_color::bright_red);
  const auto expected = std::vector<fmt::text_style>{
      outer, fmt::underline(red), fmt::underline(red), outer};
  check_states(fmt::format(outer, FMT_STRING("a{}z"), fmt::styled(42, red)),
               std::string{"a42z"}, expected);
  check_states(fmt::format(outer, FMT_STRING("a{}z"),
                          fmt::overlay(red, std::to_string(42))),
               std::string{"a42z"}, expected);
  std::ostringstream os{};
  fmt::print(os, outer, FMT_STRING("a{}z"), fmt::styled(42, red));
  check_states(os.str(), std::string{"a42z"}, expected);
  std::wostringstream wide{};
  fmt::println(wide, outer, FMT_STRING(L"a{:}z"), fmt::styled(42, red));
  check_states(wide.str(), std::wstring{L"a42z\n"},
               {outer, fmt::underline(red), fmt::underline(red), outer, {}});
  EXPECT_WRITE(stdout, fmt::print(outer, FMT_STRING("a{}z"),
                                 fmt::styled(42, red)),
      format_in_style<char>(outer, "a{}z", fmt::styled(42, red)));
  auto output = std::vector<char>{};
  fmt::format_to(std::back_inserter(output), outer, FMT_STRING("a{}z"),
                 fmt::overlay(red, "42"));
  check_states(std::string{output.begin(), output.end()}, std::string{"a42z"},
               expected);
  // Independent operations begin at defaults, even after styled output.
  EXPECT_EQ(fmt::format(FMT_STRING("{}"), 42), "42");
}

TEST(styled_text_format_test, compiled_nested_style_and_pointer_output) {
  const auto inner = fmt::bold("x");
  const auto wrapped = fmt::styled(inner, fmt::underline());
  const auto both = fmt::underline(fmt::bold());
  check_states(fmt::format(FMT_COMPILE("a{}z"), wrapped),
               std::string{"axz"}, {{}, both, {}});
  auto buffer = std::string(128, '\0');
  const auto end = fmt::format_to(&buffer[0], FMT_COMPILE("a{:>3}z"), wrapped);
  buffer.resize(static_cast<size_t>(end - &buffer[0]));
  check_states(buffer, std::string{"a  xz"},
               {{}, both, both, both, {}});
}

TEST(styled_text_format_test, unsupported_specifications) {
#if FMT_USE_EXCEPTIONS
  const auto value = fmt::bold("text");
  EXPECT_THROW((void)fmt::format(fmt::runtime("{:?}"), value), fmt::format_error);
  EXPECT_THROW((void)fmt::format(fmt::runtime("{:+}"), value), fmt::format_error);
#endif
}

TEST(styled_text_format_test, all_parent_child_emphasis_pairs) {
  const auto colors = fmt::text_style{fmt::term_color::red,
                                      fmt::cube_color{1, 2, 3}};
  for (unsigned parent = 0; parent < 256; ++parent) {
    auto enclosing = colors;
    enclosing.emph(static_cast<fmt::emphasis>(parent));
    for (unsigned child = 0; child < 256; ++child) {
      auto relative = fmt::text_style{static_cast<fmt::emphasis>(child)};
      auto absolute = relative;
      absolute.absolute();
      auto tree = fmt::styled_text{"a"} + fmt::styled_text{"b", relative} +
                  fmt::styled_text{"c", absolute} + fmt::styled_text{"d"};
      tree.style(enclosing);
      auto inherited = colors;
      inherited.emph(static_cast<fmt::emphasis>(parent | child));
      check_states(render(tree), std::string{"abcd"},
                   {enclosing, inherited, relative, enclosing});
    }
  }
}

namespace {
template<typename Char>
void check_string_specifications() {
  using node = fmt::basic_styled_text<Char>;
  const char* specifications[] = {
      "{}", "{:s}", "{:8}", "{:>8}", "{:_<8}", "{:*^9}",
      "{:.0}", "{:.1}", "{:.3}", "{:.20}", "{:>8.3s}", "{:*^8.0}"};
  const auto plain = chars<Char>("abcdef");
  const auto tree = node{chars<Char>("ab")} + node{chars<Char>("cde")} +
                    node{chars<Char>("f")};
  for (const auto spec : specifications) {
    const auto pattern = chars<Char>(spec);
    EXPECT_EQ(fmt::format(fmt::runtime(pattern), tree),
              fmt::format(fmt::runtime(pattern), plain));
    EXPECT_EQ(fmt::format(fmt::runtime(pattern), node{}),
              fmt::format(fmt::runtime(pattern), std::basic_string<Char>{}));
  }
  EXPECT_EQ(fmt::format(fmt::runtime(chars<Char>("{:>{}.{}}")), tree, 8, 3),
            fmt::format(fmt::runtime(chars<Char>("{:>{}.{}}")), plain, 8, 3));
  const auto root = fmt::fg(fmt::term_color::red);
  const auto child = fmt::bold(root);
  const auto absolute = fmt::underline();
  auto styled = node{chars<Char>("ab"), fmt::bold()} +
      fmt::absolute(node{chars<Char>("cd"), absolute}) +
      node{chars<Char>("ef")};
  styled.style(root);
  const auto saved = styled;
  check_states(fmt::format(fmt::runtime(chars<Char>("{:*^8.3}")), styled),
      chars<Char>("**abc***"), {root, root, child, child, absolute, root, root, root});
  check_states(fmt::format(fmt::runtime(chars<Char>("{:>3.0}")), styled),
      chars<Char>("   "), {root, root, root});
  check_states(fmt::format(fmt::runtime(chars<Char>("{:>3}")), node{std::basic_string<Char>{}, root}),
      chars<Char>("   "), {root, root, root});
  EXPECT_EQ(styled, saved);
}
} // namespace

TEST(styled_text_format_test, string_specifications) {
  check_string_specifications<char>();
  check_string_specifications<wchar_t>();
  EXPECT_EQ(fmt::format(FMT_COMPILE("{:>6.3}"), fmt::styled_text{"abcdef"}),
            "   abc");
  auto output = std::vector<char>{};
  fmt::format_to(std::back_inserter(output), FMT_STRING("{:>6.3}"),
                 fmt::styled_text{"abcdef"});
  EXPECT_EQ((std::string{output.begin(), output.end()}), "   abc");
  EXPECT_EQ(fmt::format(FMT_STRING("{0:>{1}.{2}}"), fmt::styled_text{"abcdef"}, 6, 3),
            "   abc");
}

TEST(styled_text_format_test, unicode_precision) {
  const std::string samples[] = {
      "caf\xc3\xa9", "\xe7\x95\x8c!", "\xf0\x9f\x98\x80!", "e\xcc\x81",
      "\xff!", std::string{"a\0b", 3}};
  for (const auto& sample : samples) {
    const auto tree = fmt::styled_text{"x"} + fmt::styled_text{sample} + "later";
    for (int precision = 0; precision < 12; ++precision)
      EXPECT_EQ(fmt::format(FMT_STRING("{:*^14.{}}"), tree, precision),
                fmt::format(FMT_STRING("{:*^14.{}}"), "x" + sample + "later", precision));
  }
  // A wide code point that cannot fit stops the traversal, including later leaves.
  const auto tree = fmt::styled_text{"a"} + "\xe7\x95\x8c" + "b";
  EXPECT_EQ(fmt::format(FMT_STRING("{:.2}"), tree), "a");
  EXPECT_EQ(fmt::format(FMT_STRING("{:\xc2\xb7^5.1}"), tree),
            fmt::format(FMT_STRING("{:\xc2\xb7^5.1}"), "a"));
#if FMT_USE_EXCEPTIONS
  EXPECT_THROW((void)fmt::format(fmt::runtime("{:>{}}"), tree, -1), fmt::format_error);
  EXPECT_THROW((void)fmt::format(fmt::runtime("{:.{}}"), tree, -1), fmt::format_error);
#endif
}

namespace {
template<typename Char>
void check_deferred_transitions() {
  using node = fmt::basic_styled_text<Char>;
  const auto root = fmt::fg(fmt::term_color::red);
  const auto child = fmt::bold(root);
  const auto inner = fmt::underline(child);
  auto tree = fmt::overlay(root, fmt::bold(
      fmt::underline(node{chars<Char>("abc")}) + chars<Char>("later")) +
      chars<Char>("tail"));
  const auto opening = fmt::to_sgr<Char>(inner, fmt::text_style{});
  const auto reset = fmt::to_sgr<Char>(fmt::text_style{}, inner);
  EXPECT_EQ(fmt::format(fmt::runtime(chars<Char>("{:.1}")), tree),
            opening + chars<Char>("a") + reset);
  const auto padded = opening + chars<Char>("a") +
      fmt::to_sgr<Char>(root, inner) + chars<Char>("  ") +
      fmt::to_sgr<Char>(fmt::text_style{}, root);
  EXPECT_EQ(fmt::format(fmt::runtime(chars<Char>("{:<3.1}")), tree), padded);
  EXPECT_EQ(fmt::format(fmt::runtime(chars<Char>("{:.0}")), tree),
            std::basic_string<Char>{});

  // The enclosing group never prints: only its absolute child needs an entry.
  const auto absolute = fmt::absolute(fmt::underline());
  const auto group = fmt::overlay(root, fmt::absolute(fmt::underline(
      node{chars<Char>("abc")})) + chars<Char>("later"));
  auto relative = absolute;
  relative.absolute(false);
  EXPECT_EQ(fmt::format(fmt::runtime(chars<Char>("{:.1}")), group),
      fmt::to_sgr<Char>(absolute, fmt::text_style{}) + chars<Char>("a") +
      fmt::to_sgr<Char>(fmt::text_style{}, relative));
}
} // namespace

TEST(styled_text_format_test, deferred_transitions) {
  check_deferred_transitions<char>();
  check_deferred_transitions<wchar_t>();
}
