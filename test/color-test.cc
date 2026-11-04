// Formatting library for C++ - color tests
//
// Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/color.h"

#include <iterator>  // std::back_inserter
#include <limits>    // std::numeric_limits
#include <map>       // std::map

#include <type_traits>  // std::is_convertible, std::is_same
#include <unordered_map>  // std::unordered_map
#include <unordered_set>  // std::unordered_set
#include <utility>  // std::declval, std::move

#include "gtest-extra.h"  // EXPECT_WRITE, EXPECT_THROW_MSG

#if FMT_COLOR_COMPACT_SGR == 1
#  define FMT_SGR_EXPECTED(legacy, compact) compact
#else
#  define FMT_SGR_EXPECTED(legacy, compact) legacy
#endif

TEST(color_test, absolute_style_flag) {
  static_assert(sizeof(fmt::text_style) == sizeof(uint64_t),
                "absolute does not increase style size");
  static_assert(!fmt::text_style{}.is_absolute(), "styles default to relative");
  static_assert(noexcept(std::declval<fmt::text_style&>().absolute()) &&
                    noexcept(fmt::text_style{}.is_absolute()),
                "absolute accessors do not throw");
  auto style = fmt::fg(fmt::term_color::red) | fmt::bg(fmt::gray_color{12}) |
               fmt::underline();
  const auto original = style;
  EXPECT_EQ(&style.absolute(), &style);
  EXPECT_TRUE(style.is_absolute());
  EXPECT_EQ(fmt::to_underlying(style),
            fmt::to_underlying(original) | (uint64_t{1} << 62));
  EXPECT_NE(style, original);
  EXPECT_EQ(style.fg(), original.fg());
  EXPECT_EQ(style.bg(), original.bg());
  EXPECT_EQ(style.emph(), original.emph());
  EXPECT_EQ(&style.absolute(false), &style);
  EXPECT_EQ(style, original);
  style.absolute().fg(fmt::no_color).bg(fmt::no_color).emph(fmt::emphasis{});
  EXPECT_FALSE(style.empty());
  EXPECT_FALSE(style.has_fg());
  EXPECT_FALSE(style.has_bg());
  EXPECT_FALSE(style.has_emph());
  EXPECT_EQ(&style.clear(), &style);
  EXPECT_EQ(style, fmt::text_style{});
  EXPECT_FALSE(style.is_absolute());
}

TEST(color_test, absolute_style_helper) {
  static_assert(noexcept(fmt::absolute(fmt::text_style{})),
                "absolute style helper does not throw");
  const auto source = fmt::fg(fmt::term_color::red) |
                      fmt::bg(fmt::gray_color{12}) | fmt::underline();
  auto expected = source;
  expected.absolute();
  EXPECT_EQ(fmt::absolute(source), expected);
  EXPECT_FALSE(source.is_absolute());
  EXPECT_EQ(fmt::absolute(expected), expected);
  EXPECT_EQ(fmt::absolute(fmt::text_style{}), fmt::text_style{}.absolute());
#if FMT_USE_CONSTEXPR
  constexpr auto value = fmt::absolute(fmt::text_style{});
  static_assert(value.is_absolute(), "absolute helper supports constant evaluation");
#endif
}

TEST(color_test, absolute_style_composition) {
  const fmt::color_t colors[] = {fmt::no_color, fmt::rgb{1, 2, 3},
      fmt::term_color::red, fmt::cube_color{1, 2, 3}, fmt::gray_color{12}};
  for (auto foreground : colors) {
    for (auto background : colors) {
      const auto base = fmt::text_style{foreground, background,
                                       fmt::emphasis::underline};
      const auto other = fmt::text_style{background, foreground,
                                        fmt::emphasis::bold};
      for (unsigned flags = 0; flags != 4; ++flags) {
        auto lhs = base;
        auto rhs = other;
        lhs.absolute((flags & 1u) != 0);
        rhs.absolute((flags & 2u) != 0);
        auto expected_overlay = fmt::overlay(base, other);
        auto expected_or = base | other;
        expected_overlay.absolute(flags != 0);
        expected_or.absolute(flags != 0);
        EXPECT_EQ(fmt::overlay(lhs, rhs), expected_overlay);
        EXPECT_EQ(lhs | rhs, expected_or);
        EXPECT_EQ(rhs | lhs, expected_or);
        auto assigned = lhs;
        assigned |= rhs;
        EXPECT_EQ(assigned, expected_or);
      }
    }
  }
  const auto absolute_only = fmt::text_style{}.absolute();
  EXPECT_EQ(fmt::overlay(fmt::text_style{}, absolute_only), absolute_only);
  EXPECT_EQ(fmt::overlay(absolute_only, fmt::text_style{}), absolute_only);
  auto style = fmt::underline().absolute();
  style.overlay(fmt::bold());
  EXPECT_TRUE(style.is_absolute());
  EXPECT_TRUE(style.is_bold());
  EXPECT_TRUE(style.is_underline());
  style |= fmt::italic();
  EXPECT_TRUE(style.is_italic());
  EXPECT_TRUE(style.is_absolute());
}

TEST(color_test, absolute_style_blend) {
  const auto full = fmt::text_style{fmt::rgb{10, 20, 30}, fmt::gray_color{12},
                                   fmt::emphasis::bold}.absolute();
  auto relative = full;
  relative.absolute(false);
  const auto other = fmt::text_style{fmt::term_color::blue, fmt::no_color,
                                    fmt::emphasis::underline};
  const auto expected = fmt::blend(relative, other);
  EXPECT_EQ(fmt::blend(full, other), expected);
  EXPECT_EQ(fmt::blend(other, full), expected);
  EXPECT_EQ(fmt::blend(other, relative, full),
            fmt::blend(other, relative, relative));
  EXPECT_FALSE(fmt::blend(full, other).is_absolute());
  EXPECT_TRUE(full.is_absolute());
  static_assert(noexcept(fmt::blend(full, other)), "blend remains noexcept");
  const fmt::text_style invalid[] = {
      fmt::text_style{}.absolute(),
      fmt::fg(fmt::color::red).absolute(),
      fmt::bg(fmt::color::blue).absolute()};
  for (auto input : invalid) {
#ifdef NDEBUG
    auto cleared = input;
    cleared.absolute(false);
    EXPECT_EQ(fmt::blend(input, other), fmt::blend(cleared, other));
    EXPECT_EQ(fmt::blend(other, input), fmt::blend(other, cleared));
    EXPECT_EQ(fmt::blend(other, full, input),
              fmt::blend(other, relative, cleared));
    EXPECT_FALSE(fmt::blend(input, other).is_absolute());
#elif !defined(FMTCOLOR_SKIP)
    EXPECT_DEATH_IF_SUPPORTED((void)fmt::blend(input, other),
                              "terminal-default colors");
    EXPECT_DEATH_IF_SUPPORTED((void)fmt::blend(other, input),
                              "terminal-default colors");
    EXPECT_DEATH_IF_SUPPORTED((void)fmt::blend(other, full, input),
                              "terminal-default colors");
#else
    (void)input;
#endif
  }
}

#if FMT_USE_CONSTEXPR
TEST(color_test, absolute_style_constexpr) {
  constexpr auto reset = fmt::text_style{}.absolute();
  static_assert(reset.is_absolute() && !reset.empty(),
                "a reset-only style is absolute and nonempty");
  constexpr auto full = fmt::text_style{fmt::rgb{10, 20, 30},
                                       fmt::gray_color{12}}.absolute();
  constexpr auto modified = fmt::text_style{full}.overlay(fmt::bold());
  static_assert(modified.is_absolute() && modified.is_bold(),
                "explicit overlays preserve absolute during evaluation");
  constexpr auto combined = fmt::fg(fmt::term_color::red) | full;
  static_assert(combined.is_absolute(), "promoted OR preserves absolute");
  constexpr auto result = fmt::blend(full, fmt::text_style{});
  static_assert(!result.is_absolute() && result.fg() == full.fg() &&
                    result.bg() == full.bg(),
                "valid absolute blends remain constant-evaluable and relative");
  constexpr auto cleared = fmt::text_style{full}.clear();
  static_assert(cleared.empty() && !cleared.is_absolute(),
                "clear removes absolute during evaluation");
}
#endif

template<typename T>
void check_color_comparisons(const T* values, size_t count) {
  static_assert(noexcept(T{} == T{}), "equality must be noexcept");
  static_assert(noexcept(T{} != T{}), "inequality must be noexcept");
  static_assert(noexcept(T{} < T{}), "ordering must be noexcept");
  static_assert(noexcept(T{} <= T{}), "ordering must be noexcept");
  static_assert(noexcept(T{} > T{}), "ordering must be noexcept");
  static_assert(noexcept(T{} >= T{}), "ordering must be noexcept");
  static_assert(T{} == T{}, "default values must compare equal");
  static_assert(!(T{} < T{}), "ordering must be irreflexive");
#if defined(__cpp_lib_three_way_comparison) && \
    __cpp_lib_three_way_comparison >= 201907L
  static_assert(noexcept(T{} <=> T{}), "three-way comparison must be noexcept");
  static_assert(std::is_same<decltype(T{} <=> T{}),
                             std::strong_ordering>::value,
                "integral representations must have a strong ordering");
  static_assert((T{} <=> T{}) == std::strong_ordering::equal,
                "default values must compare equal");
#endif

  auto keys = std::map<T, size_t>();
  auto integers = std::map<uint64_t, size_t>();
  for (size_t i = 0; i < count; ++i) {
    keys[values[i]] = i;
    integers[static_cast<uint64_t>(fmt::to_underlying(values[i]))] = i;
    for (size_t j = 0; j < count; ++j) {
      auto lhs = fmt::to_underlying(values[i]);
      auto rhs = fmt::to_underlying(values[j]);
      EXPECT_EQ(values[i] == values[j], lhs == rhs);
      EXPECT_EQ(values[i] != values[j], lhs != rhs);
      EXPECT_EQ(values[i] < values[j], lhs < rhs);
      EXPECT_EQ(values[i] <= values[j], lhs <= rhs);
      EXPECT_EQ(values[i] > values[j], lhs > rhs);
      EXPECT_EQ(values[i] >= values[j], lhs >= rhs);
#if defined(__cpp_lib_three_way_comparison) && \
    __cpp_lib_three_way_comparison >= 201907L
      EXPECT_EQ(values[i] <=> values[j], lhs <=> rhs);
#endif
    }
  }
  EXPECT_EQ(keys.size(), integers.size());
  auto expected = integers.begin();
  for (const auto& entry : keys) {
    ASSERT_NE(expected, integers.end());
    EXPECT_EQ(fmt::to_underlying(entry.first), expected->first);
    EXPECT_EQ(entry.second, expected->second);
    ++expected;
  }
}

