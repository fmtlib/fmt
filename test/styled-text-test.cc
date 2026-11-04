#include "fmt/styled_text.h"
#include "fmt/xchar.h"
#include "gtest-extra.h"

namespace {
template<typename Char>
auto text(const char* value) -> std::basic_string<Char> {
  auto result = std::basic_string<Char>{};
  for (; *value; ++value) result.push_back(static_cast<Char>(*value));
  return result;
}

#define CHECK_NO_MEMBER(name) \
  template<typename T> auto has_##name(int) \
      -> decltype(std::declval<T&>().name(), std::true_type{}); \
  template<typename> auto has_##name(...) -> std::false_type
CHECK_NO_MEMBER(is_branch);
CHECK_NO_MEMBER(as_branch);
CHECK_NO_MEMBER(as_leaf);
CHECK_NO_MEMBER(begin);
#undef CHECK_NO_MEMBER

template<typename T> auto has_get(int)
    -> decltype(fmt::get<typename T::string_type>(std::declval<T&>()),
                std::true_type{});
template<typename> auto has_get(...) -> std::false_type;
template<typename T> auto has_holds(int)
    -> decltype(fmt::holds_alternative<typename T::string_type>(
                    std::declval<const T&>()), std::true_type{});
template<typename> auto has_holds(...) -> std::false_type;

template<typename Char>
void check_public_surface() {
  using node = fmt::basic_styled_text<Char>;
  using sequence = std::vector<node>;
  static_assert(!std::is_constructible<node, sequence>::value,
                "vector construction must not be public");
  static_assert(!std::is_constructible<node, sequence, fmt::text_style>::value,
                "styled vector construction must not be public");
  static_assert(!decltype(has_is_branch<node>(0))::value, "no branch query");
  static_assert(!decltype(has_as_branch<node>(0))::value, "no child access");
  static_assert(!decltype(has_as_leaf<node>(0))::value, "no leaf access");
  static_assert(!decltype(has_begin<node>(0))::value, "no iteration");
  static_assert(!decltype(has_get<node>(0))::value, "no variant get");
  static_assert(!decltype(has_holds<node>(0))::value, "no variant query");
  static_assert(std::is_nothrow_move_constructible<node>::value,
                "move construction remains noexcept");
  static_assert(std::is_nothrow_move_assignable<node>::value,
                "move assignment remains noexcept");
}

template<typename Char>
void check_construction_and_composition() {
  using node = fmt::basic_styled_text<Char>;
  const auto empty = node{};
  EXPECT_TRUE(empty.empty());
  EXPECT_TRUE(empty.style().empty());
  const auto a = node(text<Char>("a"), fmt::bold());
  const auto b = node(text<Char>("b"), fmt::underline());
  const auto c = node(text<Char>("c"));
  auto joined = a + b;
  EXPECT_FALSE(joined.empty());
  EXPECT_TRUE(joined.style().empty());
  EXPECT_NE(joined, b + a);
  EXPECT_NE(joined, node(text<Char>("ab")));
  auto appended = a;
  EXPECT_EQ(&(appended += b), &appended);
  EXPECT_EQ(appended, joined);
  appended += c;
  EXPECT_EQ(appended, a + b + c);
  EXPECT_NE(appended, a + (b + c));
  auto styled = joined;
  EXPECT_EQ(&styled.style(fmt::italic()), &styled);
  EXPECT_EQ(styled.style(), fmt::italic());
  EXPECT_NE(styled, joined);
  EXPECT_TRUE(joined.style().empty());
  EXPECT_EQ(empty + a, a);
  EXPECT_EQ(a + empty, a);
  EXPECT_EQ(empty + empty, empty);
  auto styled_empty = node(text<Char>(""), fmt::bold());
  styled_empty += a;
  EXPECT_EQ(styled_empty, a);
  auto repeated = joined;
  const auto snapshot = repeated;
  repeated += repeated;
  EXPECT_EQ(repeated, snapshot + snapshot);
  const auto bytes = std::basic_string<Char>(3, Char{});
  EXPECT_FALSE(node(bytes).empty());
  auto binary = node(bytes);
  binary += a;
  EXPECT_EQ(binary, node(bytes) + a);
}

template<typename Char>
void check_copy_move_and_reuse() {
  using node = fmt::basic_styled_text<Char>;
  auto input = std::basic_string<Char>(200, static_cast<Char>('x'));
  const auto leaf = node(input, fmt::bold());
  input[0] = static_cast<Char>('y');
  EXPECT_EQ(leaf, node(std::basic_string<Char>(200, static_cast<Char>('x')),
                       fmt::bold()));
  const auto branch = leaf + text<Char>("tail");
  for (int alternative = 0; alternative != 2; ++alternative) {
    const auto expected = alternative == 0 ? leaf : branch;
    auto source = expected;
    auto moved = node(std::move(source));
    EXPECT_EQ(moved, expected);
    EXPECT_TRUE(source.empty());
    EXPECT_EQ(source, node{});
    source += text<Char>("reused");
    EXPECT_EQ(source, node(text<Char>("reused")));
    // Exercise both target alternatives for both source alternatives.
    for (int target = 0; target != 2; ++target) {
      auto assigned = target == 0 ? leaf : branch;
      auto rhs = expected;
      assigned = std::move(rhs);
      EXPECT_EQ(assigned, expected);
      EXPECT_EQ(rhs, node{});
      rhs += text<Char>("again");
      EXPECT_EQ(rhs, node(text<Char>("again")));
      auto& self = assigned;
      assigned = self;
      EXPECT_EQ(assigned, expected);
      assigned = std::move(self);
      EXPECT_EQ(assigned, expected);
      assigned = node{};
      EXPECT_TRUE(assigned.empty());
      assigned = expected;
      EXPECT_EQ(assigned, expected);
    }
  }
  auto copy = branch;
  copy.style(fmt::italic());
  copy += text<Char>("extra");
  EXPECT_EQ(branch, leaf + text<Char>("tail"));
  auto many = node{};
  for (int i = 0; i < 100; ++i) many += leaf;
  const auto snapshot = many;
  auto moved = node(std::move(many));
  EXPECT_EQ(moved, snapshot);
  EXPECT_EQ(many, node{});
  many += leaf;
  EXPECT_EQ(many, leaf);
}
} // namespace

TEST(styled_text_test, narrow_public_surface) { check_public_surface<char>(); }
TEST(styled_text_test, wide_public_surface) { check_public_surface<wchar_t>(); }
TEST(styled_text_test, narrow_composition) { check_construction_and_composition<char>(); }
TEST(styled_text_test, wide_composition) { check_construction_and_composition<wchar_t>(); }
TEST(styled_text_test, narrow_copy_move_reuse) { check_copy_move_and_reuse<char>(); }
TEST(styled_text_test, wide_copy_move_reuse) { check_copy_move_and_reuse<wchar_t>(); }

namespace {
template<typename Char>
void check_shared_text_lifetime_and_equality() {
  using node = fmt::basic_styled_text<Char>;
  const auto large = std::basic_string<Char>(100000, static_cast<Char>('x'));
  auto surviving_leaf = node{};
  auto surviving_branch = node{};
  {
    auto source = node(large, fmt::bold());
    surviving_leaf = source;
    surviving_branch = source + text<Char>("tail");
    source.style(fmt::underline());
    source += text<Char>("different");
    EXPECT_EQ(surviving_leaf.style(), fmt::bold());
  }
  // Independently allocated strings compare by content, not pointer identity.
  const auto separate = node(large, fmt::bold());
  EXPECT_EQ(surviving_leaf, separate);
  EXPECT_EQ(surviving_branch, separate + text<Char>("tail"));
  auto different = large;
  different.back() = static_cast<Char>('y');
  EXPECT_NE(surviving_leaf, node(different, fmt::bold()));
  auto copy = surviving_branch;
  copy.style(fmt::italic());
  copy += text<Char>("more");
  EXPECT_EQ(surviving_branch, separate + text<Char>("tail"));
  const auto empty_string = node(std::basic_string<Char>{});
  EXPECT_EQ(empty_string, node{});
  EXPECT_NE(empty_string, separate);
  EXPECT_NE(separate, empty_string);
  const auto nulls = std::basic_string<Char>(3, Char{});
  EXPECT_EQ(node(nulls), node(nulls));
  EXPECT_FALSE(node(nulls).empty());
}
} // namespace