TEST(color_test, rgb_comparisons) {
  constexpr auto low = fmt::rgb{0, 255, 255};
  constexpr auto high = fmt::rgb{1, 0, 0};
  static_assert(low < high, "red precedes green and blue in packed ordering");
  static_assert(fmt::rgb{0xAB123456} == fmt::rgb{0x123456},
                "the discarded high byte must not affect equality");
  const fmt::rgb values[] = {
      {}, {0, 0, 1}, {0, 1, 0}, {1, 0, 0}, {255, 0, 0},
      {0, 255, 255}, {255, 255, 255},
      fmt::rgb{0xAB123456}, fmt::rgb{0x123456}};
  check_color_comparisons(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, gray_color) {
  using fmt::gray_color;
  static_assert(std::is_same<gray_color::value_type, uint8_t>::value &&
                    std::is_same<fmt::underlying_type<gray_color>::type,
                                 uint8_t>::value,
                "grayscale indices use uint8_t");
  static_assert(std::is_same<decltype(fmt::to_underlying(gray_color{})),
                             uint8_t>::value,
                "numeric access returns the zero-based index");
  static_assert(std::is_convertible<gray_color, fmt::rgb>::value &&
                    !std::is_convertible<int, gray_color>::value,
                "RGB conversion is implicit; index construction is explicit");
  static_assert(noexcept(static_cast<fmt::rgb>(gray_color{})),
                "RGB conversion must be noexcept");
  constexpr gray_color first{0}, last{23};
  static_assert(fmt::to_underlying(first) == 0 &&
                    fmt::to_underlying(last) == 23,
                "boundary indices must support C++11 constant evaluation");
  constexpr fmt::rgb dark = first, light = last;
  static_assert(dark == fmt::rgb{8, 8, 8} && light == fmt::rgb{238, 238, 238},
                "grayscale conversion supports C++11 constant evaluation");
  constexpr fmt::rgb middle = gray_color{12};
  static_assert(middle == fmt::rgb{128, 128, 128},
                "a temporary gray color supports C++11 RGB conversion");
  EXPECT_EQ(fmt::to_underlying(gray_color{}), 0);
  const int indices[] = {0, 1, 12, 22, 23};
  const unsigned levels[] = {8, 18, 128, 228, 238};
  for (size_t i = 0; i != sizeof(indices) / sizeof(indices[0]); ++i) {
    SCOPED_TRACE(indices[i]);
    gray_color color{indices[i]};
    fmt::rgb converted = color;
    EXPECT_EQ(fmt::to_underlying(color), indices[i]);
    EXPECT_EQ(converted.r, levels[i]);
    EXPECT_EQ(converted.g, levels[i]);
    EXPECT_EQ(converted.b, levels[i]);
    auto copy = color;
    gray_color assigned;
    assigned = color;
    EXPECT_EQ(copy, color);
    EXPECT_EQ(assigned, color);
  }
  static_assert(noexcept(gray_color{0}), "gray construction must be noexcept");
#ifdef NDEBUG
  const int invalid[] = {-1, 24, 255, 256,
                         (std::numeric_limits<int>::min)(),
                         (std::numeric_limits<int>::max)()};
  for (auto index : invalid) {
    SCOPED_TRACE(index);
    EXPECT_EQ(gray_color{index}, gray_color{index < 0 ? 0 : 23});
  }
#endif
  const gray_color ordered[] = {gray_color{0}, gray_color{1}, gray_color{23}};
  check_color_comparisons(ordered, sizeof(ordered) / sizeof(ordered[0]));
}

TEST(color_test, cube_color) {
  using fmt::cube_color;
  static_assert(std::is_same<cube_color::value_type, uint8_t>::value &&
                    std::is_same<fmt::underlying_type<cube_color>::type,
                                 uint8_t>::value,
                "cube indices use uint8_t");
  static_assert(std::is_same<decltype(fmt::to_underlying(cube_color{})),
                             uint8_t>::value,
                "numeric access returns the zero-based index");
  static_assert(std::is_convertible<cube_color, fmt::rgb>::value &&
                    !std::is_convertible<unsigned, cube_color>::value,
                "RGB conversion is implicit; index construction is explicit");
  static_assert(std::is_same<decltype(cube_color{}.r()), int>::value &&
                    std::is_same<decltype(cube_color{}.g()), int>::value &&
                    std::is_same<decltype(cube_color{}.b()), int>::value,
                "cube channel getters return int");
  static_assert(noexcept(static_cast<fmt::rgb>(cube_color{})),
                "RGB conversion must be noexcept");
  constexpr cube_color first{0}, last{215}, middle{1, 2, 3};
  constexpr fmt::rgb converted = middle;
  static_assert(fmt::to_underlying(first) == 0 &&
                    fmt::to_underlying(last) == 215 && last.r() == 5 &&
                    last.g() == 5 && last.b() == 5,
                "cube index decoding supports C++11 constant evaluation");
  static_assert(converted == fmt::rgb{95, 135, 175} &&
                    fmt::to_underlying(middle) == 51,
                "cube encoding and RGB conversion support C++11");
  EXPECT_EQ(fmt::to_underlying(cube_color{}), 0);
  static_assert(noexcept(cube_color{0}) && noexcept(cube_color{0, 0, 0}),
                "cube construction must be noexcept");
  static_assert(noexcept(std::declval<cube_color&>().r(0)) &&
                    noexcept(std::declval<cube_color&>().g(0)) &&
                    noexcept(std::declval<cube_color&>().b(0)),
                "cube setters must be noexcept");
#ifdef NDEBUG
  const unsigned invalid_indices[] = {216, 217, 251, 255, 256,
                                     (std::numeric_limits<unsigned>::max)()};
  for (auto index : invalid_indices) {
    SCOPED_TRACE(index);
    EXPECT_EQ(cube_color{index}, cube_color{215});
  }
  const int invalid_channels[] = {-1, 6, 255, 256,
                                 (std::numeric_limits<int>::min)(),
                                 (std::numeric_limits<int>::max)()};
  for (auto channel : invalid_channels) {
    SCOPED_TRACE(channel);
    const int clamped = channel < 0 ? 0 : 5;
    EXPECT_EQ((cube_color{channel, 2, 3}), (cube_color{clamped, 2, 3}));
    EXPECT_EQ((cube_color{1, channel, 3}), (cube_color{1, clamped, 3}));
    EXPECT_EQ((cube_color{1, 2, channel}), (cube_color{1, 2, clamped}));
    auto color = middle;
    EXPECT_EQ(&color.r(channel), &color);
    EXPECT_EQ(color, (cube_color{clamped, 2, 3}));
    color = middle;
    EXPECT_EQ(&color.g(channel), &color);
    EXPECT_EQ(color, (cube_color{1, clamped, 3}));
    color = middle;
    EXPECT_EQ(&color.b(channel), &color);
    EXPECT_EQ(color, (cube_color{1, 2, clamped}));
  }
#endif
  auto color = middle;
  EXPECT_EQ(&color.r(5), &color);
  EXPECT_EQ(color, (cube_color{5, 2, 3}));
  EXPECT_EQ(&color.g(0), &color);
  EXPECT_EQ(color, (cube_color{5, 0, 3}));
  EXPECT_EQ(&color.b(4), &color);
  EXPECT_EQ(color, (cube_color{5, 0, 4}));
  cube_color assigned;
  assigned = color;
  EXPECT_EQ(assigned, color);
  const cube_color ordered[] = {cube_color{0}, cube_color{1}, cube_color{5},
                                cube_color{6}, cube_color{35}, cube_color{36},
                                cube_color{215}};
  check_color_comparisons(ordered, sizeof(ordered) / sizeof(ordered[0]));
}

TEST(color_test, cube_color_xterm_rgb) {
  // Independent truth source: Xterm FAQ, "Can I set a color by its number?"
  // https://invisible-island.net/xterm/xterm.faq.html#color_by_number
  // Use its arithmetic formula, never the implementation's cube_levels table.
  // Our zero-based index omits the SGR palette's offset of 16.
  for (int red = 0; red != 6; ++red) {
    for (int green = 0; green != 6; ++green) {
      for (int blue = 0; blue != 6; ++blue) {
        unsigned index = static_cast<unsigned>(36 * red + 6 * green + blue);
        SCOPED_TRACE(index);
        fmt::cube_color color{index};
        EXPECT_EQ(color.r(), red);
        EXPECT_EQ(color.g(), green);
        EXPECT_EQ(color.b(), blue);
        EXPECT_EQ(fmt::to_underlying(color), index);
        EXPECT_EQ(color, (fmt::cube_color{red, green, blue}));
        fmt::rgb converted = color;
        EXPECT_EQ(converted.r, red == 0 ? 0 : 55 + 40 * red);
        EXPECT_EQ(converted.g, green == 0 ? 0 : 55 + 40 * green);
        EXPECT_EQ(converted.b, blue == 0 ? 0 : 55 + 40 * blue);
      }
    }
  }
}

TEST(color_test, terminal_color_xterm_rgb) {
  // Agreed Xterm-default palette, independent of the implementation table.
  const fmt::rgb expected[] = {
      {0, 0, 0}, {205, 0, 0}, {0, 205, 0}, {205, 205, 0},
      {0, 0, 238}, {205, 0, 205}, {0, 205, 205}, {229, 229, 229},
      {127, 127, 127}, {255, 0, 0}, {0, 255, 0}, {255, 255, 0},
      {92, 92, 255}, {255, 0, 255}, {0, 255, 255}, {255, 255, 255}};
  const fmt::rgb named[] = {
      fmt::color::term_black, fmt::color::term_red, fmt::color::term_green,
      fmt::color::term_yellow, fmt::color::term_blue, fmt::color::term_magenta,
      fmt::color::term_cyan, fmt::color::term_white,
      fmt::color::term_bright_black, fmt::color::term_bright_red,
      fmt::color::term_bright_green, fmt::color::term_bright_yellow,
      fmt::color::term_bright_blue, fmt::color::term_bright_magenta,
      fmt::color::term_bright_cyan, fmt::color::term_bright_white};
  static_assert(std::is_convertible<fmt::term_color, fmt::rgb>::value,
                "terminal-to-RGB conversion must be implicit");
  static_assert(std::is_same<std::underlying_type<fmt::term_color>::type,
                             int8_t>::value,
                "terminal enum storage must be signed");
  static_assert(fmt::to_underlying(static_cast<fmt::term_color>(-1)) == -1,
                "underlying conversion must preserve signed enum values");
  constexpr fmt::rgb black = fmt::term_color::black;
  constexpr fmt::rgb bright = fmt::term_color::bright_white;
  static_assert(black == fmt::rgb{} && bright == fmt::rgb{255, 255, 255},
                "terminal conversion supports C++11 constant evaluation");
  for (unsigned index = 0; index != 16; ++index) {
    SCOPED_TRACE(index);
    fmt::rgb converted = static_cast<fmt::term_color>(index);
    EXPECT_EQ(converted, expected[index]);
    EXPECT_EQ(named[index], expected[index]);
  }
}

TEST(color_test, color_type_comparisons) {
  using fmt::color_t;
  constexpr auto low = fmt::detail::unpack_color(uint32_t{1} << 24);
  constexpr auto high = fmt::detail::unpack_color(uint32_t{2} << 24);
  static_assert(color_t{} < low, "unspecified differs from RGB black");
  static_assert(low < high, "RGB black differs from terminal black");
  const color_t values[] = {
      {}, fmt::no_color, fmt::rgb{}, fmt::term_color::black,
      fmt::rgb{0xFFFFFF}, fmt::term_color::bright_white,
      fmt::rgb{0x123456}, fmt::color::coral, fmt::rgb{0xFF7F50},
      fmt::cube_color{0}, fmt::cube_color{215}, fmt::gray_color{0},
      fmt::gray_color{23}};
  check_color_comparisons(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, text_style_comparisons) {
  constexpr auto low = fmt::text_style{};
  constexpr auto high = fmt::text_style{fmt::emphasis::bold};
  static_assert(low < high, "an empty style sorts before bold");
  const fmt::text_style values[] = {
      {}, fmt::fg(fmt::no_color), fmt::fg(fmt::rgb{}),
      fmt::fg(fmt::term_color::black), fmt::bg(fmt::rgb{}),
      fmt::bg(fmt::term_color::black), fmt::fg(fmt::rgb{0xFFFFFF}),
      fmt::emphasis::bold, fmt::emphasis::strike,
      fmt::fg(fmt::color::coral) | fmt::bg(fmt::term_color::bright_white) |
          fmt::emphasis::bold | fmt::emphasis::strike,
      fmt::fg(fmt::cube_color{0}), fmt::bg(fmt::cube_color{215}),
      fmt::fg(fmt::gray_color{0}), fmt::bg(fmt::gray_color{23})};
  check_color_comparisons(values, sizeof(values) / sizeof(values[0]));
}

template<typename T>
void check_color_hashes(const T* values, size_t count) {
  using value_type = typename fmt::underlying_type<T>::type;
  static_assert(noexcept(std::hash<T>{}(T{})), "hashing must be noexcept");
  static_assert(std::is_same<decltype(std::hash<T>{}(T{})), size_t>::value,
                "hashing must return size_t");
  auto hasher = std::hash<T>();
  auto integer_hasher = std::hash<value_type>();
  auto keys = std::unordered_set<T>();
  auto entries = std::unordered_map<T, size_t>();
  auto integers = std::map<value_type, size_t>();
  for (size_t i = 0; i < count; ++i) {
    EXPECT_EQ(hasher(values[i]), integer_hasher(fmt::to_underlying(values[i])));
    keys.insert(values[i]);
    entries[values[i]] = i;
    integers[fmt::to_underlying(values[i])] = i;
    for (size_t j = 0; j < count; ++j) {
      if (values[i] == values[j]) {
        EXPECT_EQ(hasher(values[i]), hasher(values[j]));
      }
    }
  }
  EXPECT_EQ(keys.size(), integers.size());
  EXPECT_EQ(entries.size(), integers.size());
  for (size_t i = 0; i < count; ++i) {
    EXPECT_EQ(keys.count(values[i]), size_t{1});
    EXPECT_EQ(entries.at(values[i]), integers.at(fmt::to_underlying(values[i])));
  }
}

TEST(color_test, rgb_hash) {
  const fmt::rgb values[] = {
      {}, {0, 0, 0}, {0, 0, 1}, {0, 1, 0}, {1, 0, 0}, {255, 255, 255},
      fmt::rgb{0xAB123456}, fmt::rgb{0x123456},
      fmt::color::coral, fmt::rgb{0xFF7F50}};
  check_color_hashes(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, color_type_hash) {
  using fmt::color_t;
  const color_t values[] = {
      {}, fmt::no_color, fmt::rgb{}, fmt::term_color::black,
      fmt::rgb{0xFFFFFF}, fmt::term_color::bright_white,
      fmt::color::coral, fmt::rgb{0xFF7F50}, fmt::cube_color{0},
      fmt::cube_color{215}, fmt::gray_color{0}, fmt::gray_color{23}};
  check_color_hashes(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, text_style_hash) {
  const fmt::text_style values[] = {
      {}, fmt::fg(fmt::no_color), fmt::fg(fmt::rgb{}),
      fmt::fg(fmt::term_color::black), fmt::bg(fmt::rgb{}),
      fmt::bg(fmt::term_color::black), fmt::emphasis::bold,
      fmt::emphasis::strike, fmt::fg(fmt::color::coral),
      fmt::fg(fmt::rgb{0xFF7F50}),
      fmt::fg(fmt::color::coral) | fmt::bg(fmt::term_color::bright_white) |
          fmt::emphasis::bold | fmt::emphasis::strike,
      fmt::fg(fmt::cube_color{0}), fmt::bg(fmt::cube_color{215}),
      fmt::fg(fmt::gray_color{0}), fmt::bg(fmt::gray_color{23})};
  check_color_hashes(values, sizeof(values) / sizeof(values[0]));
}

TEST(color_test, emphasis_or_assign) {
  using fmt::emphasis;
  static_assert(std::is_same<decltype(std::declval<emphasis&>() |=
                                     emphasis::bold), emphasis&>::value,
                "compound assignment must return an emphasis reference");
  static_assert(noexcept(std::declval<emphasis&>() |= emphasis::bold),
                "compound assignment must be noexcept");
  for (unsigned lhs = 0; lhs < 256; ++lhs) {
    for (unsigned rhs = 0; rhs < 256; ++rhs) {
      auto value = static_cast<emphasis>(lhs);
      const auto other = static_cast<emphasis>(rhs);
      auto& result = (value |= other);
      EXPECT_EQ(fmt::to_underlying(value), lhs | rhs);
      EXPECT_EQ(&result, &value);
      EXPECT_EQ(fmt::to_underlying(other), rhs);
    }
  }
  auto value = emphasis::bold;
  (value |= emphasis::underline) |= emphasis::strike;
  EXPECT_EQ(value, emphasis::bold | emphasis::underline | emphasis::strike);
  fmt::operator|=(value, value);
  EXPECT_EQ(value, emphasis::bold | emphasis::underline | emphasis::strike);
}

#if FMT_USE_CONSTEXPR
constexpr auto check_emphasis_or_assign() -> bool {
  auto value = fmt::emphasis::bold;
  auto& result = (value |= fmt::emphasis::underline);
  (result |= fmt::emphasis::strike) |= fmt::emphasis{};
  fmt::operator|=(value, value);
  return &result == &value &&
         value == (fmt::emphasis::bold | fmt::emphasis::underline |
                   fmt::emphasis::strike);
}

TEST(color_test, emphasis_or_assign_constexpr) {
  static_assert(check_emphasis_or_assign(),
                "compound assignment must support constant evaluation");
}
#endif

TEST(color_test, underlying) {
  static_assert(std::is_same<fmt::underlying_type<fmt::term_color>::type,
                             int8_t>::value, "terminal color underlying type");
  static_assert(std::is_same<fmt::underlying_type<fmt::emphasis>::type,
                             uint8_t>::value, "emphasis underlying type");
  static_assert(std::is_same<fmt::underlying_type<fmt::rgb>::type,
                             uint32_t>::value, "RGB underlying type");
  static_assert(std::is_same<fmt::underlying_type<fmt::text_style>::type,
                             uint64_t>::value, "text style underlying type");

  static_assert(std::is_same<decltype(fmt::to_underlying(
                                 fmt::term_color::black)),
                             int8_t>::value, "terminal color return type");
  static_assert(std::is_same<decltype(fmt::to_underlying(fmt::emphasis::bold)),
                             uint8_t>::value, "emphasis return type");
  static_assert(std::is_same<decltype(fmt::to_underlying(fmt::rgb{})),
                             uint32_t>::value, "RGB return type");
  static_assert(std::is_same<decltype(fmt::to_underlying(fmt::text_style{})),
                             uint64_t>::value, "text style return type");

  static_assert(noexcept(fmt::to_underlying(fmt::term_color::black)),
                "terminal color conversion must be noexcept");
  static_assert(noexcept(fmt::to_underlying(fmt::emphasis::bold)),
                "emphasis conversion must be noexcept");
  static_assert(noexcept(fmt::to_underlying(fmt::rgb{})),
                "RGB conversion must be noexcept");
  static_assert(noexcept(fmt::to_underlying(fmt::text_style{})),
                "text style conversion must be noexcept");

  static_assert(fmt::to_underlying(fmt::term_color::black) ==
                    static_cast<uint8_t>(fmt::term_color::black),
                "terminal color conversion must preserve the enum value");
  static_assert(fmt::to_underlying(fmt::term_color::bright_white) ==
                    static_cast<uint8_t>(fmt::term_color::bright_white),
                "bright terminal color conversion must preserve the enum value");
  static_assert(fmt::to_underlying(fmt::emphasis::bold) == 1,
                "bold must retain its bit");
  static_assert(fmt::to_underlying(fmt::emphasis::strike) == 128,
                "strike must retain its bit");
  static_assert(fmt::to_underlying(fmt::rgb{0xAB123456}) == 0x123456,
                "packed RGB must have a zero high byte");
  static_assert(fmt::to_underlying(fmt::rgb{}) == 0,
                "default RGB must be black");
  static_assert(fmt::to_underlying(fmt::text_style{}) == 0,
                "default text style must be empty");
  static_assert(fmt::to_underlying(
                    fmt::text_style{fmt::emphasis::strike}) ==
                    (uint64_t{128} << 54),
                "text style conversion must preserve high emphasis bits");

  auto foreground = fmt::fg(fmt::rgb{0x123456});
  auto background = fmt::bg(fmt::term_color::black);
  auto attributes = fmt::text_style{fmt::emphasis::bold |
                                    fmt::emphasis::strike};
  auto style = foreground | background | attributes;
  EXPECT_EQ(fmt::to_underlying(style),
            fmt::to_underlying(foreground) | fmt::to_underlying(background) |
                fmt::to_underlying(attributes));
  EXPECT_NE(fmt::to_underlying(style),
            fmt::to_underlying(background | attributes));
  EXPECT_NE(fmt::to_underlying(style),
            fmt::to_underlying(foreground | attributes));
  EXPECT_NE(fmt::to_underlying(style),
            fmt::to_underlying(foreground | background));
}

TEST(color_test, color_type_underlying) {
  using fmt::color_t;
  static_assert(std::is_same<fmt::underlying_type<color_t>::type,
                             uint32_t>::value, "packed color underlying type");
  static_assert(std::is_same<decltype(fmt::to_underlying(color_t{})),
                             uint32_t>::value, "packed color return type");
  static_assert(noexcept(fmt::to_underlying(color_t{})),
                "packed color conversion must be noexcept");
  static_assert(fmt::to_underlying(color_t{}) == 0,
                "default packed color must be empty");

  // Check the internal packed representation against independent expected values.
  const color_t colors[] = {
      color_t{}, color_t{fmt::rgb{0}},
      color_t{fmt::rgb{0x123456}},
      color_t{fmt::term_color::black}, color_t{fmt::cube_color{215}},
      color_t{fmt::gray_color{23}}};
  const uint32_t expected[] = {
      0u, (uint32_t{1} << 24),
      (uint32_t{1} << 24) | 0x123456u, (uint32_t{2} << 24),
      (uint32_t{3} << 24) | 215u, (uint32_t{4} << 24) | 23u};
  for (size_t i = 0; i < sizeof(colors) / sizeof(colors[0]); ++i)
    EXPECT_EQ(fmt::to_underlying(colors[i]), expected[i]);
}

namespace {
struct composition_sample {
  fmt::color_t color;
  fmt::rgb promoted;
};

const composition_sample composition_samples[] = {
    {fmt::no_color, {0, 0, 0}},
    {fmt::rgb{1, 2, 3}, {1, 2, 3}},
    {fmt::term_color::red, {205, 0, 0}},
    {fmt::cube_color{1, 2, 3}, {95, 135, 175}},
    {fmt::gray_color{12}, {128, 128, 128}}};

// Expected RGB values above are independent of the conversion implementation.
fmt::color_t expected_composition(size_t left, size_t right) {
  if (left == 0) return composition_samples[right].color;
  if (right == 0) return composition_samples[left].color;
  auto lhs = composition_samples[left].promoted;
  auto rhs = composition_samples[right].promoted;
  return fmt::rgb{static_cast<uint8_t>(lhs.r | rhs.r),
                 static_cast<uint8_t>(lhs.g | rhs.g),
                 static_cast<uint8_t>(lhs.b | rhs.b)};
}
}  // namespace

TEST(color_test, color_type_composition) {
  static_assert(noexcept(fmt::color_t{} | fmt::color_t{}) &&
                    noexcept(std::declval<fmt::color_t&>() |= fmt::color_t{}),
                "color composition must be noexcept");
  for (size_t i = 0; i != 5; ++i) {
    for (size_t j = 0; j != 5; ++j) {
      SCOPED_TRACE(fmt::format("color kinds {} and {}", i, j));
      auto lhs = composition_samples[i].color;
      auto rhs = composition_samples[j].color;
      const auto expected = expected_composition(i, j);
      EXPECT_EQ(lhs | rhs, expected);
      EXPECT_EQ(lhs, composition_samples[i].color);
      EXPECT_EQ(&(lhs |= rhs), &lhs);
      EXPECT_EQ(lhs, expected);
      EXPECT_EQ(rhs, composition_samples[j].color);
    }
    auto self = composition_samples[i].color;
    EXPECT_EQ(&(self |= self), &self);
    EXPECT_EQ(self, expected_composition(i, i));
  }
  auto lhs = fmt::color_t{fmt::rgb{0xC0F000}};
  auto rhs = fmt::color_t{fmt::rgb{0x000FEE}};
  EXPECT_EQ(lhs | rhs, fmt::color_t{fmt::rgb{0xC0FFEE}});
  EXPECT_EQ(lhs, fmt::color_t{fmt::rgb{0xC0F000}});
  EXPECT_EQ(rhs, fmt::color_t{fmt::rgb{0x000FEE}});
}

TEST(color_test, color_sentinels) {
  using fmt::no_color_t;
  static_assert(std::is_empty<no_color_t>::value,
                "the unspecified sentinel has no payload");
  static_assert(std::is_default_constructible<no_color_t>::value,
                "the unspecified sentinel must be constructible");
  // GCC 4.9's standard library does not provide this trait.
#if !FMT_GCC_VERSION || FMT_GCC_VERSION >= 500
  static_assert(std::is_trivially_copyable<fmt::color_t>::value,
                "alternative replacement relies on trivial union copying");
#endif
  static_assert(std::is_same<fmt::underlying_type<no_color_t>::type,
                             unsigned>::value,
                "the sentinel has an unsigned numeric representation");
  static_assert(fmt::to_underlying(fmt::no_color) == 0,
                "the unspecified sentinel has zero payload");
  static_assert(noexcept(fmt::to_underlying(fmt::no_color)),
                "sentinel conversion must be noexcept");
}

TEST(color_test, text_style_empty_and_clear) {
  constexpr auto empty = fmt::text_style{};
  static_assert(empty.empty(), "default style must be empty");
  static_assert(!fmt::text_style{fmt::rgb{}}.empty(),
                "specified RGB black must not be empty");
  static_assert(!fmt::text_style{fmt::term_color::black}.empty(),
                "specified terminal black must not be empty");
  static_assert(!fmt::text_style{fmt::emphasis::strike}.empty(),
                "emphasis alone must not be empty");
  static_assert(noexcept(empty.empty()) &&
                    noexcept(std::declval<fmt::text_style&>().clear()),
                "empty and clear must be noexcept");
  static_assert(std::is_same<decltype(empty.empty()), bool>::value &&
                    std::is_same<decltype(std::declval<fmt::text_style&>().clear()),
                                 fmt::text_style&>::value,
                "empty returns bool and clear returns a style reference");

  const fmt::color_t colors[] = {fmt::no_color, fmt::rgb{},
      fmt::term_color::black, fmt::cube_color{0}, fmt::gray_color{0}};
  const fmt::emphasis attributes[] = {
      fmt::emphasis{}, fmt::emphasis::bold, fmt::emphasis::strike,
      static_cast<fmt::emphasis>(255)};
  for (size_t fg = 0; fg < 5; ++fg) {
    for (size_t bg = 0; bg < 5; ++bg) {
      for (auto attribute : attributes) {
        auto style = fmt::text_style{colors[fg], colors[bg], attribute};
        EXPECT_EQ(style.empty(),
                  fg == 0 && bg == 0 && attribute == fmt::emphasis{});
        EXPECT_EQ(&style.clear(), &style);
        EXPECT_TRUE(style.empty());
        EXPECT_EQ(style, fmt::text_style{});
        EXPECT_FALSE(style.has_fg());
        EXPECT_FALSE(style.has_bg());
        EXPECT_FALSE(style.has_emph());
        EXPECT_EQ(fmt::format(style, "plain"), "plain");
        EXPECT_EQ(&style.clear(), &style);
        EXPECT_TRUE(style.empty());
        style.clear().fg(fmt::gray_color{12}).underline();
        EXPECT_EQ(style, (fmt::text_style{fmt::gray_color{12}, fmt::no_color,
                                         fmt::emphasis::underline}));
      }
    }
  }
}

TEST(color_test, text_style_overlay) {
  using fmt::emphasis;
  using fmt::text_style;
  static_assert(noexcept(std::declval<text_style&>().overlay(
                    std::declval<const text_style&>())),
                "overlay must be noexcept");
  static_assert(std::is_same<decltype(std::declval<text_style&>().overlay(
                                std::declval<const text_style&>())),
                             text_style&>::value,
                "overlay must return a style reference");
  static_assert(noexcept(fmt::overlay(text_style{}, text_style{})),
                "free overlay must be noexcept");
  static_assert(std::is_same<decltype(fmt::overlay(text_style{}, text_style{})),
                             text_style>::value,
                "free overlay must return a style value");

  const fmt::color_t base_colors[] = {
      fmt::no_color, fmt::rgb{200, 10, 30}, fmt::term_color::red,
      fmt::cube_color{5, 4, 3}, fmt::gray_color{23}};
  const fmt::color_t overlay_colors[] = {
      fmt::no_color, fmt::rgb{}, fmt::term_color::black,
      fmt::cube_color{0}, fmt::gray_color{0}};
  for (auto base_fg : base_colors) {
    for (auto base_bg : base_colors) {
      const auto base = text_style{base_fg, base_bg, emphasis::bold};
      for (size_t fg = 0; fg < 5; ++fg) {
        for (size_t bg = 0; bg < 5; ++bg) {
          const auto other = text_style{overlay_colors[fg], overlay_colors[bg],
                                       emphasis::faint};
          const auto saved = other;
          auto input = base;
          const auto combined = fmt::overlay(input, other);
          EXPECT_EQ(combined,
                    (text_style{fg == 0 ? base_fg : overlay_colors[fg],
                                bg == 0 ? base_bg : overlay_colors[bg],
                                emphasis::bold | emphasis::faint}));
          EXPECT_EQ(input, base);
          EXPECT_EQ(other, saved);
          auto style = base;
          EXPECT_EQ(&style.overlay(other), &style);
          EXPECT_EQ(style, (text_style{fg == 0 ? base_fg : overlay_colors[fg],
                                      bg == 0 ? base_bg : overlay_colors[bg],
                                      emphasis::bold | emphasis::faint}));
          EXPECT_EQ(other, saved);
        }
      }
      auto style = base;
      EXPECT_EQ(fmt::overlay(base, text_style{}), base);
      EXPECT_EQ(fmt::overlay(text_style{}, base), base);
      EXPECT_EQ(fmt::overlay(base, base), base);
      EXPECT_EQ(&style.overlay(text_style{}), &style);
      EXPECT_EQ(style, base);
      EXPECT_EQ(&style.overlay(style), &style);
      EXPECT_EQ(style, base);
      EXPECT_EQ(&style.overlay(base), &style);
      EXPECT_EQ(style, base);
      auto empty = text_style{};
      EXPECT_EQ(&empty.overlay(base), &empty);
      EXPECT_EQ(empty, base);
    }
  }
  auto empty = text_style{};
  EXPECT_EQ(fmt::overlay(empty, empty), empty);
  EXPECT_EQ(&empty.overlay(empty), &empty);
  EXPECT_TRUE(empty.empty());
  auto style = text_style{fmt::term_color::red, fmt::gray_color{12},
                          emphasis::bold};
  style.overlay(text_style{fmt::rgb{}}).underline();
  EXPECT_EQ(style, (text_style{fmt::rgb{}, fmt::gray_color{12},
                              emphasis::bold | emphasis::underline}));
  const auto first = text_style{fmt::term_color::red};
  const auto second = text_style{fmt::term_color::blue};
  EXPECT_EQ(fmt::overlay(first, second), second);
  EXPECT_EQ(fmt::overlay(second, first), first);
}

#if FMT_USE_CONSTEXPR
TEST(color_test, text_style_overlay_constexpr) {
  constexpr auto base = fmt::text_style{fmt::term_color::red,
                                       fmt::gray_color{12}, fmt::emphasis::bold};
  constexpr auto overlaid = fmt::text_style{base}.overlay(
      fmt::text_style{fmt::rgb{}, fmt::no_color, fmt::emphasis::faint});
  static_assert(overlaid == fmt::text_style{
                                fmt::rgb{}, fmt::gray_color{12},
                                fmt::emphasis::bold | fmt::emphasis::faint},
                "overlay must support constant evaluation");
  constexpr auto unchanged = fmt::text_style{base}.overlay(fmt::text_style{});
  static_assert(unchanged == base, "an empty overlay must preserve the style");
  constexpr auto copied = fmt::text_style{}.overlay(base);
  static_assert(copied == base, "overlay must replace an empty base");
  constexpr auto equal = fmt::text_style{base}.overlay(base);
  static_assert(equal == base, "an equal overlay must preserve the style");
  constexpr auto overrides = fmt::text_style{fmt::rgb{}, fmt::no_color,
                                            fmt::emphasis::faint};
  constexpr auto combined = fmt::overlay(base, overrides);
  static_assert(combined == overlaid,
                "free overlay must support constant evaluation");
  static_assert(fmt::overlay(base, fmt::text_style{}) == base &&
                    fmt::overlay(fmt::text_style{}, base) == base &&
                    fmt::overlay(base, base) == base,
                "free overlay must preserve empty and equal cases");
  static_assert(base.fg() == fmt::term_color::red &&
                    overrides.fg() == fmt::rgb{},
                "free overlay must preserve its inputs");
}
#endif

TEST(color_test, text_style_color_composition) {
  static_assert(noexcept(fmt::text_style{} | fmt::text_style{}) &&
                    noexcept(std::declval<fmt::text_style&>() |=
                             fmt::text_style{}),
                "style composition must be noexcept");
  for (size_t i = 0; i != 5; ++i) {
    for (size_t j = 0; j != 5; ++j) {
      SCOPED_TRACE(fmt::format("style color kinds {} and {}", i, j));
      const auto left = fmt::text_style{composition_samples[i].color,
                                       composition_samples[j].color,
                                       fmt::emphasis::bold};
      const auto right = fmt::text_style{composition_samples[j].color,
                                        composition_samples[i].color,
                                        fmt::emphasis::strike};
      const auto expected = fmt::text_style{
          expected_composition(i, j), expected_composition(j, i),
          fmt::emphasis::bold | fmt::emphasis::strike};
      EXPECT_EQ(left | right, expected);
      auto assigned = left;
      EXPECT_EQ(&(assigned |= right), &assigned);
      EXPECT_EQ(assigned, expected);
      EXPECT_EQ(left.fg(), composition_samples[i].color);
      EXPECT_EQ(right.bg(), composition_samples[i].color);
      auto self = left;
      self.operator|=(self);
      EXPECT_EQ(self, (fmt::text_style{expected_composition(i, i),
                                      expected_composition(j, j),
                                      fmt::emphasis::bold}));
    }
  }
}

TEST(color_test, palette_style_replacement) {
  const fmt::color_t replacements[] = {
      fmt::gray_color{23}, fmt::cube_color{215}, fmt::term_color::black,
      fmt::rgb{}, fmt::no_color};
  auto style = fmt::text_style{fmt::gray_color{0}, fmt::gray_color{23},
                              static_cast<fmt::emphasis>(0xFF)};
  for (auto replacement : replacements) {
    const auto background = style.bg();
    EXPECT_EQ(&style.fg(replacement), &style);
    EXPECT_EQ(style.fg(), replacement);
    EXPECT_EQ(style.bg(), background);
    EXPECT_EQ(style.has_fg(), replacement.index() != 0);
    EXPECT_EQ(fmt::to_underlying(style.emph()), 0xFF);
  }
  style.fg(fmt::gray_color{23});
  for (auto replacement : replacements) {
    EXPECT_EQ(&style.bg(replacement), &style);
    EXPECT_EQ(style.bg(), replacement);
    EXPECT_EQ(style.fg(), fmt::color_t{fmt::gray_color{23}});
    EXPECT_EQ(style.has_bg(), replacement.index() != 0);
    EXPECT_EQ(fmt::to_underlying(style.emph()), 0xFF);
  }
  style.fg(fmt::no_color).emph(fmt::emphasis{});
  EXPECT_EQ(style, fmt::text_style{});
}

namespace {
void check_to_sgr(const fmt::text_style& style, const std::string& expected) {
  static_assert(std::is_same<decltype(fmt::to_sgr(style)), std::string>::value,
                "the default character type must be char");
  static_assert(std::is_same<decltype(fmt::to_sgr<char>(style)),
                             std::string>::value,
                "explicit char rendering must return std::string");
  static_assert(std::is_same<decltype(fmt::to_sgr<wchar_t>(style)),
                             std::wstring>::value,
                "wide rendering must return std::wstring");
  const auto original = style;
  EXPECT_EQ(fmt::to_sgr(style), expected);
  EXPECT_EQ(fmt::to_sgr<char>(style), expected);
  EXPECT_EQ(fmt::to_sgr<wchar_t>(style),
            (std::wstring{expected.begin(), expected.end()}));
  EXPECT_EQ(style, original);
}

template<typename Char>
std::basic_string<Char> color_escape(fmt::color_t color, bool background) {
  const auto escape = fmt::detail::ansi_color_escape<Char>{color, background};
  return std::basic_string<Char>{escape.begin(), escape.end()};
}
}  // namespace

namespace {
void check_sgr_transition(const fmt::text_style& target,
                          const fmt::text_style& initial,
                          const std::string& expected) {
  const auto target_copy = target;
  const auto initial_copy = initial;
  EXPECT_EQ(fmt::to_sgr(target, initial), expected);
  EXPECT_EQ(fmt::to_sgr<char>(target, initial), expected);
  EXPECT_EQ(fmt::to_sgr<wchar_t>(target, initial),
            (std::wstring{expected.begin(), expected.end()}));
  // The initial value describes a complete state regardless of its flag.
  auto absolute_initial = initial;
  absolute_initial.absolute();
  EXPECT_EQ(fmt::to_sgr(target, absolute_initial), expected);
  EXPECT_EQ(fmt::to_sgr<wchar_t>(target, absolute_initial),
            (std::wstring{expected.begin(), expected.end()}));
  EXPECT_EQ(target, target_copy);
  EXPECT_EQ(initial, initial_copy);
}
} // namespace

TEST(color_test, sgr_transition) {
  check_sgr_transition({}, {}, "");
  const auto red = fmt::fg(fmt::term_color::red);
  check_sgr_transition(red, red, "");
  check_sgr_transition({}, red, "\x1b[0m");
  check_sgr_transition(fmt::bold(red), red,
      FMT_SGR_EXPECTED("\x1b[0m\x1b[1m\x1b[31m", "\x1b[1m"));
  check_sgr_transition(fmt::fg(fmt::term_color::blue), red,
      FMT_SGR_EXPECTED("\x1b[0m\x1b[34m", "\x1b[34m"));
  check_sgr_transition(fmt::fg(fmt::rgb{1, 2, 3}), red,
      FMT_SGR_EXPECTED("\x1b[0m\x1b[38;2;001;002;003m", "\x1b[38;2;1;2;3m"));
  check_sgr_transition(fmt::bg(fmt::cube_color{0}), red,
      FMT_SGR_EXPECTED("\x1b[0m\x1b[48;5;016m", "\x1b[0;48;5;16m"));
  check_sgr_transition(fmt::fg(fmt::gray_color{0}), {},
      FMT_SGR_EXPECTED("\x1b[38;5;232m", "\x1b[38;5;232m"));
  check_sgr_transition(fmt::bold(), {}, "\x1b[1m");
  check_sgr_transition(red, {}, "\x1b[31m");
  check_sgr_transition(fmt::bold(red), {},
      FMT_SGR_EXPECTED("\x1b[1m\x1b[31m", "\x1b[1;31m"));
  check_sgr_transition(fmt::fg(fmt::rgb{1, 2, 3}), {},
      FMT_SGR_EXPECTED("\x1b[38;2;001;002;003m", "\x1b[38;2;1;2;3m"));
  // Identical RGB appearance does not erase a palette-kind transition.
  check_sgr_transition(fmt::fg(fmt::color::black),
      fmt::fg(fmt::term_color::black),
      FMT_SGR_EXPECTED("\x1b[0m\x1b[38;2;000;000;000m", "\x1b[38;2;0;0;0m"));
  check_sgr_transition(fmt::bold(), fmt::faint(),
      FMT_SGR_EXPECTED("\x1b[0m\x1b[1m", "\x1b[0;1m"));
  check_sgr_transition({}, fmt::bold(), "\x1b[0m");

  const auto all = static_cast<fmt::emphasis>(0xff);
  // Missing target colors select terminal defaults after a reset.
  const auto initial = fmt::text_style{all, fmt::term_color::red,
                                      fmt::term_color::blue};
  check_sgr_transition({}, initial, "\x1b[0m");
  // Compact mode selectively clears intensity and reapplies the surviving bit.
  // Noncompact mode resets and restores all requested colors and attributes.
  const auto target = fmt::text_style{static_cast<fmt::emphasis>(0xfe),
                                     initial.fg(), initial.bg()};
  check_sgr_transition(target, initial,
      FMT_SGR_EXPECTED("\x1b[0m\x1b[2;3;4;5;7;8;9m\x1b[31m\x1b[44m",
                       "\x1b[22;2m"));
  check_sgr_transition(fmt::text_style{static_cast<fmt::emphasis>(0xfd),
                                      initial.fg(), initial.bg()}, initial,
      FMT_SGR_EXPECTED("\x1b[0m\x1b[1;3;4;5;7;8;9m\x1b[31m\x1b[44m",
                       "\x1b[22;1m"));
}

TEST(color_test, sgr_transition_terminal_defaults) {
  const auto fg = fmt::rgb{12, 34, 56};
  const auto bg = fmt::rgb{78, 90, 123};
  const auto initial = fmt::text_style{fg, bg, fmt::emphasis::bold};
  // Compact mode selectively clears a channel. Noncompact mode rebuilds the
  // remaining channel and emphasis after its reset.
  check_sgr_transition(fmt::text_style{fmt::no_color, bg, fmt::emphasis::bold},
      initial, FMT_SGR_EXPECTED("\x1b[0m\x1b[1m\x1b[48;2;078;090;123m",
                                "\x1b[39m"));
  check_sgr_transition(fmt::text_style{fg, fmt::no_color, fmt::emphasis::bold},
      initial, FMT_SGR_EXPECTED("\x1b[0m\x1b[1m\x1b[38;2;012;034;056m",
                                "\x1b[49m"));
  // Compact-mode ties prefer selective changes.
  const auto palette = fmt::text_style{fmt::term_color::red,
                                      fmt::term_color::blue};
  check_sgr_transition(fmt::fg(fmt::term_color::red), palette,
      FMT_SGR_EXPECTED("\x1b[0m\x1b[31m", "\x1b[49m"));
  check_sgr_transition(fmt::bg(fmt::term_color::blue), palette,
      FMT_SGR_EXPECTED("\x1b[0m\x1b[44m", "\x1b[39m"));
  // When both channels and emphasis are removed, a full reset is shortest.
  check_sgr_transition({}, initial, "\x1b[0m");
  check_sgr_transition(fmt::bold(), initial,
      FMT_SGR_EXPECTED("\x1b[0m\x1b[1m", "\x1b[0;1m"));
}

TEST(color_test, sgr_transition_absolute_target) {
  const fmt::text_style states[] = {
      {}, fmt::underline(), fmt::fg(fmt::term_color::red),
      fmt::text_style{fmt::rgb{1, 2, 3}, fmt::gray_color{12},
                      fmt::emphasis::bold}};
  for (auto target : states) {
    target.absolute();
    // Absolute targets always reset, including from defaults, in both modes.
    for (const auto& initial : states)
      check_sgr_transition(target, initial, fmt::to_sgr(target));
    // Equal complete states still require the explicit absolute reset.
    check_sgr_transition(target, target, fmt::to_sgr(target));
  }
}

TEST(color_test, sgr_transition_all_emphasis_pairs) {
  // Interpret the emitted ANSI independently of the transition algorithm.
  const unsigned on[] = {1, 2, 3, 4, 5, 7, 8, 9};
  const unsigned off[] = {22, 22, 23, 24, 25, 27, 28, 29};
  for (unsigned before = 0; before < 256; ++before) {
    for (unsigned after = 0; after < 256; ++after) {
      const auto initial = fmt::text_style{static_cast<fmt::emphasis>(before)};
      const auto target = fmt::text_style{static_cast<fmt::emphasis>(after)};
      const auto sgr = fmt::to_sgr(target, initial);
      if (before == after) {
        EXPECT_TRUE(sgr.empty());
        continue;
      }
      ASSERT_GE(sgr.size(), size_t{4});
      unsigned state = before;
      size_t i = 0;
      // Interpret every complete sequence, carrying terminal state across them.
      // Compact output uses one sequence; noncompact resets a non-default
      // initial state before applying the target attributes.
      while (i < sgr.size()) {
        ASSERT_LT(i + 1, sgr.size());
        ASSERT_EQ(sgr[i++], '\x1b');
        ASSERT_EQ(sgr[i++], '[');
        unsigned code = 0;
        for (;;) {
          ASSERT_LT(i, sgr.size());
          const auto c = sgr[i++];
          if (c >= '0' && c <= '9') {
            code = code * 10 + static_cast<unsigned>(c - '0');
            continue;
          }
          ASSERT_TRUE(c == ';' || c == 'm');
          bool recognized = code == 0;
          if (code == 0) state = 0;
          for (unsigned bit = 0; bit < 8; ++bit) {
            if (code == on[bit]) { state |= 1u << bit; recognized = true; }
            if (code == off[bit]) { state &= ~(1u << bit); recognized = true; }
          }
          ASSERT_TRUE(recognized);
          code = 0;
          if (c == 'm') break;
        }
      }
      EXPECT_EQ(state, after) << "initial=" << before << " target=" << after;
#if FMT_COLOR_COMPACT_SGR
      // Compact output stays combined and no longer than reset-and-rebuild.
      EXPECT_EQ(sgr.find('\x1b', 1), std::string::npos);
      size_t absolute_size = 4; // ESC [ 0 m
      for (unsigned bit = 0; bit < 8; ++bit)
        if (after & (1u << bit)) absolute_size += 2;
      EXPECT_LE(sgr.size(), absolute_size);
#else
      // Reset only when the initial state is not already terminal defaults.
      if (before != 0)
        EXPECT_EQ(sgr.substr(0, 4), "\x1b[0m");
      else
        EXPECT_NE(sgr.substr(0, 4), "\x1b[0m");
#endif
    }
  }
}

TEST(color_test, palette_ansi_escape) {
  const struct {
    fmt::color_t color;
    const char* foreground;
    const char* background;
  } samples[] = {
      {fmt::no_color, "", ""},
      {fmt::rgb{1, 2, 3},
       FMT_SGR_EXPECTED("38;2;001;002;003;", "38;2;1;2;3;"),
       FMT_SGR_EXPECTED("48;2;001;002;003;", "48;2;1;2;3;")},
      {fmt::term_color::red, "31;", "41;"},
      {fmt::term_color::bright_white, "97;", "107;"},
      {fmt::cube_color{0},
       FMT_SGR_EXPECTED("38;5;016;", "38;5;16;"),
       FMT_SGR_EXPECTED("48;5;016;", "48;5;16;")},
      {fmt::cube_color{215}, "38;5;231;", "48;5;231;"},
      {fmt::gray_color{0}, "38;5;232;", "48;5;232;"},
      {fmt::gray_color{23}, "38;5;255;", "48;5;255;"}};
  for (const auto& sample : samples) {
    for (bool background : {false, true}) {
      const auto expected = std::string{background ? sample.background
                                                  : sample.foreground};
      EXPECT_EQ(color_escape<char>(sample.color, background), expected);
      EXPECT_EQ(color_escape<wchar_t>(sample.color, background),
                (std::wstring{expected.begin(), expected.end()}));
      const auto style =
          background ? fmt::bg(sample.color) : fmt::fg(sample.color);
      const auto sgr = expected.empty()
                           ? std::string{}
                           : "\x1b[" + expected.substr(0, expected.size() - 1) + "m";
      check_to_sgr(style, sgr);
      EXPECT_EQ(fmt::format(style, "value"),
                sgr + "value" + (sgr.empty() ? "" : "\x1b[0m"));
    }
  }
}

namespace {
template<typename T, size_t Index>
void check_color_access(T value) {
  using fmt::color_t;
  static_assert(std::is_convertible<T, color_t>::value,
                "color construction must be implicit");
  static_assert(noexcept(color_t{value}) &&
                    noexcept(std::declval<color_t&>() = value),
                "construction and alternative assignment must be noexcept");
#define CHECK_COLOR_GET(Color, Result)                                      \
  static_assert(std::is_same<decltype(fmt::get<T>(std::declval<Color>())),    \
                             Result>::value &&                             \
                    std::is_same<decltype(fmt::get<Index>(                  \
                                     std::declval<Color>())),              \
                                 Result>::value,                           \
                "type and index access must preserve cv/ref qualifiers")
  CHECK_COLOR_GET(color_t&, T&);
  CHECK_COLOR_GET(const color_t&, const T&);
  CHECK_COLOR_GET(color_t&&, T&&);
  CHECK_COLOR_GET(const color_t&&, const T&&);
#undef CHECK_COLOR_GET
  static_assert(std::is_same<decltype(fmt::get_if<T>(
                                 std::declval<color_t*>())), T*>::value &&
                    std::is_same<decltype(fmt::get_if<Index>(
                                     std::declval<const color_t*>())),
                                 const T*>::value,
                "pointer access must preserve constness");

  color_t color = value;
  const color_t& constant = color;
  EXPECT_EQ(color.index(), Index);
  EXPECT_TRUE(fmt::holds_alternative<T>(constant));
  EXPECT_EQ(&fmt::get<T>(color), &fmt::get<Index>(color));
  EXPECT_EQ(&fmt::get<T>(constant), &fmt::get<Index>(constant));
  EXPECT_EQ(fmt::get_if<T>(&color), &fmt::get<T>(color));
  EXPECT_EQ(fmt::get_if<Index>(&color), &fmt::get<T>(color));
  EXPECT_EQ(fmt::get_if<T>(&constant), &fmt::get<T>(constant));
  EXPECT_EQ(fmt::get_if<Index>(&constant), &fmt::get<T>(constant));
  EXPECT_EQ(fmt::get_if<T>(static_cast<color_t*>(nullptr)), nullptr);
  EXPECT_EQ(fmt::get_if<T>(static_cast<const color_t*>(nullptr)), nullptr);
  EXPECT_EQ(fmt::get_if<Index>(static_cast<color_t*>(nullptr)), nullptr);
  EXPECT_EQ(fmt::get_if<Index>(static_cast<const color_t*>(nullptr)), nullptr);
  static_assert(noexcept(fmt::holds_alternative<T>(constant)) &&
                    noexcept(fmt::get_if<T>(&color)) &&
                    noexcept(fmt::get_if<T>(&constant)) &&
                    noexcept(fmt::get_if<Index>(&color)) &&
                    noexcept(fmt::get_if<Index>(&constant)),
                "classification and pointer access must be noexcept");
  EXPECT_EQ(fmt::to_underlying(fmt::get<T>(std::move(color))),
            fmt::to_underlying(value));
  EXPECT_EQ(fmt::to_underlying(fmt::get<Index>(std::move(color))),
            fmt::to_underlying(value));
  EXPECT_EQ(fmt::to_underlying(fmt::get<T>(std::move(constant))),
            fmt::to_underlying(value));
  EXPECT_EQ(fmt::to_underlying(fmt::get<Index>(std::move(constant))),
            fmt::to_underlying(value));

  const color_t alternatives[] = {fmt::no_color, fmt::rgb{},
                                 fmt::term_color::black, fmt::cube_color{0},
                                 fmt::gray_color{0}};
  for (auto other : alternatives) {
    if (other.index() == Index) continue;
    const auto& const_other = other;
    EXPECT_FALSE(fmt::holds_alternative<T>(other));
    EXPECT_EQ(fmt::get_if<T>(&other), nullptr);
    EXPECT_EQ(fmt::get_if<T>(&const_other), nullptr);
    EXPECT_EQ(fmt::get_if<Index>(&other), nullptr);
    EXPECT_EQ(fmt::get_if<Index>(&const_other), nullptr);
    EXPECT_THROW_MSG((void)fmt::get<T>(other), fmt::format_error,
                     "wrong color_t alternative");
    EXPECT_THROW((void)fmt::get<T>(const_other), fmt::format_error);
    EXPECT_THROW((void)fmt::get<T>(std::move(other)), fmt::format_error);
    EXPECT_THROW((void)fmt::get<T>(std::move(const_other)), fmt::format_error);
    EXPECT_THROW((void)fmt::get<Index>(other), fmt::format_error);
    EXPECT_THROW((void)fmt::get<Index>(const_other), fmt::format_error);
    EXPECT_THROW((void)fmt::get<Index>(std::move(other)), fmt::format_error);
    EXPECT_THROW((void)fmt::get<Index>(std::move(const_other)), fmt::format_error);
  }
}

template<typename T>
void check_color_assignment(T replacement) {
  const fmt::color_t original[] = {fmt::no_color, fmt::rgb{1, 2, 3},
                                  fmt::term_color::bright_white,
                                  fmt::cube_color{215}, fmt::gray_color{23}};
  for (auto color : original) {
    const fmt::color_t expected = replacement;
    EXPECT_EQ(&(color = replacement), &color);
    EXPECT_EQ(color, expected);
    // Assignment from the active member must read it before replacing storage.
    EXPECT_EQ(&(color = fmt::get<T>(color)), &color);
    EXPECT_EQ(color, expected);
  }
}

struct color_kind_visitor {
#if FMT_MSC_VERSION >= 1929 && FMT_MSC_VERSION <= 1944 && !FMT_CLANG_VERSION
  // MSVC 19.29 and 19.44 reject copying the empty union member in constexpr.
  constexpr size_t operator()(const fmt::no_color_t&) const { return 0; }
#else
  constexpr size_t operator()(fmt::no_color_t) const { return 0; }
#endif
  constexpr size_t operator()(fmt::rgb) const { return 1; }
  constexpr size_t operator()(fmt::term_color) const { return 2; }
  constexpr size_t operator()(fmt::cube_color) const { return 3; }
  constexpr size_t operator()(fmt::gray_color) const { return 4; }
};

template<typename T, size_t Index>
constexpr bool check_color_constexpr_access(fmt::color_t&& color) {
  return
#if !FMT_GCC_VERSION || FMT_GCC_VERSION >= 500
         // GCC 4.9 rejects constexpr pointer comparisons involving temporaries.
         fmt::get_if<T>(&color) == &fmt::get<Index>(color) &&
         fmt::get_if<Index>(&color) == &fmt::get<T>(color) &&
#endif
         color_kind_visitor{}(fmt::get<Index>(std::move(color))) == Index &&
         fmt::visit(color_kind_visitor{}, color) == Index &&
         fmt::visit(color_kind_visitor{}, std::move(color)) == Index &&
         fmt::fg(color).fg() == color && fmt::bg(color).bg() == color;
}

struct color_category_visitor {
  color_category_visitor() = default;
  color_category_visitor(const color_category_visitor&) = delete;
  color_category_visitor(color_category_visitor&&) = delete;
  template<typename T> unsigned operator()(T&&) & {
    return (std::is_const<typename std::remove_reference<T>::type>::value
                ? 1u : 0u) + (std::is_lvalue_reference<T>::value ? 0u : 2u);
  }
  template<typename T> unsigned operator()(T&& value) && {
    return (*this)(std::forward<T>(value)) + 4u;
  }
};

struct color_mutating_visitor {
  int& calls;
  void operator()(fmt::no_color_t&) const { ++calls; }
  void operator()(fmt::rgb& value) const {
    ++calls;
    value.r = 42;
  }
  void operator()(fmt::cube_color& value) const {
    ++calls;
    value.r(5);
  }
  void operator()(fmt::gray_color& value) const {
    ++calls;
    value = fmt::gray_color{23};
  }
  void operator()(fmt::term_color& value) const {
    ++calls;
    value = fmt::term_color::bright_white;
  }
};

struct color_reference_visitor {
  int& value;
  template<typename T> int& operator()(T&&) const { return value; }
};

struct color_throwing_visitor {
  template<typename T> void operator()(T&&) const { throw 42; }
};

#if FMT_USE_CONSTEXPR
constexpr bool check_color_constexpr_assignment() {
  fmt::color_t color;
  color = fmt::rgb{1, 2, 3};
  fmt::get<fmt::rgb>(color).r = 4;
  fmt::get_if<1>(&color)->b = 5;
  bool rgb_ok = fmt::get<1>(color) == fmt::rgb{4, 2, 5};
  color = fmt::term_color::black;
  bool terminal_ok = fmt::holds_alternative<fmt::term_color>(color) &&
                     fmt::get<2>(color) == fmt::term_color::black;
  color = fmt::cube_color{215};
  bool cube_ok = fmt::get<3>(color) == fmt::cube_color{215};
  color = fmt::gray_color{23};
  bool gray_ok = fmt::get<4>(color) == fmt::gray_color{23};
  color = fmt::no_color;
  return rgb_ok && terminal_ok && cube_ok && gray_ok &&
         fmt::holds_alternative<fmt::no_color_t>(color);
}
#endif
}  // namespace

TEST(color_test, color_variant_access) {
  check_color_access<fmt::no_color_t, 0>(fmt::no_color);
  check_color_access<fmt::rgb, 1>(fmt::rgb{});
  check_color_access<fmt::rgb, 1>(fmt::color::coral);
  check_color_access<fmt::term_color, 2>(fmt::term_color::black);
  check_color_access<fmt::term_color, 2>(fmt::term_color::bright_white);
  check_color_access<fmt::cube_color, 3>(fmt::cube_color{0});
  check_color_access<fmt::cube_color, 3>(fmt::cube_color{215});
  check_color_access<fmt::gray_color, 4>(fmt::gray_color{0});
  check_color_access<fmt::gray_color, 4>(fmt::gray_color{23});
  auto color = fmt::color_t{fmt::rgb{1, 2, 3}};
  fmt::get<fmt::rgb>(color).r = 4;
  fmt::get<1>(color).g = 5;
  fmt::get_if<fmt::rgb>(&color)->b = 6;
  fmt::get_if<1>(&color)->r = 7;
  EXPECT_EQ(fmt::get<fmt::rgb>(color), (fmt::rgb{7, 5, 6}));
  EXPECT_EQ(fmt::get_if<2>(&color), nullptr);
  EXPECT_EQ(fmt::get_if<fmt::gray_color>(&color), nullptr);
  EXPECT_THROW((void)fmt::get<2>(color), fmt::format_error);
}

TEST(color_test, color_variant_assignment) {
  check_color_assignment(fmt::no_color);
  check_color_assignment(fmt::rgb{});
  check_color_assignment(fmt::color::coral);
  check_color_assignment(fmt::term_color::black);
  check_color_assignment(fmt::term_color::bright_white);
  check_color_assignment(fmt::cube_color{215});
  check_color_assignment(fmt::gray_color{23});
  const fmt::color_t colors[] = {fmt::no_color, fmt::rgb{1, 2, 3},
                                fmt::term_color::black, fmt::cube_color{1, 2, 3},
                                fmt::gray_color{12}};
  for (const auto& source : colors) {
    fmt::color_t copy(source);
    fmt::color_t moved(std::move(copy));
    EXPECT_EQ(moved, source);
    for (auto target : colors) {
      auto destination = target;
      EXPECT_EQ(&(target = source), &target);
      EXPECT_EQ(target, source);
      copy = source;
      EXPECT_EQ(&(destination = std::move(copy)), &destination);
      EXPECT_EQ(destination, source);
    }
  }
}

TEST(color_test, color_variant_visit) {
  const fmt::color_t colors[] = {fmt::no_color, fmt::rgb{1, 2, 3},
                                fmt::term_color::black, fmt::cube_color{1, 2, 3},
                                fmt::gray_color{12}};
  const fmt::color_t changed[] = {fmt::no_color, fmt::rgb{42, 2, 3},
                                 fmt::term_color::bright_white,
                                 fmt::cube_color{5, 2, 3}, fmt::gray_color{23}};
  for (size_t i = 0; i < 5; ++i) {
    auto color = colors[i];
    const auto& constant = color;
    EXPECT_EQ(fmt::visit(color_kind_visitor{}, constant), color.index());
    color_category_visitor visitor;
    EXPECT_EQ(fmt::visit(visitor, color), 0u);
    EXPECT_EQ(fmt::visit(visitor, constant), 1u);
    EXPECT_EQ(fmt::visit(visitor, std::move(color)), 2u);
    EXPECT_EQ(fmt::visit(visitor, std::move(constant)), 3u);
    EXPECT_EQ(fmt::visit(std::move(visitor), color), 4u);
    EXPECT_EQ(fmt::visit(std::move(visitor), constant), 5u);
    EXPECT_EQ(fmt::visit(std::move(visitor), std::move(color)), 6u);
    EXPECT_EQ(fmt::visit(std::move(visitor), std::move(constant)), 7u);
    int calls = 0;
    fmt::visit(color_mutating_visitor{calls}, color);
    EXPECT_EQ(calls, 1);
    EXPECT_EQ(color, changed[i]);
    int result = 0;
    EXPECT_EQ(&fmt::visit(color_reference_visitor{result}, color), &result);
    fmt::visit(color_reference_visitor{result}, color) = 9;
    EXPECT_EQ(result, 9);
    EXPECT_THROW(fmt::visit(color_throwing_visitor{}, color), int);
    EXPECT_EQ(color, changed[i]);
  }
}

TEST(color_test, color_variant_constexpr) {
  static_assert(
      check_color_constexpr_access<fmt::no_color_t, 0>(fmt::no_color) &&
          check_color_constexpr_access<fmt::rgb, 1>(fmt::rgb{1, 2, 3}) &&
          check_color_constexpr_access<fmt::term_color, 2>(fmt::term_color::red) &&
          check_color_constexpr_access<fmt::cube_color, 3>(fmt::cube_color{215}) &&
          check_color_constexpr_access<fmt::gray_color, 4>(fmt::gray_color{23}),
      "mutable access, visitation, and style factories must be constexpr in C++11");
#if FMT_GCC_VERSION && FMT_GCC_VERSION < 500
  // GCC 4.9 requires static storage for the constexpr pointer checks below.
  static constexpr fmt::color_t empty{};
  static constexpr fmt::color_t rgb = fmt::rgb{1, 2, 3};
  static constexpr fmt::color_t terminal = fmt::term_color::black;
#else
  constexpr fmt::color_t empty;
  constexpr fmt::color_t rgb = fmt::rgb{1, 2, 3};
  constexpr fmt::color_t terminal = fmt::term_color::black;
#endif
  constexpr fmt::color_t cube = fmt::cube_color{215};
  constexpr fmt::color_t gray = fmt::gray_color{23};
  static_assert(fmt::get<3>(cube) == fmt::cube_color{215} &&
                    fmt::get<4>(gray) == fmt::gray_color{23},
                "palette access must be constexpr in C++11");
  static_assert(fmt::detail::unpack_color(fmt::to_underlying(cube)) == cube &&
                    fmt::detail::unpack_color(fmt::to_underlying(gray)) == gray,
                "palette packing must round-trip in constant evaluation");
  constexpr auto palette_style = fmt::text_style{cube, gray, fmt::emphasis::bold};
  static_assert(palette_style.fg() == cube && palette_style.bg() == gray &&
                    palette_style.emph() == fmt::emphasis::bold,
                "palette style getters must be constexpr in C++11");
  static_assert(empty.index() == 0 && fmt::holds_alternative<fmt::no_color_t>(empty),
                "default construction must hold the unspecified alternative");
  static_assert(rgb.index() == 1 && fmt::get<fmt::rgb>(rgb).g == 2 &&
                    fmt::get<1>(rgb).b == 3,
                "RGB type and index access must be constexpr in C++11");
  static_assert(terminal.index() == 2 &&
                    fmt::get<fmt::term_color>(terminal) == fmt::term_color::black,
                "terminal black must remain specified in constant evaluation");
  static_assert(fmt::get_if<1>(&rgb) == &fmt::get<fmt::rgb>(rgb) &&
                    fmt::get_if<fmt::term_color>(&rgb) == nullptr &&
                    fmt::get_if<0>(static_cast<const fmt::color_t*>(nullptr)) ==
                        nullptr,
                "pointer access must be constexpr in C++11");
  static_assert(fmt::visit(color_kind_visitor{}, empty) == 0 &&
                    fmt::visit(color_kind_visitor{}, rgb) == 1 &&
                    fmt::visit(color_kind_visitor{}, terminal) == 2 &&
                    fmt::visit(color_kind_visitor{}, cube) == 3 &&
                    fmt::visit(color_kind_visitor{}, gray) == 4,
                "visitation must be constexpr in C++11");
#if FMT_USE_CONSTEXPR
  static_assert(check_color_constexpr_assignment(),
                "replacement and mutable access must support constant evaluation");
#endif
}

TEST(color_test, color_variant_style_values) {
  static_assert(std::is_same<decltype(fmt::text_style{}.fg()),
                             fmt::color_t>::value &&
                    std::is_same<decltype(fmt::text_style{}.bg()),
                                 fmt::color_t>::value &&
                    std::is_same<decltype(fmt::text_style{}.get_foreground()),
                                 fmt::color_t>::value &&
                    std::is_same<decltype(fmt::text_style{}.get_background()),
                                 fmt::color_t>::value,
                "short and compatibility getters must return color values");
  auto foreground = fmt::color_t{fmt::rgb{1, 2, 3}};
  auto background = fmt::color_t{fmt::term_color::black};
  const auto style =
      fmt::text_style{foreground, background, fmt::emphasis::strike};
  fmt::get<1>(foreground).r = 255;
  fmt::get<2>(background) = fmt::term_color::white;
  EXPECT_EQ(fmt::get<fmt::rgb>(style.fg()), (fmt::rgb{1, 2, 3}));
  EXPECT_EQ(fmt::get<fmt::term_color>(style.bg()), fmt::term_color::black);
  auto extracted = style.fg();
  fmt::get<1>(extracted).b = 255;
  EXPECT_EQ(fmt::get<1>(style.fg()).b, 3);
  // Copying an alternative from a temporary color is safe.
  auto copy = fmt::get<fmt::rgb>(style.fg());
  copy.g = 255;
  EXPECT_EQ(copy, (fmt::rgb{1, 255, 3}));
  EXPECT_EQ(fmt::get<1>(style.fg()).g, 2);
  EXPECT_EQ(fmt::format(style, "value"),
            FMT_SGR_EXPECTED(
                "\x1b[9m\x1b[38;2;001;002;003m\x1b[40mvalue\x1b[0m",
                "\x1b[9;38;2;1;2;3;40mvalue\x1b[0m"));
}

TEST(color_test, rgb_checked_channels) {
  constexpr uint8_t byte_channel = 128;
  static_assert(fmt::rgb{byte_channel, 0, 255} == fmt::rgb{128, 0, 255} &&
                    fmt::rgb{0, byte_channel, 255} == fmt::rgb{0, 128, 255} &&
                    fmt::rgb{0, 255, byte_channel} == fmt::rgb{0, 255, 128},
                "mixed byte and int inputs must resolve to the checked constructor");
  static_assert(fmt::rgb{0, 255, 128} == fmt::rgb{0x00FF80},
                "signed channel inputs must support C++11 constant evaluation");
  static_assert(noexcept(fmt::rgb{0, 0, 0}),
                "RGB channel construction must be noexcept");
  for (int channel = 0; channel <= 255; ++channel) {
    auto color = fmt::rgb{channel, channel, channel};
    EXPECT_EQ(color.r, channel);
    EXPECT_EQ(color.g, channel);
    EXPECT_EQ(color.b, channel);
  }
#ifdef NDEBUG
  const int invalid[] = {-1, 256, (std::numeric_limits<int>::min)(),
                        (std::numeric_limits<int>::max)()};
  for (auto channel : invalid) {
    SCOPED_TRACE(channel);
    const int clamped = channel < 0 ? 0 : 255;
    EXPECT_EQ((fmt::rgb{channel, 128, 64}), (fmt::rgb{clamped, 128, 64}));
    EXPECT_EQ((fmt::rgb{128, channel, 64}), (fmt::rgb{128, clamped, 64}));
    EXPECT_EQ((fmt::rgb{128, 64, channel}), (fmt::rgb{128, 64, clamped}));
  }
#endif
}

TEST(color_test, rgb) {
  static_assert(!std::is_convertible<uint32_t, fmt::rgb>::value,
                "integer to rgb conversion must be explicit");
  static_assert(!std::is_convertible<fmt::rgb, uint32_t>::value,
                "rgb to integer conversion must be explicit");
  static_assert(std::is_same<decltype(fmt::color::red), const fmt::rgb>::value,
                "named colors must be rgb constants");
  constexpr auto c = fmt::rgb{0xAB123456};
  static_assert(c.r == 0x12 && c.g == 0x34 && c.b == 0x56,
                "the high byte must be ignored");
  static_assert(static_cast<uint32_t>(c) == 0x123456,
                "packed rgb must have a zero high byte");
  static_assert(static_cast<uint32_t>(fmt::color::alice_blue) == 0xF0F8FF,
                "named colors must preserve their values");
  auto components = fmt::rgb{0x12, 0x34, 0x56};
  EXPECT_EQ(static_cast<uint32_t>(components), 0x123456u);
  EXPECT_EQ(static_cast<uint32_t>(fmt::rgb{}), 0u);
  EXPECT_EQ(static_cast<uint32_t>(fmt::color::black), 0u);
  EXPECT_EQ(static_cast<uint32_t>(fmt::color::white), 0xFFFFFFu);
}

TEST(color_test, rgb_or) {
  constexpr auto lhs = fmt::rgb{0xC0F000};
  constexpr auto rhs = fmt::rgb{0x000FEE};
  static_assert((lhs | rhs) == fmt::rgb{0xC0FFEE},
                "RGB OR supports C++11 constant evaluation");
  static_assert(noexcept(lhs | rhs), "RGB OR must be noexcept");
  EXPECT_EQ(lhs | rhs, fmt::rgb{0xC0FFEE});
  EXPECT_EQ(lhs, fmt::rgb{0xC0F000});
  EXPECT_EQ(rhs, fmt::rgb{0x000FEE});
  auto assigned = lhs;
  EXPECT_EQ(&(assigned |= rhs), &assigned);
  EXPECT_EQ(assigned, lhs | rhs);
}

TEST(color_test, rgb_blend) {
  static_assert(std::is_same<decltype(fmt::blend(fmt::rgb{}, fmt::rgb{})),
                             fmt::rgb>::value,
                "blend must return an RGB value");
  static_assert(noexcept(fmt::blend(fmt::rgb{}, fmt::rgb{})),
                "blend must be noexcept");
  auto black = fmt::rgb{};
  auto white = fmt::rgb{255, 255, 255};
  EXPECT_EQ(fmt::blend(black, black), black);
  EXPECT_EQ(fmt::blend(white, white), white);
  EXPECT_EQ(fmt::blend(black, black, black), black);
  EXPECT_EQ(fmt::blend(white, white, white, white), white);
  EXPECT_EQ(fmt::blend(black, white), (fmt::rgb{127, 127, 127}));
  EXPECT_EQ(fmt::blend(black, white, white), (fmt::rgb{170, 170, 170}));
  EXPECT_EQ(fmt::blend(white, black, white), (fmt::rgb{170, 170, 170}));
  EXPECT_EQ(fmt::blend(white, white, black), (fmt::rgb{170, 170, 170}));
  EXPECT_EQ(black, fmt::rgb{});
  EXPECT_EQ(white, (fmt::rgb{255, 255, 255}));

  auto updated = black;
  updated = fmt::blend(updated, white);
  EXPECT_EQ(updated, (fmt::rgb{127, 127, 127}));
  EXPECT_EQ(black, fmt::rgb{});
  EXPECT_EQ(white, (fmt::rgb{255, 255, 255}));

  auto first  = fmt::rgb{0, 1, 254};
  auto second = fmt::rgb{0, 2, 255};
  auto third  = fmt::rgb{2, 4, 255};
  EXPECT_EQ(fmt::blend(first, second, third), (fmt::rgb{0, 2, 254}));
  EXPECT_EQ(fmt::blend(third, second, first), (fmt::rgb{0, 2, 254}));
  EXPECT_EQ(fmt::blend(fmt::color::red, fmt::color::green, fmt::color::blue),
            (fmt::rgb{85, 42, 85}));
  constexpr auto same = fmt::blend(fmt::rgb{1, 2, 3}, fmt::rgb{1, 2, 3});
  constexpr auto average = fmt::blend(fmt::rgb{255, 0, 0}, fmt::rgb{0, 255, 0},
                                     fmt::rgb{0, 0, 255});
  static_assert(same.r == 1 && same.g == 2 && same.b == 3,
                "equal RGB colors preserve all channels");
  static_assert(average.r == 85 && average.g == 85 && average.b == 85,
                "blend must support constant evaluation");
}

namespace {
template<typename... Colors>
auto can_blend(int) -> decltype(fmt::blend(std::declval<Colors>()...),
                                std::true_type{});
template<typename... Colors> auto can_blend(...) -> std::false_type;
template<typename T>
auto can_to_rgb(int) -> decltype(fmt::to_rgb(std::declval<T>()), std::true_type{});
template<typename T> auto can_to_rgb(...) -> std::false_type;
} // namespace

TEST(color_test, to_rgb) {
  static_assert(std::is_same<decltype(fmt::to_rgb(fmt::rgb{})), fmt::rgb>::value &&
                    std::is_same<decltype(fmt::to_rgb(fmt::term_color::red)),
                                 fmt::rgb>::value &&
                    std::is_same<decltype(fmt::to_rgb(std::declval<fmt::gray_color>())),
                                 fmt::rgb>::value &&
                    std::is_same<decltype(fmt::to_rgb(std::declval<fmt::cube_color>())),
                                 fmt::rgb>::value,
                "plain colors convert to RGB values");
  static_assert(std::is_same<decltype(fmt::to_rgb(fmt::no_color)),
                             fmt::no_color_t>::value &&
                    std::is_same<decltype(fmt::to_rgb(fmt::color_t{})),
                                 fmt::color_t>::value,
                "unspecified and variant conversions preserve their result types");
  static_assert(!decltype(can_to_rgb<int>(0))::value &&
                    !decltype(can_to_rgb<const char*>(0))::value &&
                    !decltype(can_to_rgb<fmt::emphasis>(0))::value &&
                    !decltype(can_to_rgb<fmt::text_style>(0))::value,
                "to_rgb rejects non-color arguments");
  static_assert(noexcept(fmt::to_rgb(fmt::rgb{})) &&
                    noexcept(fmt::to_rgb(fmt::term_color::red)) &&
                    noexcept(fmt::to_rgb(std::declval<fmt::gray_color>())) &&
                    noexcept(fmt::to_rgb(std::declval<fmt::cube_color>())) &&
                    noexcept(fmt::to_rgb(fmt::no_color)) &&
                    noexcept(fmt::to_rgb(fmt::color_t{})),
                "all to_rgb overloads must be noexcept");
  static_assert(fmt::to_rgb(fmt::rgb{0x123456}) == fmt::rgb{0x123456} &&
                    fmt::to_rgb(fmt::term_color::red) == fmt::rgb{205, 0, 0} &&
                    fmt::to_rgb(fmt::term_color::bright_blue) == fmt::rgb{92, 92, 255} &&
                    fmt::to_rgb(fmt::gray_color{0}) == fmt::rgb{8, 8, 8} &&
                    fmt::to_rgb(fmt::gray_color{23}) == fmt::rgb{238, 238, 238} &&
                    fmt::to_rgb(fmt::cube_color{0}) == fmt::rgb{} &&
                    fmt::to_rgb(fmt::cube_color{215}) == fmt::rgb{255, 255, 255},
                "plain conversion supports C++11 constant evaluation");
  constexpr auto unspecified = fmt::to_rgb(fmt::no_color);
  static_assert(fmt::color_t{unspecified} == fmt::color_t{},
                "the unspecified sentinel remains unspecified");
  static_assert(fmt::to_rgb(fmt::color_t{}) == fmt::color_t{} &&
                    fmt::to_rgb(fmt::color_t{fmt::rgb{1, 2, 3}}) ==
                        fmt::color_t{fmt::rgb{1, 2, 3}} &&
                    fmt::to_rgb(fmt::color_t{fmt::term_color::black}) ==
                        fmt::color_t{fmt::rgb{}} &&
                    fmt::to_rgb(fmt::color_t{fmt::gray_color{12}}) ==
                        fmt::color_t{fmt::rgb{128, 128, 128}} &&
                    fmt::to_rgb(fmt::color_t{fmt::cube_color{1, 2, 3}}) ==
                        fmt::color_t{fmt::rgb{95, 135, 175}},
                "all variant alternatives support C++11 constant conversion");
  for (const auto& sample : composition_samples) {
    const auto source = sample.color;
    const auto converted = fmt::to_rgb(source);
    EXPECT_EQ(converted, source.index() == 0 ? fmt::color_t{}
                                             : fmt::color_t{sample.promoted});
    EXPECT_EQ(source, sample.color);
    EXPECT_EQ(fmt::to_rgb(converted), converted);
  }
  EXPECT_EQ(fmt::to_rgb(fmt::gray_color{12}), (fmt::rgb{128, 128, 128}));
  EXPECT_EQ(fmt::to_rgb(fmt::cube_color{1, 2, 3}), (fmt::rgb{95, 135, 175}));
}

TEST(color_test, blend_overloads) {
  static_assert(std::is_same<decltype(fmt::blend(fmt::term_color::red,
                                                fmt::term_color::red)),
                             fmt::rgb>::value &&
                    std::is_same<decltype(fmt::blend(fmt::cube_color{0},
                                                     fmt::gray_color{0})),
                                 fmt::rgb>::value,
                "plain palette arguments return RGB");
  static_assert(std::is_same<decltype(fmt::blend(fmt::rgb{}, fmt::color_t{})),
                             fmt::color_t>::value &&
                    std::is_same<decltype(fmt::blend(fmt::color_t{}, fmt::rgb{})),
                                 fmt::color_t>::value &&
                    std::is_same<decltype(fmt::blend(fmt::rgb{}, fmt::no_color)),
                                 fmt::color_t>::value,
                "a variant or unspecified argument selects variant blending");
  static_assert(!decltype(can_blend<>(0))::value &&
                    !decltype(can_blend<int>(0))::value &&
                    !decltype(can_blend<fmt::rgb, int>(0))::value &&
                    !decltype(can_blend<int, fmt::rgb>(0))::value &&
                    !decltype(can_blend<fmt::color_t, const char*>(0))::value,
                "blending requires valid color arguments");
  static_assert(noexcept(fmt::blend(fmt::term_color::red,
                                    std::declval<fmt::gray_color>())) &&
                    noexcept(fmt::blend(fmt::color_t{}, fmt::rgb{})),
                "both blend overloads must be noexcept");
#if FMT_CPLUSPLUS > 202002L && defined(__cpp_concepts) && \
    __cpp_concepts >= 201907L
  static_assert(fmt::Color<const fmt::rgb&> && fmt::Color<fmt::term_color> &&
                    fmt::Color<fmt::cube_color> && fmt::Color<fmt::gray_color> &&
                    !fmt::Color<fmt::color_t> && !fmt::Color<fmt::no_color_t> &&
                    !fmt::Color<int>,
                "Color identifies only plain color types");
#endif
  constexpr auto mixed = fmt::blend(fmt::term_color::red,
                                    fmt::cube_color{1, 2, 3},
                                    fmt::gray_color{12});
  static_assert(mixed == fmt::rgb{142, 87, 101},
                "plain palette promotion and averaging must be constexpr");
  constexpr auto terminal = fmt::blend(fmt::term_color::red,
                                       fmt::term_color::red);
  static_assert(terminal == fmt::rgb{205, 0, 0},
                "equal plain terminal colors return their RGB equivalent");
}

TEST(color_test, blend_minimum_arity) {
  static_assert(!decltype(can_blend<>(0))::value &&
                    !decltype(can_blend<fmt::rgb>(0))::value &&
                    !decltype(can_blend<fmt::term_color>(0))::value &&
                    !decltype(can_blend<fmt::cube_color>(0))::value &&
                    !decltype(can_blend<fmt::gray_color>(0))::value &&
                    !decltype(can_blend<fmt::color_t>(0))::value &&
                    !decltype(can_blend<fmt::no_color_t>(0))::value &&
                    !decltype(can_blend<fmt::text_style>(0))::value,
                "public blend requires at least two arguments");
  static_assert(decltype(can_blend<fmt::rgb, fmt::rgb>(0))::value &&
                    decltype(can_blend<fmt::rgb, fmt::color_t>(0))::value &&
                    decltype(can_blend<fmt::color_t, fmt::rgb>(0))::value &&
                    decltype(can_blend<fmt::no_color_t,
                                       fmt::no_color_t>(0))::value &&
                    decltype(can_blend<fmt::text_style,
                                       fmt::text_style>(0))::value,
                "two-argument color and style blends remain available");
}

TEST(color_test, color_variant_blend) {
  constexpr auto empty = fmt::blend(fmt::no_color, fmt::color_t{}, fmt::no_color);
  static_assert(empty == fmt::color_t{}, "all unspecified colors remain empty");
  constexpr auto single = fmt::blend(fmt::no_color, fmt::gray_color{12},
                                     fmt::color_t{});
  static_assert(single == fmt::color_t{fmt::gray_color{12}},
                "one specified color retains its palette kind");
  constexpr auto same = fmt::blend(fmt::color_t{fmt::gray_color{12}},
                                   fmt::gray_color{12});
  static_assert(same == fmt::color_t{fmt::rgb{128, 128, 128}},
                "two specified palette colors produce RGB");
  constexpr auto average = fmt::blend(
      fmt::rgb{0, 1, 254}, fmt::no_color, fmt::rgb{0, 2, 255},
      fmt::color_t{fmt::rgb{2, 4, 255}}, fmt::no_color);
  static_assert(average == fmt::color_t{fmt::rgb{0, 2, 254}},
                "ignore unspecified colors and divide once after summation");
  constexpr auto palette = fmt::blend(fmt::cube_color{1, 2, 3},
                                      fmt::color_t{fmt::gray_color{12}});
  static_assert(palette == fmt::color_t{fmt::rgb{111, 131, 151}},
                "a trailing variant selects RGB promotion of plain arguments");

  for (size_t i = 0; i != 5; ++i) {
    for (size_t j = 0; j != 5; ++j) {
      SCOPED_TRACE(fmt::format("blend kinds {} and {}", i, j));
      const auto lhs = composition_samples[i].color;
      const auto rhs = composition_samples[j].color;
      const auto a = composition_samples[i].promoted;
      const auto b = composition_samples[j].promoted;
      const auto expected = i == 0 ? rhs : j == 0 ? lhs : fmt::color_t{fmt::rgb{
          static_cast<uint8_t>((unsigned{a.r} + b.r) / 2),
          static_cast<uint8_t>((unsigned{a.g} + b.g) / 2),
          static_cast<uint8_t>((unsigned{a.b} + b.b) / 2)}};
      EXPECT_EQ(fmt::blend(lhs, rhs), expected);
      EXPECT_EQ(fmt::blend(fmt::no_color, lhs, fmt::no_color, rhs,
                           fmt::no_color), expected);
      EXPECT_EQ(fmt::blend(lhs, fmt::no_color), lhs);
      EXPECT_EQ(lhs, composition_samples[i].color);
      EXPECT_EQ(rhs, composition_samples[j].color);
    }
  }
}

TEST(color_test, text_style_blend) {
  static_assert(std::is_same<decltype(fmt::blend(fmt::text_style{},
                                                fmt::text_style{})),
                             fmt::text_style>::value &&
                    decltype(can_blend<const fmt::text_style&,
                                       fmt::text_style>(0))::value,
                "style blending accepts style values and returns a style");
  static_assert(!decltype(can_blend<fmt::text_style, fmt::rgb>(0))::value &&
                    !decltype(can_blend<fmt::rgb, fmt::text_style>(0))::value &&
                    !decltype(can_blend<fmt::text_style, int>(0))::value &&
                    !decltype(can_blend<fmt::emphasis>(0))::value,
                "style blending requires style arguments without color overlap");
  static_assert(noexcept(fmt::blend(fmt::text_style{}, fmt::text_style{})),
                "style blending must be noexcept");
  constexpr auto empty = fmt::blend(fmt::text_style{}, fmt::text_style{});
  static_assert(empty == fmt::text_style{}, "empty styles blend to an empty style");
  constexpr auto single = fmt::text_style{fmt::cube_color{1, 2, 3},
                                         fmt::gray_color{12},
                                         fmt::emphasis::bold};
  static_assert(fmt::blend(fmt::text_style{}, single) == single &&
                    fmt::blend(fmt::text_style{}, single, fmt::text_style{}) == single,
                "one specified color per channel retains its kind");
  constexpr auto first = fmt::text_style{fmt::term_color::red, fmt::no_color,
                                        fmt::emphasis::bold};
  constexpr auto second = fmt::text_style{fmt::gray_color{12},
                                         fmt::cube_color{1, 2, 3},
                                         fmt::emphasis::underline};
  constexpr auto third = fmt::text_style{fmt::rgb{}, fmt::no_color,
                                        fmt::emphasis::strike};
  constexpr auto averaged = fmt::blend(first, second, third);
  static_assert(averaged.fg() == fmt::color_t{fmt::rgb{111, 42, 42}} &&
                    averaged.bg() == fmt::color_t{fmt::cube_color{1, 2, 3}} &&
                    averaged.emph() == (fmt::emphasis::bold |
                                        fmt::emphasis::underline |
                                        fmt::emphasis::strike),
                "channels use independent counts and emphasis bits are ORed");

  for (size_t i = 0; i != 5; ++i) {
    for (size_t j = 0; j != 5; ++j) {
      SCOPED_TRACE(fmt::format("style blend kinds {} and {}", i, j));
      const auto a = composition_samples[i].promoted;
      const auto b = composition_samples[j].promoted;
      const auto foreground = i == 0 ? composition_samples[j].color
          : j == 0 ? composition_samples[i].color : fmt::color_t{fmt::rgb{
              static_cast<uint8_t>((unsigned{a.r} + b.r) / 2),
              static_cast<uint8_t>((unsigned{a.g} + b.g) / 2),
              static_cast<uint8_t>((unsigned{a.b} + b.b) / 2)}};
      const auto lhs = fmt::text_style{composition_samples[i].color,
                                       fmt::gray_color{12}, fmt::emphasis::bold};
      const auto rhs = fmt::text_style{composition_samples[j].color, fmt::no_color,
                                       fmt::emphasis::underline};
      EXPECT_EQ(fmt::blend(lhs, rhs),
                (fmt::text_style{foreground, fmt::gray_color{12},
                                 fmt::emphasis::bold | fmt::emphasis::underline}));
      EXPECT_EQ(fmt::blend(lhs, rhs, fmt::bg(fmt::cube_color{1, 2, 3})),
                (fmt::text_style{foreground, fmt::rgb{111, 131, 151},
                                 fmt::emphasis::bold | fmt::emphasis::underline}));
      EXPECT_EQ(lhs.fg(), composition_samples[i].color);
      EXPECT_EQ(lhs.bg(), fmt::color_t{fmt::gray_color{12}});
      EXPECT_EQ(rhs.fg(), composition_samples[j].color);
      EXPECT_FALSE(rhs.has_bg());
    }
  }
  for (unsigned mask = 0; mask != 256; ++mask) {
    auto lhs = fmt::text_style{static_cast<fmt::emphasis>(mask)};
    auto rhs = fmt::text_style{static_cast<fmt::emphasis>(mask ^ 0xFFu)};
    EXPECT_EQ(fmt::blend(lhs, rhs),
              fmt::text_style{static_cast<fmt::emphasis>(0xFF)});
  }
}

TEST(color_test, text_style_color_constructor) {
  using fmt::color_t;
  static_assert(std::is_default_constructible<fmt::text_style>::value,
                "default construction must remain unambiguous");
  static_assert(std::is_convertible<fmt::emphasis, fmt::text_style>::value,
                "emphasis conversion must remain implicit");
  static_assert(std::is_convertible<color_t, fmt::text_style>::value,
                "the color constructor must not be explicit");
  static_assert(noexcept(fmt::text_style{fmt::color::red,
                                         fmt::term_color::black,
                                         fmt::emphasis::bold}),
                "color construction must be noexcept");
  static_assert(noexcept(fmt::text_style{fmt::emphasis::bold,
                                         fmt::color::red,
                                         fmt::term_color::black}),
                "emphasis-first construction must be noexcept");
  constexpr auto emphasis_only = fmt::text_style{fmt::emphasis::bold};
  static_assert(fmt::to_underlying(emphasis_only) == (uint64_t{1} << 54),
                "emphasis-only construction must remain constexpr");
  constexpr auto empty = fmt::text_style{fmt::no_color};
  static_assert(fmt::to_underlying(empty) == 0,
                "an unspecified foreground must produce an empty style");

  const color_t colors[] = {
      fmt::no_color, fmt::rgb{}, fmt::rgb{0xFFFFFF},
      fmt::term_color::black, fmt::term_color::bright_white,
      fmt::color::coral, fmt::cube_color{0}, fmt::cube_color{215},
      fmt::gray_color{0}, fmt::gray_color{23}};
  const fmt::emphasis attributes[] = {
      fmt::emphasis{}, fmt::emphasis::bold | fmt::emphasis::faint,
      fmt::emphasis::strike, static_cast<fmt::emphasis>(0xFF)};
  for (auto foreground : colors) {
    fmt::text_style implicit = foreground;
    EXPECT_EQ(implicit, fmt::fg(foreground));
    for (auto emphasis : attributes) {
      fmt::text_style implicit_emphasis = emphasis;
      EXPECT_EQ(implicit_emphasis, fmt::text_style{emphasis});
      EXPECT_EQ((fmt::text_style{emphasis, foreground}),
                fmt::fg(foreground) | emphasis);
    }
    for (auto background : colors) {
      EXPECT_EQ((fmt::text_style{foreground, background}),
                fmt::fg(foreground) | fmt::bg(background));
      for (auto emphasis : attributes) {
        auto style = fmt::text_style{foreground, background, emphasis};
        EXPECT_EQ((fmt::text_style{emphasis, foreground, background}), style);
        EXPECT_EQ(style, fmt::fg(foreground) | fmt::bg(background) | emphasis);
        EXPECT_EQ(style.get_foreground(), foreground);
        EXPECT_EQ(style.get_background(), background);
        EXPECT_EQ(style.get_emphasis(), emphasis);
      }
    }
  }

  EXPECT_EQ(fmt::text_style{}, fmt::text_style{fmt::emphasis{}});
  EXPECT_EQ(fmt::text_style{fmt::color::red}, fmt::fg(fmt::color::red));
  EXPECT_TRUE(fmt::text_style{fmt::rgb{}}.has_foreground());
  EXPECT_TRUE(fmt::text_style{fmt::term_color::black}.has_foreground());
  fmt::text_style listed = {fmt::color::red, fmt::color::blue,
                           fmt::emphasis::bold};
  EXPECT_EQ(listed, fmt::fg(fmt::color::red) | fmt::bg(fmt::color::blue) |
                        fmt::emphasis::bold);
  fmt::text_style attributes_first = {fmt::emphasis::bold, fmt::color::red,
                                     fmt::color::blue};
  EXPECT_EQ(attributes_first, listed);
  EXPECT_EQ((fmt::text_style{fmt::emphasis::bold, fmt::no_color,
                             fmt::term_color::black}),
            fmt::bg(fmt::term_color::black) | fmt::emphasis::bold);
#if FMT_USE_CONSTEXPR
  constexpr auto style = fmt::text_style{fmt::color::coral,
                                         fmt::term_color::black,
                                         fmt::emphasis::bold};
  static_assert(fmt::to_underlying(style.get_foreground()) ==
                    ((uint32_t{1} << 24) | 0xFF7F50),
                "constant evaluation must preserve the RGB kind and payload");
  static_assert(fmt::to_underlying(style.get_background()) ==
                    (uint32_t{2} << 24),
                "constant evaluation must preserve specified terminal black");
  static_assert(style.get_emphasis() == fmt::emphasis::bold,
                "constant evaluation must preserve emphasis");
  constexpr auto constexpr_attributes_first = fmt::text_style{
      fmt::emphasis::bold, fmt::color::coral, fmt::term_color::black};
  static_assert(constexpr_attributes_first == style,
                "both constructor orders must agree in constant evaluation");
#endif
}

void check_fluent_emphasis(fmt::emphasis attribute,
                          fmt::text_style& (fmt::text_style::*set)(bool),
                          bool (fmt::text_style::*get)() const) {
  for (unsigned bits = 0; bits < 256; ++bits) {
    SCOPED_TRACE(bits);
    auto original = fmt::text_style{static_cast<fmt::emphasis>(bits),
                                    fmt::color::coral,
                                    fmt::term_color::black};
    auto mask = static_cast<unsigned>(attribute);
    EXPECT_EQ((original.*get)(), (bits & mask) != 0);
    for (bool enabled : {false, true}) {
      SCOPED_TRACE(enabled);
      auto style = original;
      auto expected = fmt::text_style{
          static_cast<fmt::emphasis>(enabled ? bits | mask : bits & ~mask),
          fmt::color::coral, fmt::term_color::black};
      EXPECT_EQ(&(style.*set)(enabled), &style);
      EXPECT_EQ(style, expected);
      EXPECT_EQ((style.*get)(), enabled);
      EXPECT_EQ(&(style.*set)(enabled), &style);
      EXPECT_EQ(style, expected);  // Repeating a setter is idempotent.
    }
  }
}

void check_free_emphasis(fmt::emphasis attribute,
                        fmt::text_style (*set)(fmt::text_style, bool)) {
  for (unsigned bits = 0; bits < 256; ++bits) {
    SCOPED_TRACE(bits);
    auto original = fmt::text_style{static_cast<fmt::emphasis>(bits),
                                    fmt::cube_color{1, 2, 3},
                                    fmt::gray_color{12}};
    const auto saved = original;
    const auto mask = static_cast<unsigned>(attribute);
    for (bool enabled : {false, true}) {
      SCOPED_TRACE(enabled);
      const auto result = set(original, enabled);
      EXPECT_EQ(result, (fmt::text_style{
                            static_cast<fmt::emphasis>(
                                enabled ? bits | mask : bits & ~mask),
                            fmt::cube_color{1, 2, 3}, fmt::gray_color{12}}));
      EXPECT_EQ(original, saved);
      EXPECT_EQ(set(result, enabled), result);
    }
  }
}

TEST(color_test, emphasis_factories) {
#define CHECK_EMPHASIS_FACTORY(name)                                      \
  static_assert(std::is_same<decltype(fmt::name()),                        \
                             fmt::text_style>::value,                    \
                "attribute factories must return text_style");           \
  static_assert(noexcept(fmt::name()) && noexcept(fmt::name(true)) &&      \
                    noexcept(fmt::name(false)),                          \
                "attribute factories must be noexcept");                 \
  static_assert(fmt::name() == fmt::text_style{fmt::emphasis::name} &&      \
                    fmt::name(true) == fmt::name(),                       \
                "default and true must set exactly one attribute");       \
  static_assert(fmt::name(false).empty(),                                 \
                "false must produce an empty style");                    \
  for (bool enabled : {false, true}) {                                    \
    const auto style = fmt::name(enabled);                               \
    EXPECT_EQ(style.emph(), enabled ? fmt::emphasis::name                  \
                                   : fmt::emphasis{});                   \
    EXPECT_FALSE(style.has_fg());                                        \
    EXPECT_FALSE(style.has_bg());                                        \
  }
  CHECK_EMPHASIS_FACTORY(bold);
  CHECK_EMPHASIS_FACTORY(faint);
  CHECK_EMPHASIS_FACTORY(italic);
  CHECK_EMPHASIS_FACTORY(underline);
  CHECK_EMPHASIS_FACTORY(blink);
  CHECK_EMPHASIS_FACTORY(reverse);
  CHECK_EMPHASIS_FACTORY(conceal);
  CHECK_EMPHASIS_FACTORY(strike);
#undef CHECK_EMPHASIS_FACTORY
  EXPECT_EQ(fmt::format("{}", fmt::styled(42, fmt::bold())),
            "\x1b[1m42\x1b[0m");
  EXPECT_EQ(fmt::format("{}", fmt::styled(42, fmt::bold(false))), "42");
}

TEST(color_test, free_emphasis) {
#define CHECK_FREE_EMPHASIS(name)                                            \
  static_assert(noexcept(fmt::name(fmt::text_style{})) &&                    \
                    noexcept(fmt::name(fmt::text_style{}, false)),          \
                "free attribute functions must be noexcept");               \
  static_assert(std::is_same<decltype(fmt::name(fmt::text_style{})),          \
                             fmt::text_style>::value,                      \
                "free attribute functions must return a style value");      \
  EXPECT_EQ(fmt::name(fmt::text_style{}),                                    \
            (fmt::text_style{fmt::emphasis::name}));                         \
  EXPECT_EQ(fmt::name(fmt::text_style{}),                                    \
            fmt::name(fmt::text_style{}, true));                            \
  check_free_emphasis(fmt::emphasis::name, &fmt::name)
  CHECK_FREE_EMPHASIS(bold);
  CHECK_FREE_EMPHASIS(faint);
  CHECK_FREE_EMPHASIS(italic);
  CHECK_FREE_EMPHASIS(underline);
  CHECK_FREE_EMPHASIS(blink);
  CHECK_FREE_EMPHASIS(reverse);
  CHECK_FREE_EMPHASIS(conceal);
  CHECK_FREE_EMPHASIS(strike);
#undef CHECK_FREE_EMPHASIS
}

#if FMT_USE_CONSTEXPR
TEST(color_test, free_emphasis_constexpr) {
  constexpr auto base = fmt::text_style{fmt::cube_color{1, 2, 3},
                                       fmt::gray_color{12}};
#define CHECK_FREE_CONSTEXPR(name)                                          \
  constexpr auto name##_style = fmt::name(base);                            \
  static_assert(name##_style.is_##name(), "default must enable attribute"); \
  static_assert(fmt::name(name##_style, false) == base,                     \
                "disabling must restore the original style");              \
  static_assert(name##_style.fg() == base.fg() &&                           \
                    name##_style.bg() == base.bg(),                        \
                "free attribute functions must preserve colors")
  CHECK_FREE_CONSTEXPR(bold);
  CHECK_FREE_CONSTEXPR(faint);
  CHECK_FREE_CONSTEXPR(italic);
  CHECK_FREE_CONSTEXPR(underline);
  CHECK_FREE_CONSTEXPR(blink);
  CHECK_FREE_CONSTEXPR(reverse);
  CHECK_FREE_CONSTEXPR(conceal);
  CHECK_FREE_CONSTEXPR(strike);
#undef CHECK_FREE_CONSTEXPR
  static_assert(!base.has_emph(), "the original style must remain unchanged");
}
#endif

TEST(color_test, fluent_emphasis) {
  using fmt::text_style;
#define CHECK_EMPHASIS(name)                                                 \
  static_assert(noexcept(text_style{}.name()), "setter must be noexcept");   \
  static_assert(noexcept(text_style{}.is_##name()),                          \
                "getter must be noexcept");                                \
  static_assert(!text_style{}.is_##name(), "empty getter must be constexpr"); \
  check_fluent_emphasis(fmt::emphasis::name, &text_style::name,                \
                        &text_style::is_##name)
  CHECK_EMPHASIS(bold);
  CHECK_EMPHASIS(faint);
  CHECK_EMPHASIS(italic);
  CHECK_EMPHASIS(underline);
  CHECK_EMPHASIS(blink);
  CHECK_EMPHASIS(reverse);
  CHECK_EMPHASIS(conceal);
  CHECK_EMPHASIS(strike);
#undef CHECK_EMPHASIS

  auto style = text_style{};
  EXPECT_EQ(&style.bold().faint().italic().underline().blink().reverse()
                 .conceal().strike(), &style);
  EXPECT_EQ(style.emph(), static_cast<fmt::emphasis>(0xFF));
  EXPECT_EQ(&style.bold(false).faint(false).italic(false).underline(false)
                 .blink(false).reverse(false).conceal(false)
                 .strike(false), &style);
  EXPECT_EQ(style, text_style{});

  style.bold().faint();
  EXPECT_TRUE(style.is_bold());
  EXPECT_TRUE(style.is_faint());
  EXPECT_EQ(fmt::format(style, "both"), "\x1b[1;2mboth\x1b[0m");
  style.bold(false);
  EXPECT_FALSE(style.is_bold());
  EXPECT_TRUE(style.is_faint());
  style.bold().faint(false);
  EXPECT_TRUE(style.is_bold());
  EXPECT_FALSE(style.is_faint());
}

TEST(color_test, fluent_components) {
  using fmt::color_t;
  using fmt::emphasis;
  using fmt::text_style;
  constexpr auto empty = text_style{};
  static_assert(fmt::to_underlying(empty.fg()) == 0,
                "empty foreground getter must be constexpr");
  static_assert(fmt::to_underlying(empty.bg()) == 0,
                "empty background getter must be constexpr");
  static_assert(empty.emph() == emphasis{},
                "empty emphasis getter must be constexpr");
  constexpr auto filled = text_style{emphasis::strike};
  static_assert(filled.emph() == emphasis::strike &&
                    filled.get_emphasis() == emphasis::strike,
                "both emphasis getters must be constexpr in C++11");
  static_assert(noexcept(empty.fg()) && noexcept(empty.bg()) &&
                    noexcept(empty.emph()), "getters must be noexcept");
  static_assert(noexcept(text_style{}.fg(fmt::no_color)) &&
                    noexcept(text_style{}.bg(fmt::no_color)) &&
                    noexcept(text_style{}.emph(emphasis{})),
                "setters must be noexcept");

  const color_t colors[] = {
      fmt::no_color, fmt::rgb{}, fmt::color::coral,
      fmt::term_color::black, fmt::term_color::bright_white};
  for (auto foreground : colors) {
    for (auto background : colors) {
      const auto original = text_style{
          static_cast<emphasis>(0xFF), foreground, background};
      EXPECT_EQ(original.fg(), original.get_foreground());
      EXPECT_EQ(original.bg(), original.get_background());
      EXPECT_EQ(original.emph(), original.get_emphasis());
      for (auto replacement : colors) {
        auto style = original;
        EXPECT_EQ(&style.fg(replacement), &style);
        EXPECT_EQ(style, (text_style{static_cast<emphasis>(0xFF),
                                     replacement, background}));
        EXPECT_EQ(style.fg(), replacement);
        EXPECT_EQ(style.has_foreground(), replacement != color_t{});
        style = original;
        EXPECT_EQ(&style.bg(replacement), &style);
        EXPECT_EQ(style, (text_style{static_cast<emphasis>(0xFF),
                                     foreground, replacement}));
        EXPECT_EQ(style.bg(), replacement);
        EXPECT_EQ(style.has_background(), replacement != color_t{});
      }
      auto style = original;
      auto selected = emphasis::bold | emphasis::blink | emphasis::strike;
      EXPECT_EQ(&style.emph(selected), &style);
      EXPECT_EQ(style, (text_style{selected, foreground, background}));
      EXPECT_EQ(style.emph(), selected);
      EXPECT_EQ(&style.emph(emphasis{}), &style);
      EXPECT_EQ(style, (text_style{foreground, background}));
      EXPECT_FALSE(style.has_emphasis());
    }
  }

  auto style = text_style{};
  EXPECT_EQ(&style.fg(fmt::color::coral).bg(fmt::term_color::black)
                 .emph(emphasis::italic).underline(), &style);
  EXPECT_EQ(style, (text_style{emphasis::italic | emphasis::underline,
                               fmt::color::coral, fmt::term_color::black}));
  EXPECT_EQ(&style.fg(fmt::no_color).bg(fmt::no_color).emph(emphasis{}), &style);
  EXPECT_EQ(style, empty);
  EXPECT_EQ(fmt::format(style, "plain"), "plain");
}

TEST(color_test, fluent_presence) {
  using fmt::emphasis;
  using fmt::text_style;
  constexpr auto empty = text_style{};
  static_assert(!empty.has_fg() && !empty.has_bg() && !empty.has_emph(),
                "empty presence queries must be constexpr in C++11");
  static_assert(noexcept(empty.has_fg()) && noexcept(empty.has_bg()) &&
                    noexcept(empty.has_emph()), "presence queries must be noexcept");
  static_assert(std::is_same<decltype(empty.has_fg()), bool>::value &&
                    std::is_same<decltype(empty.has_bg()), bool>::value &&
                    std::is_same<decltype(empty.has_emph()), bool>::value,
                "presence queries must return bool");
  constexpr auto high_bit = text_style{emphasis::strike};
  static_assert(high_bit.has_emph() && !high_bit.has_fg() && !high_bit.has_bg(),
                "emphasis presence must be constexpr and independent of colors");

  const struct {
    fmt::color_t value;
    bool specified;
  } colors[] = {{fmt::no_color, false}, {fmt::rgb{}, true},
                {fmt::term_color::black, true}, {fmt::color::coral, true},
                {fmt::term_color::bright_white, true}};
  const emphasis attributes[] = {
      emphasis{}, emphasis::bold, emphasis::faint, emphasis::italic,
      emphasis::underline, emphasis::blink, emphasis::reverse,
      emphasis::conceal, emphasis::strike, static_cast<emphasis>(0xFF)};
  for (const auto& foreground : colors) {
    for (const auto& background : colors) {
      for (auto attribute : attributes) {
        const auto style = text_style{attribute, foreground.value,
                                       background.value};
        EXPECT_EQ(style.has_fg(), foreground.specified);
        EXPECT_EQ(style.has_bg(), background.specified);
        EXPECT_EQ(style.has_emph(), attribute != emphasis{});
        EXPECT_EQ(style.has_fg(), style.has_foreground());
        EXPECT_EQ(style.has_bg(), style.has_background());
        EXPECT_EQ(style.has_emph(), style.has_emphasis());
      }
    }
  }

  auto style = text_style{emphasis::bold, fmt::rgb{}, fmt::term_color::black};
  style.fg(fmt::no_color);
  EXPECT_FALSE(style.has_fg());
  EXPECT_TRUE(style.has_bg());
  EXPECT_TRUE(style.has_emph());
  style.bg(fmt::no_color);
  EXPECT_FALSE(style.has_fg());
  EXPECT_FALSE(style.has_bg());
  EXPECT_TRUE(style.has_emph());
  style.bold(false);
  EXPECT_FALSE(style.has_emph());
  style.emph(emphasis::faint).fg(fmt::term_color::black).bg(fmt::rgb{});
  EXPECT_TRUE(style.has_fg());
  EXPECT_TRUE(style.has_bg());
  EXPECT_TRUE(style.has_emph());
  style.emph(emphasis{});
  EXPECT_TRUE(style.has_fg());
  EXPECT_TRUE(style.has_bg());
  EXPECT_FALSE(style.has_emph());
}

#if FMT_USE_CONSTEXPR
TEST(color_test, fluent_constexpr) {
  constexpr auto all = fmt::text_style{}.bold().faint().italic().underline()
                          .blink().reverse().conceal().strike();
  static_assert(all.is_bold() && all.is_faint() && all.is_italic() &&
                    all.is_underline() && all.is_blink() && all.is_reverse() &&
                    all.is_conceal() && all.is_strike(),
                "all emphasis setters must support constant evaluation");
  constexpr auto cleared = fmt::text_style{all}.bold(false).faint(false)
                              .italic(false).underline(false).blink(false)
                              .reverse(false).conceal(false).strike(false);
  static_assert(!cleared.has_emphasis(),
                "disabling emphasis must support constant evaluation");
  constexpr auto replaced = fmt::text_style{all}.fg(fmt::color::coral)
                               .bg(fmt::term_color::black)
                               .emph(fmt::emphasis::underline);
  static_assert(replaced == fmt::text_style{fmt::emphasis::underline,
                                             fmt::color::coral,
                                             fmt::term_color::black},
                "component replacement must support constant evaluation");
  static_assert(replaced.has_fg() && replaced.has_bg() && replaced.has_emph(),
                "presence queries must detect constexpr component replacement");
  constexpr auto empty = fmt::text_style{replaced}.fg(fmt::no_color)
                            .bg(fmt::no_color).emph(fmt::emphasis{});
  static_assert(empty == fmt::text_style{},
                "clearing components must support constant evaluation");
  static_assert(!empty.has_fg() && !empty.has_bg() && !empty.has_emph(),
                "presence queries must detect constexpr component clearing");
  constexpr auto cleared_style = fmt::text_style{replaced}.clear();
  static_assert(cleared_style.empty() && cleared_style == fmt::text_style{},
                "clear must restore an empty style during constant evaluation");
  constexpr auto reused = fmt::text_style{replaced}.clear().strike();
  static_assert(reused == fmt::text_style{fmt::emphasis::strike},
                "clear must support fluent reuse during constant evaluation");
}
#endif

TEST(color_test, text_style) {
  EXPECT_FALSE(fmt::text_style().has_foreground());
  EXPECT_FALSE(fmt::text_style().has_background());
  EXPECT_FALSE(fmt::text_style().has_emphasis());

  EXPECT_TRUE(fg(fmt::rgb(0)).has_foreground());
  EXPECT_FALSE(fg(fmt::rgb(0)).has_background());
  EXPECT_FALSE(fg(fmt::rgb(0)).has_emphasis());
  EXPECT_TRUE(bg(fmt::rgb(0)).has_background());
  EXPECT_FALSE(bg(fmt::rgb(0)).has_foreground());
  EXPECT_FALSE(bg(fmt::rgb(0)).has_emphasis());

  EXPECT_TRUE(
      (fg(fmt::rgb(0xFFFFFF)) | bg(fmt::rgb(0xFFFFFF))).has_foreground());
  EXPECT_TRUE(
      (fg(fmt::rgb(0xFFFFFF)) | bg(fmt::rgb(0xFFFFFF))).has_background());
  EXPECT_FALSE(
      (fg(fmt::rgb(0xFFFFFF)) | bg(fmt::rgb(0xFFFFFF))).has_emphasis());

  EXPECT_EQ(fg(fmt::rgb(0x000000)) | fg(fmt::rgb(0x000000)),
            fg(fmt::rgb(0x000000)));
  EXPECT_EQ(fg(fmt::rgb(0x00000F)) | fg(fmt::rgb(0x00000F)),
            fg(fmt::rgb(0x00000F)));
  EXPECT_EQ(fg(fmt::rgb(0xC0F000)) | fg(fmt::rgb(0x000FEE)),
            fg(fmt::rgb(0xC0FFEE)));

  EXPECT_EQ(fg(fmt::terminal_color::black) | fg(fmt::terminal_color::black),
            fg(fmt::rgb(0)));
  EXPECT_EQ(fg(fmt::terminal_color::black) | fg(fmt::terminal_color::white),
            fg(fmt::rgb(0xE5E5E5)));
  EXPECT_EQ(bg(fmt::terminal_color::black) | bg(fmt::terminal_color::black),
            bg(fmt::rgb(0)));
  EXPECT_EQ(bg(fmt::terminal_color::black) | bg(fmt::terminal_color::white),
            bg(fmt::rgb(0xE5E5E5)));
  EXPECT_EQ(fg(fmt::terminal_color::black) | fg(fmt::color::black),
            fg(fmt::rgb(0)));
  EXPECT_EQ(bg(fmt::terminal_color::black) | bg(fmt::color::black),
            bg(fmt::rgb(0)));

  EXPECT_NO_THROW((void) (fg(fmt::terminal_color::white) |
                          bg(fmt::terminal_color::white)));
  EXPECT_NO_THROW(
      (void) (fg(fmt::terminal_color::white) | bg(fmt::rgb(0xFFFFFF))));
  EXPECT_NO_THROW((void) (fg(fmt::terminal_color::white) | fmt::text_style()));
  EXPECT_NO_THROW((void) (bg(fmt::terminal_color::white) | fmt::text_style()));
}

TEST(color_test, short_names_compatibility) {
  using fmt::emphasis;
  using fmt::term_color;
  using fmt::text_style;
  static_assert(std::is_same<term_color, fmt::terminal_color>::value,
                "terminal_color must remain an alias");
  static_assert(emphasis::strike == emphasis::strikethrough,
                "both names must select the same emphasis bit");
  static_assert(text_style{emphasis::strike}.is_strikethrough() &&
                    text_style{emphasis::strikethrough}.is_strike(),
                "both queries must recognize either emphasis name");

  for (unsigned i = 0; i < 16; ++i) {
    const auto color = static_cast<term_color>(i);
    const auto legacy = static_cast<fmt::terminal_color>(i);
    const auto stored = fmt::color_t{color};
    EXPECT_EQ(fmt::get<fmt::terminal_color>(stored), color);
    EXPECT_TRUE(fmt::holds_alternative<fmt::terminal_color>(stored));
    EXPECT_EQ(fmt::get<term_color>(
                  fmt::color_t{legacy}), color);
    EXPECT_EQ(fmt::format(fg(color), "fg"), fmt::format(fg(legacy), "fg"));
    EXPECT_EQ(fmt::format(bg(color), "bg"), fmt::format(bg(legacy), "bg"));
  }

  auto style = text_style{emphasis::bold, term_color::red,
                         fmt::terminal_color::black};
  EXPECT_EQ(&style.strike(), &style);
  EXPECT_TRUE(style.is_strikethrough());
  EXPECT_EQ(fmt::format(style, "mixed"),
            FMT_SGR_EXPECTED(
                "\x1b[1;9m\x1b[31m\x1b[40mmixed\x1b[0m",
                "\x1b[1;9;31;40mmixed\x1b[0m"));
  EXPECT_EQ(&style.strikethrough(false), &style);
  EXPECT_FALSE(style.is_strike());
  style.strikethrough().strike(false);
  EXPECT_FALSE(style.is_strikethrough());
  EXPECT_EQ(style, (text_style{emphasis::bold, term_color::red,
                              fmt::terminal_color::black}));
  EXPECT_EQ(fmt::format(emphasis::strike, "strike"),
            "\x1b[9mstrike\x1b[0m");
#if FMT_USE_CONSTEXPR
  constexpr auto cleared = text_style{}.strike().strikethrough(false);
  static_assert(!cleared.is_strike() && !cleared.is_strikethrough(),
                "mixed setters must support constant evaluation");
#endif
}

TEST(color_test, term_color_boundaries) {
  using fmt::term_color;
  EXPECT_EQ(static_cast<int>(term_color::black), 0);
  EXPECT_EQ(static_cast<int>(term_color::white), 7);
  EXPECT_EQ(static_cast<int>(term_color::bright_black), 8);
  EXPECT_EQ(static_cast<int>(term_color::bright_white), 15);

  EXPECT_TRUE(fg(term_color::black).has_foreground());
  EXPECT_TRUE(bg(term_color::black).has_background());

  EXPECT_EQ(fmt::format(fg(term_color::black), "black"),
            "\x1b[30mblack\x1b[0m");
  EXPECT_EQ(fmt::format(bg(term_color::black), "black"),
            "\x1b[40mblack\x1b[0m");
  EXPECT_EQ(fmt::format(fg(term_color::white), "white"),
            "\x1b[37mwhite\x1b[0m");
  EXPECT_EQ(fmt::format(bg(term_color::white), "white"),
            "\x1b[47mwhite\x1b[0m");
  EXPECT_EQ(fmt::format(fg(term_color::bright_black), "bright black"),
            "\x1b[90mbright black\x1b[0m");
  EXPECT_EQ(fmt::format(bg(term_color::bright_black), "bright black"),
            "\x1b[100mbright black\x1b[0m");
  EXPECT_EQ(fmt::format(fg(term_color::bright_white), "bright white"),
            "\x1b[97mbright white\x1b[0m");
  EXPECT_EQ(fmt::format(bg(term_color::bright_white), "bright white"),
            "\x1b[107mbright white\x1b[0m");
}

TEST(color_test, format) {
  EXPECT_EQ(fmt::format(fmt::text_style(), "no style"), "no style");
  EXPECT_EQ(fmt::format(fg(fmt::rgb(255, 20, 30)), "rgb(255,20,30)"),
            FMT_SGR_EXPECTED(
                "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m",
                "\x1b[38;2;255;20;30mrgb(255,20,30)\x1b[0m"));
  EXPECT_EQ(fmt::format(fg(fmt::rgb(255, 0, 0)) | fg(fmt::rgb(0, 20, 30)),
                        "rgb(255,20,30)"),
            FMT_SGR_EXPECTED(
                "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m",
                "\x1b[38;2;255;20;30mrgb(255,20,30)\x1b[0m"));
  EXPECT_EQ(
      fmt::format(fg(fmt::rgb(0, 0, 0)) | fg(fmt::rgb(0, 0, 0)), "rgb(0,0,0)"),
      FMT_SGR_EXPECTED(
          "\x1b[38;2;000;000;000mrgb(0,0,0)\x1b[0m",
          "\x1b[38;2;0;0;0mrgb(0,0,0)\x1b[0m"));
  EXPECT_EQ(fmt::format(fg(fmt::color::blue), "blue"),
            FMT_SGR_EXPECTED(
                "\x1b[38;2;000;000;255mblue\x1b[0m",
                "\x1b[38;2;0;0;255mblue\x1b[0m"));
  EXPECT_EQ(
      fmt::format(fg(fmt::color::blue) | bg(fmt::color::red), "two color"),
      FMT_SGR_EXPECTED(
          "\x1b[38;2;000;000;255m\x1b[48;2;255;000;000mtwo color\x1b[0m",
          "\x1b[38;2;0;0;255;48;2;255;0;0mtwo color\x1b[0m"));
  EXPECT_EQ(fmt::format(fmt::emphasis::bold, "bold"), "\x1b[1mbold\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::faint, "faint"), "\x1b[2mfaint\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::italic, "italic"),
            "\x1b[3mitalic\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::underline, "underline"),
            "\x1b[4munderline\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::blink, "blink"), "\x1b[5mblink\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::reverse, "reverse"),
            "\x1b[7mreverse\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::conceal, "conceal"),
            "\x1b[8mconceal\x1b[0m");
  EXPECT_EQ(fmt::format(fmt::emphasis::strikethrough, "strikethrough"),
            "\x1b[9mstrikethrough\x1b[0m");
  EXPECT_EQ(
      fmt::format(fg(fmt::color::blue) | fmt::emphasis::bold, "blue/bold"),
      FMT_SGR_EXPECTED(
          "\x1b[1m\x1b[38;2;000;000;255mblue/bold\x1b[0m",
          "\x1b[1;38;2;0;0;255mblue/bold\x1b[0m"));
  EXPECT_EQ(fmt::format(fmt::emphasis::bold, "bold error"),
            "\x1b[1mbold error\x1b[0m");
  EXPECT_EQ(fmt::format(fg(fmt::color::blue), "blue log"),
            FMT_SGR_EXPECTED(
                "\x1b[38;2;000;000;255mblue log\x1b[0m",
                "\x1b[38;2;0;0;255mblue log\x1b[0m"));
  EXPECT_EQ(fmt::format(fmt::text_style(), "hi"), "hi");
  EXPECT_EQ(fmt::format(fg(fmt::terminal_color::red), "tred"),
            "\x1b[31mtred\x1b[0m");
  EXPECT_EQ(fmt::format(bg(fmt::terminal_color::cyan), "tcyan"),
            "\x1b[46mtcyan\x1b[0m");
  EXPECT_EQ(fmt::format(fg(fmt::terminal_color::bright_green), "tbgreen"),
            "\x1b[92mtbgreen\x1b[0m");
  EXPECT_EQ(fmt::format(bg(fmt::terminal_color::bright_magenta), "tbmagenta"),
            "\x1b[105mtbmagenta\x1b[0m");
  EXPECT_EQ(fmt::format(fg(fmt::terminal_color::red), "{}", "foo"),
            "\x1b[31mfoo\x1b[0m");
  EXPECT_EQ(fmt::format("{}{}", fmt::styled("red", fg(fmt::color::red)),
                        fmt::styled("bold", fmt::emphasis::bold)),
            FMT_SGR_EXPECTED(
                "\x1b[38;2;255;000;000mred\x1b[0m\x1b[1mbold\x1b[0m",
                "\x1b[38;2;255;0;0mred\x1b[0m\x1b[1mbold\x1b[0m"));
  EXPECT_EQ(fmt::format("{}", fmt::styled("bar", fg(fmt::color::blue) |
                                                     fmt::emphasis::underline)),
            FMT_SGR_EXPECTED(
                "\x1b[4m\x1b[38;2;000;000;255mbar\x1b[0m",
                "\x1b[4;38;2;0;0;255mbar\x1b[0m"));
  EXPECT_EQ(
      fmt::format(
          "{}", fmt::styled(
                    "all", fmt::emphasis::bold | fmt::emphasis::faint |
                               fmt::emphasis::italic |
                               fmt::emphasis::underline | fmt::emphasis::blink |
                               fmt::emphasis::reverse | fmt::emphasis::conceal |
                               fmt::emphasis::strikethrough)),
      "\x1b[1;2;3;4;5;7;8;9mall\x1b[0m");
}

TEST(color_test, format_to) {
  auto out = std::string();
  fmt::format_to(std::back_inserter(out), fg(fmt::rgb(255, 20, 30)),
                 "rgb(255,20,30){}{}{}", 1, 2, 3);
  EXPECT_EQ(fmt::to_string(out),
            FMT_SGR_EXPECTED(
                "\x1b[38;2;255;020;030mrgb(255,20,30)123\x1b[0m",
                "\x1b[38;2;255;20;30mrgb(255,20,30)123\x1b[0m"));
}

TEST(color_test, print) {
  EXPECT_WRITE(stdout, fmt::print(fg(fmt::rgb(255, 20, 30)), "rgb(255,20,30)"),
               FMT_SGR_EXPECTED(
                   "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m",
                   "\x1b[38;2;255;20;30mrgb(255,20,30)\x1b[0m"));
}

TEST(color_test, println) {
  EXPECT_WRITE(stdout,
               fmt::println(fg(fmt::rgb(255, 20, 30)), "rgb(255,20,30)"),
               FMT_SGR_EXPECTED(
                   "\x1b[38;2;255;020;030mrgb(255,20,30)\x1b[0m\n",
                   "\x1b[38;2;255;20;30mrgb(255,20,30)\x1b[0m\n"));
  EXPECT_WRITE(stdout, fmt::println(fmt::emphasis::bold, "bold"),
               "\x1b[1mbold\x1b[0m\n");
  EXPECT_WRITE(
      stdout,
      fmt::println(fg(fmt::color::blue) | fmt::emphasis::bold, "blue/bold"),
      FMT_SGR_EXPECTED(
          "\x1b[1m\x1b[38;2;000;000;255mblue/bold\x1b[0m\n",
          "\x1b[1;38;2;0;0;255mblue/bold\x1b[0m\n"));
}

#if FMT_COLOR_COMPACT_SGR == 1
TEST(color_test, compact_sgr_combined_style) {
  const auto style = fmt::text_style{static_cast<fmt::emphasis>(0xff),
                                    fmt::rgb{0, 9, 255}, fmt::cube_color{0}};
  const auto sgr = std::string{
      "\x1b[1;2;3;4;5;7;8;9;38;2;0;9;255;48;5;16m"};
  check_to_sgr(style, sgr);
  EXPECT_EQ(fmt::format(style, "value"), sgr + "value\x1b[0m");
  EXPECT_EQ(fmt::format("{:>5}", fmt::styled(42, style)),
            sgr + "   42\x1b[0m");
  EXPECT_EQ(fmt::format("{}plain", fmt::styled("value", style)),
            sgr + "value\x1b[0mplain");

  // Appending a style must preserve existing buffer contents.
  auto buf = fmt::memory_buffer();
  const auto prefix = std::string{"prefix"};
  buf.append(prefix.data(), prefix.data() + prefix.size());
  fmt::detail::append_sgr(buf, style);
  EXPECT_EQ(fmt::to_string(buf), prefix + sgr);
}

TEST(color_test, compact_sgr_empty_and_single_components) {
  check_to_sgr(fmt::text_style{}, "");
  EXPECT_EQ(fmt::format(fmt::text_style{}, "plain"), "plain");
  EXPECT_EQ(fmt::format("{:>5}", fmt::styled(42, fmt::text_style{})), "   42");
  check_to_sgr(fmt::text_style{fmt::emphasis::bold}, "\x1b[1m");
  check_to_sgr(fmt::fg(fmt::term_color::black), "\x1b[30m");
  check_to_sgr(fmt::bg(fmt::term_color::bright_white), "\x1b[107m");
  check_to_sgr(fmt::fg(fmt::gray_color{0}), "\x1b[38;5;232m");
  check_to_sgr(fmt::bg(fmt::gray_color{23}), "\x1b[48;5;255m");
}

TEST(color_test, compact_sgr_decimal_channels) {
  for (unsigned n = 0; n < 256; ++n) {
    const auto channel = static_cast<uint8_t>(n);
    const auto rgb = fmt::rgb{channel, channel, channel};
    const auto value = std::to_string(n);
    const auto parameters = value + ";" + value + ";" + value;
    check_to_sgr(fmt::fg(rgb), "\x1b[38;2;" + parameters + "m");
    check_to_sgr(fmt::bg(rgb), "\x1b[48;2;" + parameters + "m");
  }
}
#endif

namespace {
void check_wrap_sgr(const fmt::text_style& style, const std::string& text,
                   const std::string& expected) {
  const auto original_style = style;
  const auto original_text = text;
  EXPECT_EQ(fmt::wrap_sgr(style, text), expected);
  EXPECT_EQ(fmt::wrap_sgr<char>(style, fmt::string_view{text.data(), text.size()}),
            expected);
  const auto wide_text = std::wstring{text.begin(), text.end()};
  EXPECT_EQ(fmt::wrap_sgr<wchar_t>(
                style, fmt::basic_string_view<wchar_t>{wide_text.data(),
                                                     wide_text.size()}),
            (std::wstring{expected.begin(), expected.end()}));
  EXPECT_EQ(style, original_style);
  EXPECT_EQ(text, original_text);
}
}  // namespace

TEST(color_test, wrap_sgr) {
  static_assert(std::is_same<decltype(fmt::wrap_sgr(fmt::text_style{}, "text")),
                             std::string>::value,
                "the convenience overload must return std::string");
  static_assert(std::is_same<decltype(fmt::wrap_sgr<wchar_t>(
                                fmt::text_style{},
                                fmt::basic_string_view<wchar_t>{L"text"})),
                             std::wstring>::value,
                "wide wrapping must return std::wstring");
  EXPECT_EQ(fmt::wrap_sgr(fmt::text_style{fmt::emphasis::bold}, "literal"),
            "\x1b[1mliteral\x1b[0m");
  check_wrap_sgr(fmt::text_style{}, "value", "value");
  check_wrap_sgr(fmt::text_style{fmt::emphasis::bold}, "value",
                 "\x1b[1mvalue\x1b[0m");
  check_wrap_sgr(fmt::text_style{fmt::rgb{1, 2, 3}}, "value",
                 FMT_SGR_EXPECTED("\x1b[38;2;001;002;003mvalue\x1b[0m",
                                  "\x1b[38;2;1;2;3mvalue\x1b[0m"));
  check_wrap_sgr(fmt::text_style{fmt::term_color::black,
                                 fmt::term_color::bright_white},
                 "value", FMT_SGR_EXPECTED("\x1b[30m\x1b[107mvalue\x1b[0m",
                                            "\x1b[30;107mvalue\x1b[0m"));
  check_wrap_sgr(fmt::text_style{fmt::cube_color{0}, fmt::gray_color{23}},
                 "value", FMT_SGR_EXPECTED(
                              "\x1b[38;5;016m\x1b[48;5;255mvalue\x1b[0m",
                              "\x1b[38;5;16;48;5;255mvalue\x1b[0m"));
  check_wrap_sgr(fmt::text_style{static_cast<fmt::emphasis>(0xff),
                                 fmt::rgb{255, 255, 255},
                                 fmt::rgb{255, 255, 255}},
                 "value", FMT_SGR_EXPECTED(
                              "\x1b[1;2;3;4;5;7;8;9m\x1b[38;2;255;255;255m"
                              "\x1b[48;2;255;255;255mvalue\x1b[0m",
                              "\x1b[1;2;3;4;5;7;8;9;38;2;255;255;255;"
                              "48;2;255;255;255mvalue\x1b[0m"));
}

TEST(color_test, wrap_sgr_empty) {
  const fmt::text_style styles[] = {fmt::text_style{},
                                    fmt::text_style{fmt::emphasis::bold}};
  for (const auto& style : styles) {
    EXPECT_EQ(fmt::wrap_sgr(style, fmt::string_view{}), "");
    EXPECT_EQ(fmt::wrap_sgr(style, ""), "");
    EXPECT_EQ(fmt::wrap_sgr<wchar_t>(style, fmt::basic_string_view<wchar_t>{}),
              L"");
    EXPECT_EQ(fmt::wrap_sgr<wchar_t>(style, L""), L"");
  }
}

TEST(color_test, wrap_sgr_literal_payload) {
  const auto style = fmt::text_style{fmt::emphasis::bold};
  const char bytes[] = "before\0{}\x1b[31mafter";
  const auto text = std::string{bytes, sizeof(bytes) - 1};
  check_wrap_sgr(style, text, "\x1b[1m" + text + "\x1b[0m");
  check_wrap_sgr(fmt::text_style{}, text, text);
  const auto long_text = std::string(4096, 'x');
  check_wrap_sgr(style, long_text, "\x1b[1m" + long_text + "\x1b[0m");
  EXPECT_EQ(fmt::wrap_sgr<wchar_t>(style, L"wide \u03a9"),
            L"\x1b[1mwide \u03a9\x1b[0m");

  auto source = std::string{"owned"};
  const auto wrapped = fmt::wrap_sgr(style, source);
  source.assign("changed");
  EXPECT_EQ(wrapped, "\x1b[1mowned\x1b[0m");
}

TEST(color_test, absolute_style_sgr) {
  const auto reset = fmt::text_style{}.absolute();
  check_to_sgr(reset, "\x1b[0m");
  const auto style = fmt::underline().absolute();
  const auto prefix = std::string{FMT_SGR_EXPECTED("\x1b[0m\x1b[4m",
                                                  "\x1b[0;4m")};
  check_to_sgr(style, prefix);
  EXPECT_EQ(fmt::format(style, "text"), prefix + "text\x1b[0m");
  // Styled arguments honor the explicit absolute reset in both modes.
  EXPECT_EQ(fmt::format("{}", fmt::styled("text", style)),
            prefix + "text\x1b[0m");
  EXPECT_EQ(fmt::wrap_sgr(reset, "text"), "\x1b[0mtext\x1b[0m");
  EXPECT_EQ(fmt::wrap_sgr<wchar_t>(reset, L"text"),
            L"\x1b[0mtext\x1b[0m");
  const auto maximal = fmt::text_style{fmt::rgb{255, 255, 255},
      fmt::rgb{255, 255, 255}, static_cast<fmt::emphasis>(0xFF)}.absolute();
  const auto expected = std::string{FMT_SGR_EXPECTED(
      "\x1b[0m\x1b[1;2;3;4;5;7;8;9m"
      "\x1b[38;2;255;255;255m\x1b[48;2;255;255;255m",
      "\x1b[0;1;2;3;4;5;7;8;9;38;2;255;255;255;48;2;255;255;255m")};
  check_to_sgr(maximal, expected);
  EXPECT_EQ(expected.size(), FMT_SGR_EXPECTED(size_t{60}, size_t{54}));
  EXPECT_LE(expected.size(), fmt::sgr_max_length);
}

#undef FMT_SGR_EXPECTED