TEST(styled_text_test, narrow_shared_text_lifetime) {
  check_shared_text_lifetime_and_equality<char>();
}
TEST(styled_text_test, wide_shared_text_lifetime) {
  check_shared_text_lifetime_and_equality<wchar_t>();
}

namespace {
template<typename Char>
void check_plain_text() {
  using node = fmt::basic_styled_text<Char>;
  using string_type = typename node::string_type;
  static_assert(std::is_same<decltype(std::declval<const node&>().plain_text()),
                             string_type>::value, "returns an owning string");
  EXPECT_EQ(node{}.plain_text(), string_type{});
  EXPECT_EQ((node{string_type{}, fmt::bold()}.plain_text()), string_type{});
  const auto a = node{text<Char>(" a "), fmt::bold()};
  auto nested = a + (node{text<Char>("b"), fmt::underline()} + text<Char>("\tc\n"));
  nested.style(fmt::italic());
  const auto snapshot = nested;
  const auto shared_copy = nested;
  const auto& read_only = nested;
  auto result = read_only.plain_text();
  EXPECT_EQ(result, text<Char>(" a b\tc\n"));
  EXPECT_EQ(nested, snapshot);
  EXPECT_EQ(nested.style(), fmt::italic());
  result[0] = static_cast<Char>('z');
  EXPECT_EQ(nested.plain_text(), text<Char>(" a b\tc\n"));
  EXPECT_EQ(shared_copy.plain_text(), text<Char>(" a b\tc\n"));
  const auto nulls = string_type(3, Char{});
  EXPECT_EQ((a + node{nulls} + text<Char>("tail")).plain_text(),
            text<Char>(" a ") + nulls + text<Char>("tail"));
  const auto escapes = text<Char>("\x1b[31mred\x1b[0m");
  EXPECT_EQ((node{escapes, fmt::bold()}.plain_text()), escapes);
  const auto large = string_type(100000, static_cast<Char>('x'));
  EXPECT_EQ(node{large}.plain_text(), large);
  auto many = node{};
  for (int i = 0; i < 100; ++i) many += a;
  auto expected = string_type{};
  for (int i = 0; i < 100; ++i) expected.append(text<Char>(" a "));
  EXPECT_EQ(many.plain_text(), expected);
  auto moved = node{std::move(nested)};
  EXPECT_EQ(nested.plain_text(), string_type{});
  EXPECT_EQ(moved.plain_text(), text<Char>(" a b\tc\n"));
  nested = std::move(moved);
  EXPECT_EQ(moved.plain_text(), string_type{});
  EXPECT_EQ(nested.plain_text(), text<Char>(" a b\tc\n"));
  auto survivor = node{};
  {
    const auto source = node{large} + a;
    survivor = source;
  }
  EXPECT_EQ(survivor.plain_text(), large + text<Char>(" a "));
}
} // namespace

TEST(styled_text_test, narrow_plain_text) { check_plain_text<char>(); }
TEST(styled_text_test, wide_plain_text) { check_plain_text<wchar_t>(); }

namespace {
template<typename Char>
void check_string_concatenation() {
  using node = fmt::basic_styled_text<Char>;
  using string = typename node::string_type;
  static_assert(std::is_convertible<const string&, node>::value,
                "strings convert directly to owning nodes");
  auto backing = string(200, static_cast<Char>('a'));
  const auto original = backing;
  const auto prefix = text<Char>("prefix ");
  const auto suffix = text<Char>(" suffix");
  node value = backing;
  const auto copy = value;
  const auto paragraph = prefix + value + suffix;
  EXPECT_EQ(paragraph.plain_text(), prefix + original + suffix);
  EXPECT_EQ((value + suffix).plain_text(), original + suffix);
  value += suffix;
  EXPECT_EQ(value.plain_text(), original + suffix);
  backing[0] = static_cast<Char>('z');
  EXPECT_EQ(copy.plain_text(), original);
  EXPECT_EQ(value.plain_text(), original + suffix);
  EXPECT_EQ(paragraph.plain_text(), prefix + original + suffix);
  backing.clear();
  backing.shrink_to_fit();
  EXPECT_EQ(copy.plain_text(), original);
  EXPECT_EQ(value.plain_text(), original + suffix);
  EXPECT_EQ(paragraph.plain_text(), prefix + original + suffix);

  // Temporary string operands can be destroyed before the resulting tree.
  const auto temporary_text = string(200, static_cast<Char>('b'));
  const auto temporary_rhs = node{prefix} + string(200, static_cast<Char>('b'));
  const auto temporary_lhs = string(200, static_cast<Char>('b')) + node{suffix};
  auto temporary_append = node{prefix};
  temporary_append += string(200, static_cast<Char>('b'));
  EXPECT_EQ(temporary_rhs.plain_text(), prefix + temporary_text);
  EXPECT_EQ(temporary_lhs.plain_text(), temporary_text + suffix);
  EXPECT_EQ(temporary_append.plain_text(), prefix + temporary_text);

  EXPECT_EQ(string{} + copy, copy);
  EXPECT_EQ(copy + string{}, copy);
  auto appended_empty = copy;
  appended_empty += string{};
  EXPECT_EQ(appended_empty, copy);
}
} // namespace

TEST(styled_text_test, narrow_string_concatenation) {
  check_string_concatenation<char>();
}
TEST(styled_text_test, wide_string_concatenation) {
  check_string_concatenation<wchar_t>();
}

namespace {
template<typename Char>
void check_overlay() {
  using node = fmt::basic_styled_text<Char>;
  using string = typename node::string_type;
  static_assert(noexcept(std::declval<node&>().overlay(fmt::text_style{})),
                "member overlay does not throw");
  static_assert(std::is_same<decltype(std::declval<node&>().overlay(
                                fmt::text_style{})), node&>::value,
                "member overlay returns the same node");
  const auto left = node{text<Char>("left"), fmt::italic()};
  const auto right = node{text<Char>("right"), fmt::faint()};
  auto source = left + right;
  source.style(fmt::fg(fmt::color::blue) | fmt::bg(fmt::gray_color{5}) |
               fmt::underline());
  const auto snapshot = source;
  const auto overrides = fmt::fg(fmt::term_color::red) | fmt::bold();
  auto expected = left + right;
  expected.style(fmt::fg(fmt::term_color::red) | fmt::bg(fmt::gray_color{5}) |
                 fmt::underline() | fmt::bold());
  auto member = source;
  EXPECT_EQ(&member.overlay(overrides), &member);
  EXPECT_EQ(member, expected);
  EXPECT_EQ(member.style().fg().index(),
            fmt::to_underlying(fmt::color_t::kind::term));
  EXPECT_EQ(member.plain_text(), text<Char>("leftright"));
  member.overlay(fmt::text_style{}).overlay(member.style());
  EXPECT_EQ(member, expected);
  EXPECT_EQ(fmt::overlay(overrides, source), expected);
  EXPECT_EQ(source, snapshot);
  auto argument = source;
  EXPECT_EQ(fmt::overlay(overrides, std::move(argument)), expected);
  EXPECT_EQ(argument, node{});

  auto empty = node{};
  empty.overlay(overrides);
  EXPECT_TRUE(empty.empty());
  EXPECT_EQ(empty.style(), overrides);
  EXPECT_EQ(empty, (node{string{}, overrides}));
  EXPECT_EQ(fmt::overlay(overrides, string{}), empty);
  auto input = string(200, static_cast<Char>('x'));
  input[1] = Char{};
  const auto original = input;
  const auto owned = fmt::overlay(overrides, input);
  input.assign(400, static_cast<Char>('y'));
  EXPECT_EQ(owned.plain_text(), original);
  EXPECT_EQ(owned.style(), overrides);
  const auto temporary = fmt::overlay(overrides, string(200, Char{}));
  EXPECT_EQ(temporary.plain_text(), string(200, Char{}));
}

template<typename Char>
void check_emphasis_modifiers() {
  using node = fmt::basic_styled_text<Char>;
  using string = typename node::string_type;
  struct modifier {
    fmt::emphasis bit;
    node (*on_node)(node&&);
    node (*on_string)(const string&);
    fmt::text_style (*factory)(bool);
  };
  const modifier modifiers[] = {
      {fmt::emphasis::bold, fmt::bold<node>,
       fmt::bold<const string&>, fmt::bold},
      {fmt::emphasis::faint, fmt::faint<node>,
       fmt::faint<const string&>, fmt::faint},
      {fmt::emphasis::italic, fmt::italic<node>,
       fmt::italic<const string&>, fmt::italic},
      {fmt::emphasis::underline, fmt::underline<node>,
       fmt::underline<const string&>, fmt::underline},
      {fmt::emphasis::blink, fmt::blink<node>,
       fmt::blink<const string&>, fmt::blink},
      {fmt::emphasis::reverse, fmt::reverse<node>,
       fmt::reverse<const string&>, fmt::reverse},
      {fmt::emphasis::conceal, fmt::conceal<node>,
       fmt::conceal<const string&>, fmt::conceal},
      {fmt::emphasis::strike, fmt::strike<node>,
       fmt::strike<const string&>, fmt::strike}};
  const auto leaf = node{text<Char>("left"), fmt::underline()};
  const auto sibling = node{text<Char>("right"), fmt::faint()};
  auto source = leaf + sibling;
  const auto base_style = fmt::fg(fmt::color::blue) |
                          fmt::bg(fmt::term_color::black) | fmt::italic();
  source.style(base_style);
  const auto snapshot = source;
  for (const auto& item : modifiers) {
    auto expected = source;
    expected.style(fmt::text_style{base_style.fg(), base_style.bg(),
                                  base_style.emph() | item.bit});
    auto argument = source;
    EXPECT_EQ(item.on_node(std::move(argument)), expected);
    EXPECT_EQ(argument, node{});
    EXPECT_EQ(source, snapshot);
    auto input = string(200, static_cast<Char>('x'));
    input[1] = Char{};
    const auto original = input;
    const auto result = item.on_string(input);
    input.assign(400, static_cast<Char>('y'));
    EXPECT_EQ(result.plain_text(), original);
    EXPECT_EQ(result.style(), fmt::text_style{item.bit});
    EXPECT_EQ(item.factory(true), fmt::text_style{item.bit});
    EXPECT_TRUE(item.factory(false).empty());
  }
}
} // namespace

TEST(styled_text_test, narrow_overlay) { check_overlay<char>(); }
TEST(styled_text_test, wide_overlay) { check_overlay<wchar_t>(); }
TEST(styled_text_test, narrow_emphasis_modifiers) {
  check_emphasis_modifiers<char>();
}
TEST(styled_text_test, wide_emphasis_modifiers) {
  check_emphasis_modifiers<wchar_t>();
}
TEST(styled_text_test, modifier_literals_and_composition) {
  static_assert(std::is_same<decltype(fmt::bold()), fmt::text_style>::value,
                "the no-argument style factory is unchanged");
  static_assert(std::is_same<decltype(fmt::bold(fmt::text_style{})),
                             fmt::text_style>::value,
                "the style modifier overload is unchanged");
  static_assert(std::is_same<decltype(fmt::bold("text")),
                             fmt::styled_text>::value,
                "literals select the text modifier");
  static_assert(std::is_same<decltype(fmt::bold(L"text")),
                             fmt::wstyled_text>::value,
                "wide literals produce wide styled text");
  const auto inner = fmt::styled_text{"nested bold", fmt::bold()};
  auto expected = fmt::styled_text{"red text with "} + inner + " still red";
  expected.style(fmt::fg(fmt::color::red));
  const auto line = fmt::overlay(fmt::fg(fmt::color::red),
      "red text with " + fmt::bold("nested bold") + " still red");
  EXPECT_EQ(line, expected);
  EXPECT_EQ(line.plain_text(), "red text with nested bold still red");
  EXPECT_EQ(fmt::underline(fmt::bold("text")),
            (fmt::styled_text{"text", fmt::bold() | fmt::underline()}));
  EXPECT_EQ(fmt::overlay(fmt::italic(), L"wide"),
            (fmt::wstyled_text{L"wide", fmt::italic()}));
  const auto wide = fmt::overlay(fmt::bg(fmt::gray_color{12}),
      L"prefix " + fmt::bold(L"bold") + L" suffix");
  EXPECT_EQ(wide.plain_text(), L"prefix bold suffix");
  EXPECT_EQ(wide.style(), fmt::bg(fmt::gray_color{12}));
  EXPECT_TRUE(fmt::bold("").empty());
  EXPECT_EQ(fmt::bold("").style(), fmt::bold());
}

TEST(styled_text_test, aliases_and_literals) {
  static_assert(std::is_same<fmt::styled_text, fmt::basic_styled_text<char>>::value,
                "narrow alias");
  static_assert(std::is_same<fmt::wstyled_text, fmt::basic_styled_text<wchar_t>>::value,
                "wide alias");
  static_assert(!std::is_constructible<fmt::styled_text, fmt::wstyled_text>::value,
                "no implicit character conversion");
  fmt::styled_text narrow = "hello";
  fmt::wstyled_text wide = L"\u03b1";
  EXPECT_EQ("a" + narrow, fmt::styled_text("a") + narrow);
  EXPECT_EQ(wide + L"\u03b2", wide + std::wstring(L"\u03b2"));
  EXPECT_EQ((wide + L"\u03b2").plain_text(), std::wstring{L"\u03b1\u03b2"});
}

namespace {
template<typename Char>
void check_absolute_helper() {
  using node = fmt::basic_styled_text<Char>;
  using string = typename node::string_type;
  auto source = fmt::bold(text<Char>("left")) +
                fmt::underline(text<Char>("right"));
  source.style(fmt::fg(fmt::term_color::red));
  const auto saved = source;
  auto expected = source;
  expected.style(fmt::absolute(expected.style()));
  EXPECT_EQ(fmt::absolute(source), expected);
  EXPECT_EQ(source, saved);
  EXPECT_EQ(fmt::absolute(expected), expected);
  auto movable = source;
  EXPECT_EQ(fmt::absolute(std::move(movable)), expected);
  EXPECT_EQ(movable, node{});
  EXPECT_EQ(fmt::absolute(text<Char>("owned")),
            (node{text<Char>("owned"), fmt::absolute(fmt::text_style{})}));
  EXPECT_EQ(fmt::absolute(string{}),
            (node{string{}, fmt::absolute(fmt::text_style{})}));
}
} // namespace

TEST(styled_text_test, narrow_absolute) { check_absolute_helper<char>(); }
TEST(styled_text_test, wide_absolute) { check_absolute_helper<wchar_t>(); }

TEST(styled_text_test, absolute_literal_overloads) {
  EXPECT_EQ(fmt::absolute("text"),
            (fmt::styled_text{"text", fmt::absolute(fmt::text_style{})}));
  EXPECT_EQ(fmt::absolute(L"text"),
            (fmt::wstyled_text{L"text", fmt::absolute(fmt::text_style{})}));
}

namespace {
template<typename Char>
void check_display_width(const std::basic_string<Char>& value) {
  using node = fmt::basic_styled_text<Char>;
  const auto leaf = node{value, fmt::bold()};
  const auto width = leaf.display_width();
  const auto formatted = fmt::format(
      fmt::runtime(text<Char>("{:>20}")), value);
  // Ordinary formatting adds one space per missing display column.
  EXPECT_EQ(formatted.size() - value.size(), std::size_t{20} - width);
  auto nested = fmt::underline(leaf + node{text<Char>("!")});
  EXPECT_EQ(nested.display_width(), width + 1);
  auto moved = node{std::move(nested)};
  EXPECT_EQ(moved.display_width(), width + 1);
  EXPECT_EQ(nested.display_width(), std::size_t{0});
}
} // namespace

TEST(styled_text_test, display_width) {
  EXPECT_EQ(fmt::styled_text{}.display_width(), std::size_t{0});
  EXPECT_EQ(fmt::wstyled_text{}.display_width(), std::size_t{0});
  EXPECT_EQ(fmt::bold("").display_width(), std::size_t{0});
  const std::string samples[] = {
      "", "ASCII", "caf\xc3\xa9", "\xe7\x95\x8c", "\xf0\x9f\x98\x80",
      "e\xcc\x81", "\xff", std::string{"a\0b", 3}, "\x1b[1mx\x1b[0m"};
  for (const auto& sample : samples) check_display_width(sample);
  EXPECT_EQ(fmt::styled_text{"caf\xc3\xa9"}.display_width(), std::size_t{4});
  EXPECT_EQ(fmt::styled_text{"\xe7\x95\x8c"}.display_width(), std::size_t{2});
  check_display_width(std::wstring{L"wide"});
  check_display_width(std::wstring{L"\u754c"});
  check_display_width(std::wstring{L"e\u0301"});
}
