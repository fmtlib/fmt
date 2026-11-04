// Formatting library for C++ - color support
//
// Copyright (c) 2018 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

// Currently being refactored by Terry Golubiewski.

#ifndef FMT_COLOR_H_
#define FMT_COLOR_H_

#include "format.h"

#ifndef FMT_MODULE
#  include <functional>  // std::hash
#endif

#if defined(__cpp_impl_three_way_comparison) && \
    __cpp_impl_three_way_comparison >= 201907L && FMT_HAS_INCLUDE(<compare>)
#  include <compare>
#endif

#ifndef FMT_COLOR_COMPACT_SGR
#define FMT_COLOR_COMPACT_SGR 0
#endif

FMT_BEGIN_NAMESPACE
FMT_BEGIN_EXPORT

/// A narrowing static_cast.
template<typename T, typename U>
constexpr inline auto narrow_cast(U x) noexcept -> T
{ return static_cast<T>(x); }

/// An enum-related static_cast.
template<typename T, typename U>
constexpr inline auto enum_cast(U x) noexcept -> T
{ return static_cast<T>(x); }

/// Provides the integer representation as the member type `type`.
template<typename T>
struct underlying_type { using type = void; };

/**
 * Indicates an unspecified color, distinct from RGB or terminal black.
 *
 * Use `no_color` with `fg()` or `bg()` to construct an unspecified channel.
 */
struct no_color_t {};

template<> struct underlying_type<no_color_t> { using type = unsigned; };

FMT_NODISCARD constexpr auto to_underlying(no_color_t) noexcept -> unsigned
{ return 0u; }

/// The unspecified-color value.
FMT_INLINE_VARIABLE constexpr no_color_t no_color{};


/**
 * The eight normal and eight bright colors of the terminal's palette.
 *
 * Pass a value to `fg()` or `bg()` to select a foreground or background color.
 * The actual RGB values depend on the terminal's palette and theme.
 *
 * Values are palette indices: 0-7 for normal colors and 8-15 for bright colors,
 * in declaration order.  Prefer symbolic names when selecting colors.
 */
enum class term_color: int8_t {
  black,
  red,
  green,
  yellow,
  blue,
  magenta,
  cyan,
  white,
  bright_black,
  bright_red,
  bright_green,
  bright_yellow,
  bright_blue,
  bright_magenta,
  bright_cyan,
  bright_white
}; // term_color

/// Alias for `term_color`, retained for backward compatibility.
using terminal_color = term_color;

template<> struct underlying_type<term_color> { using type = int8_t; };

FMT_NODISCARD constexpr auto to_underlying(term_color x) noexcept -> int8_t
{ return enum_cast<int8_t>(x); }
/**
 * Independent text attributes that can be combined with `operator|`.
 * `emphasis()` specifies no attributes.
 *
 * The enum values are bit masks. Attribute support and the appearance of
 * combinations depend on the terminal.
 */
enum class emphasis : uint8_t {
  bold          = 1 << 0, ///< Increased intensity (SGR 1).
  faint         = 1 << 1, ///< Decreased intensity (SGR 2).
  italic        = 1 << 2, ///< Italic text         (SGR 3).
  underline     = 1 << 3, ///< Underlined text     (SGR 4).
  blink         = 1 << 4, ///< Blinking text       (SGR 5).
  reverse       = 1 << 5, ///< Reversed fg and bg  (SGR 7).
  conceal       = 1 << 6, ///< Concealed text      (SGR 8).
  strike        = 1 << 7, ///< Crossed-out text    (SGR 9).
  strikethrough = strike  ///< Alias for `strike`, for backward compatibility.
}; // emphasis

template<> struct underlying_type<emphasis> { using type = uint8_t; };

FMT_NODISCARD constexpr auto to_underlying(emphasis x) noexcept -> uint8_t
{ return enum_cast<uint8_t>(x); }

FMT_NODISCARD
constexpr inline auto operator|(emphasis lhs, emphasis rhs) noexcept
  -> emphasis
{ return enum_cast<emphasis>(to_underlying(lhs) | to_underlying(rhs)); }

FMT_CONSTEXPR inline auto operator|=(emphasis& lhs, emphasis rhs) noexcept
  -> emphasis&
{ return lhs = lhs | rhs; }

#if defined(__cpp_lib_three_way_comparison) && \
    __cpp_lib_three_way_comparison >= 201907L

#define FMT_COLOR_COMPARISONS(C) \
  FMT_PRAGMA_CLANG(diagnostic push) \
  FMT_PRAGMA_CLANG(diagnostic ignored "-Wzero-as-null-pointer-constant") \
  FMT_NODISCARD constexpr auto operator<=>(const C&) const noexcept \
    -> std::strong_ordering = default; \
  FMT_PRAGMA_CLANG(diagnostic pop)

#else

#define FMT_COLOR_COMPARISONS(C) \
  FMT_NODISCARD constexpr auto operator==(const C& rhs) const noexcept -> bool \
  { return to_underlying(*this) == to_underlying(rhs); } \
  FMT_NODISCARD constexpr auto operator!=(const C& rhs) const noexcept -> bool \
  { return to_underlying(*this) != to_underlying(rhs); } \
  FMT_NODISCARD constexpr auto operator< (const C& rhs) const noexcept -> bool \
  { return to_underlying(*this) <  to_underlying(rhs); } \
  FMT_NODISCARD constexpr auto operator<=(const C& rhs) const noexcept -> bool \
  { return to_underlying(*this) <= to_underlying(rhs); } \
  FMT_NODISCARD constexpr auto operator> (const C& rhs) const noexcept -> bool \
  { return to_underlying(*this) >  to_underlying(rhs); } \
  FMT_NODISCARD constexpr auto operator>=(const C& rhs) const noexcept -> bool \
  { return to_underlying(*this) >= to_underlying(rhs); }

#endif

/// An RGB color with red, green, and blue components in the range 0-255.
struct rgb {
  using value_type = uint32_t;
  using chan_type  = uint8_t;

  chan_type r = 0;  ///< Red   component
  chan_type g = 0;  ///< Green component
  chan_type b = 0;  ///< Blue  component

private:

  FMT_NODISCARD constexpr auto value() const noexcept -> value_type {
    return (value_type{r} << 16)
         | (value_type{g} <<  8)
         | (value_type{b} <<  0);
  }

  FMT_NODISCARD static constexpr auto check_arg(int v) noexcept -> chan_type {
    return (v >= 0 && v <= 255) ? narrow_cast<chan_type>(v)
        : (FMT_ASSERT(false, "rgb::rgb: channel out of range"),
           narrow_cast<chan_type>(v < 0 ? 0 : 255));
  }

  friend constexpr auto to_underlying(rgb) noexcept -> value_type;

public:
  /// Constructs black, with all three components set to zero.
  constexpr rgb() noexcept = default;

  /// Constructs channels in the range 0-255.
  /// Asserts on invalid values; clamps them when assertions are disabled.
  constexpr rgb(int r_, int g_, int b_) noexcept
    : r{check_arg(r_)}, g{check_arg(g_)}, b{check_arg(b_)} {}

  /**
   * Constructs a color from the low 24 bits of `hex`, interpreted as
   * `0xRRGGBB`. The high byte is ignored.
   */
  constexpr explicit rgb(value_type hex) noexcept
    : r{narrow_cast<chan_type>(hex >> 16)}
    , g{narrow_cast<chan_type>(hex >>  8)}
    , b{narrow_cast<chan_type>(hex >>  0)}
    {}

  /// Converts a terminal color using the fixed Xterm-default RGB palette.
  constexpr rgb(term_color clr) noexcept;

  /// Returns a numeric value for the color as 0x00RRGGBB.
  FMT_NODISCARD
  constexpr explicit operator value_type() const noexcept { return value(); }

  FMT_COLOR_COMPARISONS(rgb)

  /// Combines another color.
  FMT_CONSTEXPR auto operator|=(rgb rhs) noexcept -> rgb& {
    r |= rhs.r;
    g |= rhs.g;
    b |= rhs.b;
    return *this;
  }

  /// Combines two colors.
  FMT_NODISCARD
  friend constexpr auto operator|(rgb lhs, rgb rhs) noexcept -> rgb {
    return rgb{narrow_cast<chan_type>(lhs.r | rhs.r),
               narrow_cast<chan_type>(lhs.g | rhs.g),
               narrow_cast<chan_type>(lhs.b | rhs.b)};
  }

}; // rgb

template<> struct underlying_type<rgb> { using type = rgb::value_type; };

FMT_NODISCARD constexpr auto to_underlying(rgb x) noexcept -> rgb::value_type
{ return x.value(); }

namespace color {

// The agreed Xterm-default RGB mapping for terminal palette indices 0-15.
constexpr FMT_INLINE_VARIABLE rgb term_black          {  0,   0,   0};
constexpr FMT_INLINE_VARIABLE rgb term_red            {205,   0,   0};
constexpr FMT_INLINE_VARIABLE rgb term_green          {  0, 205,   0};
constexpr FMT_INLINE_VARIABLE rgb term_yellow         {205, 205,   0};
constexpr FMT_INLINE_VARIABLE rgb term_blue           {  0,   0, 238};
constexpr FMT_INLINE_VARIABLE rgb term_magenta        {205,   0, 205};
constexpr FMT_INLINE_VARIABLE rgb term_cyan           {  0, 205, 205};
constexpr FMT_INLINE_VARIABLE rgb term_white          {229, 229, 229};
constexpr FMT_INLINE_VARIABLE rgb term_bright_black   {127, 127, 127};
constexpr FMT_INLINE_VARIABLE rgb term_bright_red     {255,   0,   0};
constexpr FMT_INLINE_VARIABLE rgb term_bright_green   {  0, 255,   0};
constexpr FMT_INLINE_VARIABLE rgb term_bright_yellow  {255, 255,   0};
constexpr FMT_INLINE_VARIABLE rgb term_bright_blue    { 92,  92, 255};
constexpr FMT_INLINE_VARIABLE rgb term_bright_magenta {255,   0, 255};
constexpr FMT_INLINE_VARIABLE rgb term_bright_cyan    {  0, 255, 255};
constexpr FMT_INLINE_VARIABLE rgb term_bright_white   {255, 255, 255};

} // color

namespace detail {

FMT_INLINE_VARIABLE constexpr rgb term_color_palette[16] = {
  // 0–7 normal
  color::term_black,
  color::term_red,
  color::term_green,
  color::term_yellow,
  color::term_blue,
  color::term_magenta,
  color::term_cyan,
  color::term_white,

  // 8–15 bright
  color::term_bright_black,
  color::term_bright_red,
  color::term_bright_green,
  color::term_bright_yellow,
  color::term_bright_blue,
  color::term_bright_magenta,
  color::term_bright_cyan,
  color::term_bright_white
}; // term_color_palette

constexpr auto checked_term_index(term_color clr) noexcept -> int8_t {
  return (  to_underlying(clr) >= 0
         && to_underlying(clr) <= to_underlying(term_color::bright_white))
      ? to_underlying(clr)
      : (FMT_ASSERT(false, "rgb::rgb: terminal color out of range"),
         to_underlying(clr));
}

} // detail

FMT_END_EXPORT

#if FMT_USE_CONSTEXPR

constexpr inline rgb::rgb(term_color clr) noexcept
{ *this = detail::term_color_palette[detail::checked_term_index(clr)]; }

#else

constexpr inline rgb::rgb(term_color clr) noexcept
  : rgb{detail::term_color_palette[detail::checked_term_index(clr)]} {}

#endif

FMT_BEGIN_EXPORT

/// A zero-based gray palette index from 0 to 23, rendered at palette index + 232.
struct gray_color {
  using value_type = uint8_t;

private:
  using chan_type = int8_t;
  chan_type _value = 0;

  static constexpr auto make_rgb(int level) noexcept -> rgb
  { return rgb{level, level, level}; }

  FMT_NODISCARD static constexpr chan_type check_arg(int v) noexcept {
    return (v >= 0 && v < 24) ? narrow_cast<chan_type>(v)
        : (FMT_ASSERT(false, "gray_color::gray_color: index out of range"),
           narrow_cast<chan_type>(v < 0 ? 0 : 23));
  }

  friend constexpr auto to_underlying(gray_color) noexcept -> value_type;

public:
  /// Constructs gray index zero, whose RGB equivalent is {8, 8, 8}.
  constexpr gray_color() noexcept = default;
  constexpr gray_color(const gray_color&) noexcept = default;
  FMT_CONSTEXPR gray_color& operator=(const gray_color&)  noexcept = default;

  FMT_COLOR_COMPARISONS(gray_color)

  /// Constructs a gray index in the range 0-23.
  /// Asserts on invalid values; clamps them when assertions are disabled.
  constexpr explicit gray_color(int v) noexcept : _value{check_arg(v)} {}

  /// Converts to RGB, with each channel equal to 8 + 10 * index.
  FMT_NODISCARD constexpr operator rgb() const noexcept
  { return make_rgb(narrow_cast<rgb::chan_type>(10 * _value + 8)); }

}; // gray_color

template<>
struct underlying_type<gray_color> { using type = gray_color::value_type; };

FMT_NODISCARD
constexpr auto to_underlying(gray_color v) noexcept -> gray_color::value_type
{ return static_cast<gray_color::value_type>(v._value); }

namespace detail {

FMT_INLINE_VARIABLE constexpr rgb::chan_type cube_levels[6] =
                                                    {0, 95, 135, 175, 215, 255};
} // detail

/// A 6x6x6 palette color with channel indices 0-5 and a combined index 0-215.
struct cube_color {
  using value_type  = uint8_t;

private:
  using chan_type  =  int8_t;
  chan_type _r = 0;
  chan_type _g = 0;
  chan_type _b = 0;

  FMT_NODISCARD
  static constexpr auto check_arg(int v) noexcept -> chan_type {
    return (v >= 0 && v < 6) ? narrow_cast<chan_type>(v)
        : (FMT_ASSERT(false, "cube_color::cube_color: channel out of range"),
           narrow_cast<chan_type>(v < 0 ? 0 : 5));
  }

  FMT_NODISCARD static constexpr auto check_index(unsigned v) noexcept
      -> value_type {
    return v <= 215 ? narrow_cast<value_type>(v)
        : (FMT_ASSERT(false, "cube_color::cube_color: index out of range"),
           value_type{215});
  }

  FMT_NODISCARD constexpr value_type value() const noexcept
  { return narrow_cast<value_type>(36 * _r + 6 * _g + _b); }

  friend constexpr auto to_underlying(cube_color) noexcept -> value_type;

public:
  /// Returns the red channel index, from 0 to 5.
  FMT_NODISCARD constexpr auto r() const noexcept -> int { return _r; };
  /// Returns the green channel index, from 0 to 5.
  FMT_NODISCARD constexpr auto g() const noexcept -> int { return _g; };
  /// Returns the blue channel index, from 0 to 5.
  FMT_NODISCARD constexpr auto b() const noexcept -> int { return _b; };

  /// Sets the red index; asserts on invalid input and clamps to 0-5.
  FMT_CONSTEXPR auto r(int v) noexcept -> cube_color&
  { _r = check_arg(v); return *this; }

  /// Sets the green index; asserts on invalid input and clamps to 0-5.
  FMT_CONSTEXPR auto g(int v) noexcept -> cube_color&
  { _g = check_arg(v); return *this; }

  /// Sets the blue index; asserts on invalid input and clamps to 0-5.
  FMT_CONSTEXPR auto b(int v) noexcept -> cube_color&
  { _b = check_arg(v); return *this; }

  /// Constructs cube index zero, with all channel indices zero.
  constexpr cube_color() noexcept = default;
  constexpr cube_color(const cube_color&) noexcept = default;
  FMT_CONSTEXPR cube_color& operator=(const cube_color&) noexcept = default;

  FMT_COLOR_COMPARISONS(cube_color)

  /// Constructs channel indices 0-5.
  /// Asserts on invalid values; clamps them when assertions are disabled.
  constexpr cube_color(int r_, int g_, int b_) noexcept
    : _r{check_arg(r_)}, _g{check_arg(g_)}, _b{check_arg(b_)} {}

  /// Constructs a zero-based index 0-215.
  /// Asserts on invalid values; clamps them when assertions are disabled.
  constexpr explicit cube_color(unsigned value) noexcept
    : cube_color{check_index(value) / 36, (check_index(value) / 6) % 6,
                 check_index(value) % 6} {}

  /// Converts channel indices to RGB levels {0, 95, 135, 175, 215, 255}.
  FMT_NODISCARD constexpr operator rgb() const noexcept {
    return rgb{detail::cube_levels[_r],
               detail::cube_levels[_g],
               detail::cube_levels[_b]};
  }

}; // cube_color

template<>
struct underlying_type<cube_color> { using type = cube_color::value_type; };

FMT_NODISCARD
constexpr cube_color::value_type to_underlying(cube_color x) noexcept
{ return x.value(); }

namespace detail {

template<typename T>
struct is_color : std::integral_constant<bool,
    std::is_same<typename std::decay<T>::type, rgb>::value ||
    std::is_same<typename std::decay<T>::type, term_color>::value ||
    std::is_same<typename std::decay<T>::type, cube_color>::value ||
    std::is_same<typename std::decay<T>::type, gray_color>::value> {};

template<typename... Colors> struct all_colors;
template<> struct all_colors<> : std::true_type {};
template<typename First, typename... Rest>
struct all_colors<First, Rest...> : std::integral_constant<bool,
    is_color<First>::value && all_colors<Rest...>::value> {};

} // detail

#if FMT_CPLUSPLUS > 202002L && defined(__cpp_concepts) && \
    __cpp_concepts >= 201907L

/// Identifies types that can be used directly as color values.
template<typename T>
concept Color = detail::is_color<T>::value;

#endif

/// Returns a plain color's fixed RGB equivalent without changing the argument.
#if FMT_CPLUSPLUS > 202002L && defined(__cpp_concepts) && \
    __cpp_concepts >= 201907L
template<Color T>
#else
template<typename T, typename std::enable_if<
    detail::is_color<T>::value, int>::type = 0>
#endif
FMT_NODISCARD
constexpr auto to_rgb(T color) noexcept -> rgb
{ return rgb(color); }

/// Preserves an unspecified color rather than converting it to RGB black.
FMT_NODISCARD constexpr auto to_rgb(no_color_t) noexcept -> no_color_t
{ return no_color_t{}; }

/**
 * A color value holding no_color_t, rgb, term_color, cube_color, or gray_color.
 * Indices 0 through 4, respectively, match the packed discriminator.
 * Default construction holds no_color_t. Construction and assignment from a
 * color preserve its kind. get/get_if provide reference/pointer access, and
 * visit dispatches a visitor to the active alternative. The common interface
 * supports single-color visitation with one result type for every alternative.
 *
 * Uses the union implementation in every supported language mode.
 * References and pointers must not outlive the color or an alternative change.
 * Wrong-kind get reports format_error; get_if provides nonthrowing access.
 */
class color_t;
template<> struct underlying_type<color_t> { using type = uint32_t; };
constexpr auto to_underlying(const color_t&) noexcept -> uint32_t;
namespace detail { template<class T> struct color_access; }

// Alternative indices match text_style's packed discriminator encoding.
class color_t {
public:
   using value_type = uint32_t;

  static_assert(std::is_same<value_type, underlying_type<color_t>::type>::value,
                "color_t underlying type must match value_type");

  /// Alternative identifiers, matching index() and the packed discriminator.
  enum class kind : uint8_t {
    none, ///< Unspecified color.
    rgb,  ///< RGB color.
    term, ///< Terminal palette color.
    cube, ///< 6x6 cube color
    gray  ///< 24-level gray color
  };

private:
  friend constexpr auto to_underlying(const color_t&) noexcept -> value_type;

  union storage {
    no_color_t none;
    rgb        truecolor;
    term_color terminal;
    cube_color cube;
    gray_color gray;
    constexpr storage(no_color_t v) noexcept : none(v)      {}
    constexpr storage(rgb v)        noexcept : truecolor(v) {}
    constexpr storage(term_color v) noexcept : terminal(v)  {}
    constexpr storage(cube_color v) noexcept : cube(v)      {}
    constexpr storage(gray_color v) noexcept : gray(v)      {}
  } _data{no_color};

  kind _kind{kind::none};

  template<class T> friend struct detail::color_access;

public:
  /// Constructs an unspecified color.
  constexpr color_t() noexcept {}

  /// Constructs an unspecified color from its sentinel.
  constexpr color_t(no_color_t v) noexcept : _data{v}, _kind{kind::none} {}

  /// Constructs an RGB color, preserving all three channels.
  constexpr color_t(rgb v)        noexcept : _data{v}, _kind{kind::rgb} {}

  /// Constructs a terminal palette color.
  constexpr color_t(term_color v) noexcept : _data{v}, _kind{kind::term} {}

  /// Constructs a 6x6x6 cube color.
  constexpr color_t(cube_color v) noexcept : _data{v}, _kind{kind::cube} {}

  /// Constructs a gray palette color.
  constexpr color_t(gray_color v) noexcept : _data{v}, _kind{kind::gray} {}

  color_t(const color_t&)            = default;
  color_t(color_t&&)                 = default;
  color_t& operator=(const color_t&) = default;
  color_t& operator=(color_t&&)      = default;

#if defined(__cpp_lib_three_way_comparison) && \
    __cpp_lib_three_way_comparison >= 201907L
  // A union cannot use memberwise defaulted comparisons. Compare the packed
  // values so ordering agrees with the older-language implementation.
  FMT_NODISCARD constexpr auto operator==(const color_t& rhs) const noexcept
    -> bool
  { return to_underlying(*this) == to_underlying(rhs); }

  FMT_NODISCARD constexpr auto operator<=>(const color_t& rhs) const noexcept
    -> std::strong_ordering
  { return to_underlying(*this) <=> to_underlying(rhs); }
#else
  FMT_COLOR_COMPARISONS(color_t)
#endif

  // Trivial union copy assignment starts the corresponding member lifetime.
  // These overloads also permit assignment from the currently held member.
  /// Replaces the active alternative with an unspecified color.
  FMT_CONSTEXPR color_t& operator=(no_color_t v) noexcept
  { return *this = color_t{v}; }
  /// Replaces the active alternative with an RGB color.
  FMT_CONSTEXPR color_t& operator=(rgb v) noexcept
  { return *this = color_t{v}; }
  /// Replaces the active alternative with a terminal palette color.
  FMT_CONSTEXPR color_t& operator=(term_color v) noexcept
  { return *this = color_t{v}; }
  /// Replaces the active alternative with a cube palette color.
  FMT_CONSTEXPR color_t& operator=(cube_color v) noexcept
  { return *this = color_t{v}; }
  /// Replaces the active alternative with a gray palette color.
  FMT_CONSTEXPR color_t& operator=(gray_color v) noexcept
  { return *this = color_t{v}; }

  /// Returns the active alternative: 0 none, 1 RGB, 2 terminal, 3 cube, 4 gray
  constexpr std::size_t index() const noexcept
  { return enum_cast<std::size_t>(_kind); }

private:

  FMT_CONSTEXPR auto promote_to_rgb() noexcept -> color_t& {
    switch (_kind) {
      default:
      case kind::none: return *this = rgb{};
      case kind::rgb:  break;
      case kind::term: *this = rgb(_data.terminal); break;
      case kind::cube: *this = rgb(_data.cube);     break;
      case kind::gray: *this = rgb(_data.gray);     break;
    }
    return *this;
  }

public:

  /**
   * Combines this color with `rhs`.
   *
   * An unspecified color takes the other operand's value, preserving its kind.
   * If both colors are specified, they are promoted to RGB and corresponding
   * channel bits are ORed. This color then holds RGB; `rhs` remains unchanged.
   *
   * Terminal colors use the fixed Xterm-default RGB palette, which may differ
   * from the user's terminal theme. This operation does not average colors.
   */
  FMT_CONSTEXPR auto operator|=(color_t rhs) noexcept -> color_t& {
    if (rhs._kind == kind::none) return *this;
    if (_kind == kind::none) return *this = rhs;
    promote_to_rgb();
    _data.truecolor |= rhs.promote_to_rgb()._data.truecolor;
    return *this;
  }

  /**
   * Returns the composition of `lhs` and `rhs`.
   *
   * Uses the same unspecified-color handling, RGB promotion, and bitwise-OR
   * rules as `operator|=`.
   */
  FMT_NODISCARD
  friend FMT_CONSTEXPR auto operator|(color_t lhs, color_t rhs) noexcept
    -> color_t
  { return lhs |= rhs; }

}; // color_t

template<> struct underlying_type<color_t::kind> { using type = uint8_t; };

FMT_NODISCARD
constexpr auto to_underlying(color_t::kind x) noexcept -> uint8_t
{ return enum_cast<uint8_t>(x); }

namespace detail {

template<> struct color_access<no_color_t> {
  static constexpr std::size_t index = to_underlying(color_t::kind::none);
  static constexpr no_color_t& ref(color_t& c) noexcept
  { return c._data.none; }
  static constexpr const no_color_t& ref(const color_t& c) noexcept
  { return c._data.none; }
};

template<> struct color_access<rgb> {
  static constexpr std::size_t index = to_underlying(color_t::kind::rgb);
  static constexpr rgb& ref(color_t& c) noexcept
  { return c._data.truecolor; }
  static constexpr const rgb& ref(const color_t& c) noexcept
  { return c._data.truecolor; }
};

template<> struct color_access<term_color> {
  static constexpr std::size_t index = to_underlying(color_t::kind::term);
  static constexpr term_color& ref(color_t& c) noexcept
  { return c._data.terminal; }
  static constexpr const term_color& ref(const color_t& c) noexcept
  { return c._data.terminal; }
};

template<> struct color_access<cube_color> {
  static constexpr std::size_t index = to_underlying(color_t::kind::cube);
  static constexpr cube_color& ref(color_t& c) noexcept
  { return c._data.cube; }
  static constexpr const cube_color& ref(const color_t& c) noexcept
  { return c._data.cube; }
};

template<> struct color_access<gray_color> {
  static constexpr std::size_t index = to_underlying(color_t::kind::gray);
  static constexpr gray_color& ref(color_t& c) noexcept
  { return c._data.gray; }
  static constexpr const gray_color& ref(const color_t& c) noexcept
  { return c._data.gray; }
};

template<std::size_t Index> struct color_alternative;
template<> struct color_alternative<to_underlying(color_t::kind::none)>
{ using type = no_color_t; };
template<> struct color_alternative<to_underlying(color_t::kind::rgb)>
{ using type = rgb; };
template<> struct color_alternative<to_underlying(color_t::kind::term)>
{ using type = term_color; };
template<> struct color_alternative<to_underlying(color_t::kind::cube)>
{ using type = cube_color; };
template<> struct color_alternative<to_underlying(color_t::kind::gray)>
{ using type = gray_color; };

} // detail

/// Returns true if the color holds T; T must be a supported alternative.
template<class T>
constexpr bool holds_alternative(const color_t& c) noexcept
{ return c.index() == detail::color_access<T>::index; }

/// Returns a reference to T, reporting an error if another kind is active.
template<class T>
constexpr T& get(color_t& c) {
  return holds_alternative<T>(c) ? detail::color_access<T>::ref(c)
                                : (report_error("wrong color_t alternative"),
                                   detail::color_access<T>::ref(c));
}
template<class T>
constexpr const T& get(const color_t& c) {
  return holds_alternative<T>(c) ? detail::color_access<T>::ref(c)
                                : (report_error("wrong color_t alternative"),
                                   detail::color_access<T>::ref(c));
}
template<class T>
constexpr T&& get(color_t&& c) { return std::move(get<T>(c)); }

template<class T>
constexpr const T&& get(const color_t&& c) { return std::move(get<T>(c)); }

/// Returns a pointer to T, or nullptr for null input or another active kind.
template<class T>
constexpr T* get_if(color_t* c) noexcept {
  return c && holds_alternative<T>(*c) ? &detail::color_access<T>::ref(*c)
                                       : nullptr;
}

template<class T>
constexpr const T* get_if(const color_t* c) noexcept {
  return c && holds_alternative<T>(*c) ? &detail::color_access<T>::ref(*c)
                                       : nullptr;
}

// Index access shares precisely the same checks as type access.
template<std::size_t Index>
constexpr typename detail::color_alternative<Index>::type& get(color_t& c)
{ return get<typename detail::color_alternative<Index>::type>(c); }

template<std::size_t Index>
constexpr
const typename detail::color_alternative<Index>::type& get(const color_t& c)
{ return get<typename detail::color_alternative<Index>::type>(c); }

template<std::size_t Index>
constexpr typename detail::color_alternative<Index>::type&& get(color_t&& c)
{ return get<typename detail::color_alternative<Index>::type>(std::move(c)); }

template<std::size_t Index>
constexpr
const typename detail::color_alternative<Index>::type&& get(const color_t&& c)
{ return get<typename detail::color_alternative<Index>::type>(std::move(c)); }

template<std::size_t Index>
constexpr
typename detail::color_alternative<Index>::type* get_if(color_t* c) noexcept
{ return get_if<typename detail::color_alternative<Index>::type>(c); }

template<std::size_t Index>
constexpr
const typename detail::color_alternative<Index>::type* get_if(const color_t* c)
  noexcept
{ return get_if<typename detail::color_alternative<Index>::type>(c); }

/**
 * Calls visitor with the active alternative of one color.
 * Preserves the color's cv/ref category and forwards the visitor.
 * Every alternative must be callable with exactly the same return type.
 * Visitor exceptions propagate; void and reference results are supported.
 */
template<class Visitor, class Color,
         typename std::enable_if<std::is_same<
           typename std::remove_cv<
                             typename std::remove_reference<Color>::type>::type,
           color_t>::value, int>::type = 0>
constexpr auto visit(Visitor&& visitor, Color&& color)
  -> decltype(std::forward<Visitor>(visitor)
                                  (get<no_color_t>(std::forward<Color>(color))))
{
  using kind = color_t::kind;
  using result = decltype(std::forward<Visitor>(visitor)(
                                  get<no_color_t>(std::forward<Color>(color))));
  static_assert(std::is_same<result, decltype(std::forward<Visitor>(visitor)(
                  get<rgb>(std::forward<Color>(color))))>::value,
          "color visitor must have the same return type for every alternative");
  static_assert(std::is_same<result, decltype(std::forward<Visitor>(visitor)(
                  get<term_color>(std::forward<Color>(color))))>::value,
          "color visitor must have the same return type for every alternative");
  static_assert(std::is_same<result, decltype(std::forward<Visitor>(visitor)(
                  get<cube_color>(std::forward<Color>(color))))>::value,
          "color visitor must have the same return type for every alternative");
  static_assert(std::is_same<result, decltype(std::forward<Visitor>(visitor)(
                  get<gray_color>(std::forward<Color>(color))))>::value,
          "color visitor must have the same return type for every alternative");
#if FMT_USE_CONSTEXPR
  switch (enum_cast<kind>(color.index())) {
    case kind::rgb:
      return std::forward<Visitor>(visitor)(
                                  get<rgb>(std::forward<Color>(color)));
    case kind::term:
      return std::forward<Visitor>(visitor)(
                                  get<term_color>(std::forward<Color>(color)));
    case kind::cube:
      return std::forward<Visitor>(visitor)(
                                  get<cube_color>(std::forward<Color>(color)));
    case kind::gray:
      return std::forward<Visitor>(visitor)(
                                  get<gray_color>(std::forward<Color>(color)));
    case kind::none:
    default:
      return std::forward<Visitor>(visitor)(
                                  get<no_color_t>(std::forward<Color>(color)));
  }
#else
  return color.index() == to_underlying(kind::rgb)
    ? std::forward<Visitor>(visitor)(
                                  get<rgb>(std::forward<Color>(color)))
    : color.index() == to_underlying(kind::term)
    ? std::forward<Visitor>(visitor)(
                                  get<term_color>(std::forward<Color>(color)))
    : color.index() == to_underlying(kind::cube)
    ? std::forward<Visitor>(visitor)(
                                  get<cube_color>(std::forward<Color>(color)))
    : color.index() == to_underlying(kind::gray)
    ? std::forward<Visitor>(visitor)(
                                  get<gray_color>(std::forward<Color>(color)))
    : std::forward<Visitor>(visitor)(
                                  get<no_color_t>(std::forward<Color>(color)));
#endif
} // visit

namespace detail {

struct to_rgb_visitor {
  constexpr auto operator()(const no_color_t&) const noexcept -> color_t
  { return color_t{no_color}; }
  template<typename T>
  constexpr auto operator()(const T& color) const noexcept -> color_t
  { return color_t{fmt::to_rgb(color)}; }
};
} // detail

/**
 * Returns a color variant holding the fixed RGB equivalent of a specified
 * color. An unspecified color remains unspecified. The argument is unchanged.
 */
FMT_NODISCARD constexpr auto to_rgb(color_t color) noexcept -> color_t
{ return fmt::visit(detail::to_rgb_visitor{}, color); }

namespace detail {
struct color_payload {
  template<typename Color>
  constexpr auto operator()(const Color& color) const noexcept
    -> color_t::value_type
  { return static_cast<color_t::value_type>(to_underlying(color)); }
};
} // detail

/// The packed numeric representation used by text_style, not a stable archive.
constexpr inline auto to_underlying(const color_t& color) noexcept
  -> color_t::value_type
{
  return (narrow_cast<color_t::value_type>(color.index()) << 24)
       | fmt::visit(detail::color_payload{}, color);
}

namespace detail {

// Decode valid packed colors produced by text_style using color_t::kind tags.
constexpr auto unpack_color(color_t::value_type value) -> color_t {
  using kind = color_t::kind;
#if FMT_USE_CONSTEXPR
  switch (enum_cast<kind>(value >> 24)) {
    case kind::rgb:  return color_t{rgb{value & 0x00ffffff}};
    case kind::term: return color_t{enum_cast<term_color>(value & 0xff)};
    case kind::cube: return color_t{cube_color{value & 0xff}};
    case kind::gray: return color_t{gray_color{static_cast<int>(value & 0xff)}};
    case kind::none:
    default: return color_t{no_color};
  }
#else
  return (value >> 24) == to_underlying(kind::rgb)
      ? color_t{rgb{value & 0x00ffffff}}
      : (value >> 24) == to_underlying(kind::term)
      ? color_t{enum_cast<term_color>(value & 0xff)}
      : (value >> 24) == to_underlying(kind::cube)
      ? color_t{cube_color{value & 0xff}}
      : (value >> 24) == to_underlying(kind::gray)
      ? color_t{gray_color{static_cast<int>(value & 0xff)}}
      : color_t{no_color};
#endif
} // unpack_color

template<typename T>
struct is_blend_color : std::integral_constant<bool,
    is_color<T>::value ||
    std::is_same<typename std::decay<T>::type, color_t>::value ||
    std::is_same<typename std::decay<T>::type, no_color_t>::value> {};

template<typename... Colors> struct all_blend_colors;
template<> struct all_blend_colors<> : std::true_type {};
template<typename First, typename... Rest>
struct all_blend_colors<First, Rest...> : std::integral_constant<bool,
    is_blend_color<First>::value && all_blend_colors<Rest...>::value> {};

struct blend_rgb {
  constexpr auto operator()(const no_color_t&) const noexcept -> rgb
  { return rgb{}; }
  template<typename T>
  constexpr auto operator()(const T& color) const noexcept -> rgb
  { return rgb(color); }
};

// Pass accumulated values through single-return helpers for C++11 constexpr.
struct blend_state {
  uint64_t r, g, b;
  size_t count;
  color_t first;

  constexpr blend_state(uint64_t red = 0, uint64_t green = 0,
                       uint64_t blue = 0, size_t n = 0,
                       color_t color = no_color) noexcept
    : r{red}, g{green}, b{blue}, count{n}, first{color} {}

  constexpr auto add(rgb color, color_t original) const noexcept -> blend_state {
    return blend_state{r + color.r, g + color.g, b + color.b, count + 1,
                       count == 0 ? original : first};
  }

  constexpr auto add(const color_t& color) const noexcept -> blend_state {
    return color.index() == to_underlying(color_t::kind::none)
        ? *this : add(fmt::visit(blend_rgb{}, color), color);
  }

  constexpr auto average() const noexcept -> rgb {
    return rgb{narrow_cast<rgb::chan_type>(r / count),
               narrow_cast<rgb::chan_type>(g / count),
               narrow_cast<rgb::chan_type>(b / count)};
  }

  constexpr auto result() const noexcept -> color_t
  { return count < 2 ? first : color_t{average()}; }
};

constexpr auto blend_accumulate(blend_state state) noexcept -> blend_state
{ return state; }

template<typename First, typename... Rest>
constexpr auto blend_accumulate(blend_state state, First first,
                                Rest... rest) noexcept -> blend_state
{ return blend_accumulate(state.add(color_t{first}), rest...); }

} // detail

/**
 * Returns the component-wise arithmetic mean of two or more plain colors.
 * Accepts rgb, term_color, cube_color, and gray_color, converting each to RGB.
 * Each argument has equal weight. Fractional channels are rounded down after
 * summing all arguments. The arguments are unchanged.
 * Supports constant evaluation in C++11 and later.
 */
#if FMT_CPLUSPLUS > 202002L && defined(__cpp_concepts) && \
    __cpp_concepts >= 201907L
template<Color First, Color Second, Color... Colors>
#else
template<typename First, typename Second, typename... Colors,
         typename std::enable_if<
             detail::all_colors<First, Second, Colors...>::value,
             int>::type = 0>
#endif
FMT_NODISCARD
constexpr auto blend(First first, Second second, Colors... rest) noexcept
    -> rgb {
#if FMT_CPLUSPLUS > 202002L && defined(__cpp_concepts) && \
    __cpp_concepts >= 201907L
  detail::blend_state state;
  const rgb colors[] = {rgb(first), rgb(second), rgb(rest)...};
  for (auto color : colors) state = state.add(color, color_t{color});
  return state.average();
#else
  return detail::blend_accumulate(
      detail::blend_state{}, first, second, rest...).average();
#endif
}

/**
 * Blends two or more colors, returning the equal-weight RGB average of
 * specified colors and ignoring no_color.
 * Accepts color_t and plain colors. No specified colors produce no_color;
 * one specified color retains its kind and value; two or more produce RGB.
 * Fractional channels are rounded down after summing all specified colors.
 * The arguments are unchanged. Supports constant evaluation in C++11 and later.
 */
#if FMT_CPLUSPLUS > 202002L && defined(__cpp_concepts) && \
    __cpp_concepts >= 201907L
template<typename Second, typename... Colors>
  requires (detail::is_blend_color<Second>::value &&
            (detail::is_blend_color<Colors>::value && ...))
#else
template<typename Second, typename... Colors,
         typename std::enable_if<
             detail::all_blend_colors<Second, Colors...>::value, int>::type = 0>
#endif
FMT_NODISCARD
constexpr auto blend(color_t first, Second second, Colors... rest) noexcept
    -> color_t {
#if FMT_CPLUSPLUS > 202002L && defined(__cpp_concepts) && \
    __cpp_concepts >= 201907L
  detail::blend_state state;
  const color_t colors[] = {first, color_t{second}, color_t{rest}...};
  for (auto color : colors) state = state.add(color);
  return state.result();
#else
  return detail::blend_accumulate(
      detail::blend_state{}, first, second, rest...).result();
#endif
}

namespace detail { struct style_context; }

/**
 * A text style consisting of optional foreground and background colors and a
 * set of emphasis attributes. Each color may be RGB or a terminal palette
 * color.
 *
 * Default construction produces an empty style. Use `fg()`, `bg()`, and
 * emphasis values to construct styles, and `blend()` to combine them.
 * Blending combines emphasis bits by OR and averages the specified colors
 * independently for the foreground and background, ignoring unspecified colors.
 *
 * **Example (C++20 and later)**:
 *
 *     {
 *       using namespace fmt;
 *       using enum emphasis;
 *       using enum term_color;
 *
 *       auto style1 = text_style{color::red, black, bold | underline};
 *
 *       auto style2 = fg(color::red).bg(black).bold().underline();
 *
 *       auto style3 = fg(color::red) | bg(black) | bold | underline;
 *
 *       assert(style1 == style2 && style1 == style3);
 *
 *       auto style4 = style3.underline(false);
 *     }
 *
 * The final line disables underline on `style3` and copies the resulting style
 * into `style4`.
 */
class text_style {
  // The information is packed as follows:
  // ┌──┐
  // │ 0│─┐
  // │..│ ├── foreground color value
  // │23│─┘
  // ├──┤
  // │24│─┬── discriminator for the above value. 000 if unset, 001 if it's
  // │..│ │   an RGB color, 010 if it's a terminal color, 011 if it's a cube
  // │26│─┘   color, and 100 if it's a gray color.
  // ├──┤
  // │27│─┐
  // │..│ │
  // │50│ │
  // ├──┤ │
  // │51│ ├── background color (same format as the foreground color)
  // │..│ │
  // │53│─┘
  // ├──┤
  // │54│─┐
  // │..│ ├── emphases
  // │61│─┘
  // ├──┤
  // │62│──── absolute (reset before applying this style)
  // ├──┤
  // │63│──── unused
  // └──┘

public:
  using value_type = uint64_t;

private:
  value_type style_ = 0;
  friend struct detail::style_context;

  static constexpr auto color_bits = 24;
  static constexpr auto disc_bits  =  3;
  static constexpr auto emph_bits  =  8;
  static constexpr auto fg_shift   =  0;
  static constexpr auto bg_shift   = 27;
  static constexpr auto emph_shift = 54;
  static constexpr auto abs_shift  = emph_shift + emph_bits;

  static constexpr auto color_mask = (color_t::value_type{1} << color_bits) - 1;
  static constexpr auto disc_mask  = (1U  << disc_bits ) - 1;
  static constexpr auto value_bits = color_bits + disc_bits;
  static constexpr auto value_mask = (color_t::value_type{1} << value_bits) - 1;
  static constexpr auto abs_mask   = value_type{1} << abs_shift;

  static constexpr auto fg_mask = value_type{value_mask} << fg_shift;
  static constexpr auto bg_mask = value_type{value_mask} << bg_shift;
  static constexpr auto fg_disc_mask =
                              value_type{disc_mask} << (fg_shift + color_bits);
  static constexpr auto bg_disc_mask =
                              value_type{disc_mask} << (bg_shift + color_bits);

  static constexpr auto emph_mask0 = (1U << emph_bits) - 1;
  static constexpr auto emph_mask  = value_type{emph_mask0} << emph_shift;

 public:
  /**
   * Constructs a style from emphasis and optional foreground and background
   * colors. Both colors default to unspecified.
   *
   * Default construction produces an empty style. Conversion from `emphasis`
   * is implicit.
   */
  constexpr text_style(emphasis emp=emphasis{},
                       color_t fg = no_color,
                       color_t bg = no_color) noexcept
    : text_style{fg, bg, emp} {}

  /**
   * Constructs a style from foreground and background colors and emphasis.
   *
   * Colors may be named RGB constants, RGB, terminal, cube, or gray values,
   * or `no_color`.
   * The background and emphasis default to unspecified and empty, respectively.
   * The foreground is required; `no_color` specifies a background-only style.
   */
  constexpr text_style(color_t fg, color_t bg = no_color,
                       emphasis emp = emphasis{}) noexcept
    : style_{(value_type{to_underlying(fg)}  <<   fg_shift)
           | (value_type{to_underlying(bg)}  <<   bg_shift)
           | (value_type{to_underlying(emp)} << emph_shift)}
           {}

  /// Returns true if no colors, emphasis, or absolute flag are set.
  FMT_NODISCARD constexpr auto empty() const noexcept -> bool
  { return style_ == 0; }

  /**
   * Clears both colors, all emphasis, and absolute, restoring an empty style.
   * Returns a reference to this style for chaining.
   */
  FMT_CONSTEXPR auto clear() noexcept -> text_style& {
    style_ = 0;
    return *this;
  }

  /**
   * Enables absolute if `enable` is true, or disables it otherwise.
   * An absolute style resets the terminal before applying its attributes.
   * Unspecified colors select terminal defaults and unset emphasis is cleared.
   * Absolute does not prevent explicit modification of this style.
   */
  FMT_CONSTEXPR auto absolute(bool enable = true) noexcept -> text_style& {
    if (enable)
      style_ |= abs_mask;
    else
      style_ &= ~abs_mask;
    return *this;
  }

  /// Returns true if this style resets the terminal before applying attributes.
  FMT_NODISCARD constexpr auto is_absolute() const noexcept -> bool
  { return ((style_ & abs_mask) != 0); }

  /**
   * Applies `other` over this style.
   *
   * Specified foreground and background colors in `other` replace the matching
   * channels, preserving their color kinds. Unspecified channels leave this
   * style's colors unchanged. Emphasis bits are combined by OR. The result is
   * absolute if either input is absolute; explicit overlays remain allowed.
   */
  FMT_CONSTEXPR auto overlay(const text_style& other) noexcept -> text_style& {
    if (other == *this || other.empty())
      return *this;

    if (empty()) return *this = other;

    absolute(is_absolute() || other.is_absolute());
    emph(emph() | other.emph());
    if (other.has_fg()) fg(other.fg());
    if (other.has_bg()) bg(other.bg());
    return *this;
  } // overlay

  /**
   * Combines this style with `rhs` and returns a reference to this style.
   *
   * Emphasis bits are combined by OR. An unspecified color channel takes the
   * other style's color, preserving its kind. If both styles specify colors
   * in the same channel, they are converted to RGB and their corresponding
   * component bits are ORed, without averaging or alpha compositing.
   *
   * The result is absolute if either input is absolute.
   * Terminal colors are converted using the fixed Xterm-default RGB palette,
   * which may differ from the user's terminal theme.
   */
  FMT_CONSTEXPR auto operator|=(text_style rhs) noexcept -> text_style& {
    constexpr auto mask = value_type{disc_mask & ~1U};
    constexpr auto promotion_mask = (mask << (fg_shift + color_bits))
                                  | (mask << (bg_shift + color_bits));
    auto style = style_ | rhs.style_;
    if ((style & promotion_mask) == 0) {
      style_ = style;
      return *this;
    }
    fg  (fg()   | rhs.fg());
    bg  (bg()   | rhs.bg());
    emph(emph() | rhs.emph());
    absolute(is_absolute() || rhs.is_absolute());
    return *this;
  }

  /**
   * Returns the composition of `lhs` and `rhs`.
   *
   * Uses the same color promotion, RGB bitwise-OR, and emphasis combination
   * rules as `operator|=`.
   */
  FMT_NODISCARD
  friend FMT_CONSTEXPR auto operator|(text_style lhs, text_style rhs) noexcept
    -> text_style
  { return lhs |= rhs; }

  /// Returns true if a foreground color is specified.
  FMT_NODISCARD constexpr auto has_fg() const noexcept -> bool
  { return ((style_ & fg_disc_mask) != 0); }

  /// Returns true if a background color is specified.
  FMT_NODISCARD constexpr auto has_bg() const noexcept -> bool
  { return ((style_ & bg_disc_mask) != 0); }

  /// Returns true if at least one emphasis attribute is set.
  FMT_NODISCARD constexpr auto has_emph() const noexcept -> bool
  { return ((style_ & emph_mask) != 0); }

  /// Returns true if a foreground color is specified.
  FMT_NODISCARD constexpr auto has_foreground() const noexcept -> bool
  { return has_fg(); }

  /// Returns true if a background color is specified.
  FMT_NODISCARD constexpr auto has_background() const noexcept -> bool
  { return has_bg(); }

  /// Returns true if at least one emphasis attribute is set.
  FMT_NODISCARD constexpr auto has_emphasis() const noexcept -> bool
  { return has_emph(); }

  /**
   * Returns the foreground color, preserving its kind and value.
   * The result can be passed to `fg()` or `bg()`.
   */
  FMT_NODISCARD
  constexpr auto fg() const noexcept -> color_t {
    using value_t = color_t::value_type;
    return detail::unpack_color(
                      narrow_cast<value_t>((style_ >> fg_shift) & value_mask));
  }

  /**
   * Returns the background color, preserving its kind and value.
   * The result can be passed to `fg()` or `bg()`.
   */
  FMT_NODISCARD
  constexpr auto bg() const noexcept -> color_t {
    using value_t = color_t::value_type;
    return detail::unpack_color(
                      narrow_cast<value_t>((style_ >> bg_shift) & value_mask));
  }

  /// Returns the combined emphasis bit mask.
  FMT_NODISCARD constexpr auto emph() const noexcept -> emphasis
  { return enum_cast<emphasis>((style_ & emph_mask) >> emph_shift); }

  /**
   * Returns the foreground color, preserving its kind and value.
   * The result can be passed to `fg()` or `bg()`.
   */
  FMT_NODISCARD constexpr auto get_foreground() const noexcept
    -> color_t
  { return fg(); }

  /**
   * Returns the background color, preserving its kind and value.
   * The result can be passed to `fg()` or `bg()`.
   */
  FMT_NODISCARD constexpr auto get_background() const noexcept
    -> color_t
  { return bg(); }

  /// Returns the combined emphasis bit mask, or `emphasis{}` if none is set.
  FMT_NODISCARD constexpr auto get_emphasis() const noexcept -> emphasis
  { return emph(); }

  /**
   * Replaces the foreground color and returns a reference to this style.
   * Accepts named RGB constants, RGB, terminal, cube, or gray values, or `no_color`.
   * Passing `no_color` clears the foreground to unspecified; it does not store
   * a terminal reset. Preserves the background color and emphasis attributes.
   */
  FMT_CONSTEXPR auto fg(color_t color) noexcept -> text_style& {
    style_ = (style_ & ~fg_mask)
           | (value_type{to_underlying(color)} << fg_shift);
    return *this;
  }

  /**
   * Replaces the background color and returns a reference to this style.
   * Accepts named RGB constants, RGB, terminal, cube, or gray values, or `no_color`.
   * Passing `no_color` clears the background to unspecified; it does not store
   * a terminal reset. Preserves the foreground color and emphasis attributes.
   */
  FMT_CONSTEXPR auto bg(color_t color) noexcept -> text_style& {
    style_ = (style_ & ~bg_mask)
           | (value_type{to_underlying(color)} << bg_shift);
    return *this;
  }

  /**
   * Replaces all emphasis attributes and returns a reference to this style.
   * Only the bits in `em` remain enabled; `emphasis{}` clears all attributes.
   * Preserves both colors. Bold and faint remain independent bits.
   */
  FMT_CONSTEXPR auto emph(emphasis em) noexcept -> text_style& {
    style_ = (style_ & ~emph_mask) | (enum_cast<value_type>(em) << emph_shift);
    return *this;
  }

private:

  FMT_CONSTEXPR auto set(emphasis em, bool enable=true) noexcept -> text_style&
  {
    auto mask = enum_cast<value_type>(em) << emph_shift;
    style_ = enable ? (style_ | mask) : (style_ & ~mask);
    return *this;
  }

public:

  /// Enables bold if `enable` is true, or disables it otherwise.
  FMT_CONSTEXPR auto bold(bool enable = true) noexcept -> text_style&
  { return set(emphasis::bold, enable); }

  /// Enables faint if `enable` is true, or disables it otherwise.
  FMT_CONSTEXPR auto faint(bool enable = true) noexcept -> text_style&
  { return set(emphasis::faint, enable); }

  /// Enables italic if `enable` is true, or disables it otherwise.
  FMT_CONSTEXPR auto italic(bool enable = true) noexcept -> text_style&
  { return set(emphasis::italic, enable); }

  /// Enables underline if `enable` is true, or disables it otherwise.
  FMT_CONSTEXPR auto underline(bool enable = true) noexcept -> text_style&
  { return set(emphasis::underline, enable); }

  /// Enables blink if `enable` is true, or disables it otherwise.
  FMT_CONSTEXPR auto blink(bool enable = true) noexcept -> text_style&
  { return set(emphasis::blink, enable); }

  /// Enables reverse if `enable` is true, or disables it otherwise.
  FMT_CONSTEXPR auto reverse(bool enable = true) noexcept -> text_style&
  { return set(emphasis::reverse, enable); }

  /// Enables conceal if `enable` is true, or disables it otherwise.
  FMT_CONSTEXPR auto conceal(bool enable = true) noexcept -> text_style&
  { return set(emphasis::conceal, enable); }

  /// Enables strikethrough if `enable` is true, or disables it otherwise.
  FMT_CONSTEXPR auto strike(bool enable = true) noexcept -> text_style&
  { return set(emphasis::strike, enable); }

private:

  FMT_NODISCARD constexpr auto is_enabled(emphasis em) const noexcept -> bool
  { return ((style_ & (enum_cast<value_type>(em) << emph_shift)) != 0); }

public:

  /// Returns true if bold is enabled, or false if it is unset.
  FMT_NODISCARD constexpr auto is_bold() const noexcept -> bool
  { return is_enabled(emphasis::bold); }

  /// Returns true if faint is enabled, or false if it is unset.
  FMT_NODISCARD constexpr auto is_faint() const noexcept -> bool
  { return is_enabled(emphasis::faint); }

  /// Returns true if italic is enabled, or false if it is unset.
  FMT_NODISCARD constexpr auto is_italic() const noexcept -> bool
  { return is_enabled(emphasis::italic); }

  /// Returns true if underline is enabled, or false if it is unset.
  FMT_NODISCARD constexpr auto is_underline() const noexcept -> bool
  { return is_enabled(emphasis::underline); }

  /// Returns true if blink is enabled, or false if it is unset.
  FMT_NODISCARD constexpr auto is_blink() const noexcept -> bool
  { return is_enabled(emphasis::blink); }

  /// Returns true if reverse is enabled, or false if it is unset.
  FMT_NODISCARD constexpr auto is_reverse() const noexcept -> bool
  { return is_enabled(emphasis::reverse); }

  /// Returns true if conceal is enabled, or false if it is unset.
  FMT_NODISCARD constexpr auto is_conceal() const noexcept -> bool
  { return is_enabled(emphasis::conceal); }

  /// Returns true if strikethrough is enabled, or false if it is unset.
  FMT_NODISCARD constexpr auto is_strike() const noexcept -> bool
  { return is_enabled(emphasis::strike); }

  /// Alias for `strike()`, retained for backward compatibility.
  FMT_CONSTEXPR auto strikethrough(bool enable = true) noexcept -> text_style&
  { return strike(enable); }

  /// Alias for `is_strike()`, retained for backward compatibility.
  FMT_NODISCARD constexpr auto is_strikethrough() const noexcept -> bool
  { return is_strike(); }

  FMT_COLOR_COMPARISONS(text_style)

 private:
  constexpr explicit text_style(value_type style) noexcept : style_{style} {}

  friend constexpr auto to_underlying(text_style value) noexcept -> value_type;
  friend constexpr auto fg(color_t foreground) noexcept -> text_style;
  friend constexpr auto bg(color_t background) noexcept -> text_style;
}; // text_style

/// Returns a copy of `style` marked absolute, preserving its colors and emphasis.
FMT_NODISCARD FMT_CONSTEXPR inline
auto absolute(text_style style) noexcept -> text_style
{ return style.absolute(); }

/**
 * Returns a style with only bold enabled when `enable` is true (the default),
 * or an empty style otherwise. Both color channels are unspecified.
 */
FMT_NODISCARD constexpr inline
auto bold(bool enable = true) noexcept -> text_style
{ return text_style{enable ? emphasis::bold : emphasis{}}; }

/// Returns a copy of `style` with bold enabled or disabled by `enable`.
FMT_NODISCARD FMT_CONSTEXPR inline
auto bold(text_style style, bool enable = true) noexcept -> text_style
{ return style.bold(enable); }

/**
 * Returns a style with only faint enabled when `enable` is true (the default),
 * or an empty style otherwise. Both color channels are unspecified.
 */
FMT_NODISCARD constexpr inline
auto faint(bool enable = true) noexcept -> text_style
{ return text_style{enable ? emphasis::faint : emphasis{}}; }

/// Returns a copy of `style` with faint enabled or disabled by `enable`.
FMT_NODISCARD FMT_CONSTEXPR inline
auto faint(text_style style, bool enable = true) noexcept -> text_style
{ return style.faint(enable); }

/**
 * Returns a style with only italic enabled when `enable` is true (the default),
 * or an empty style otherwise. Both color channels are unspecified.
 */
FMT_NODISCARD constexpr inline
auto italic(bool enable = true) noexcept -> text_style
{ return text_style{enable ? emphasis::italic : emphasis{}}; }

/// Returns a copy of `style` with italic enabled or disabled by `enable`.
FMT_NODISCARD FMT_CONSTEXPR inline
auto italic(text_style style, bool enable = true) noexcept -> text_style
{ return style.italic(enable); }

/**
 * Returns a style with only underline enabled when `enable` is true (the default),
 * or an empty style otherwise. Both color channels are unspecified.
 */
FMT_NODISCARD constexpr inline
auto underline(bool enable = true) noexcept -> text_style
{ return text_style{enable ? emphasis::underline : emphasis{}}; }

/// Returns a copy of `style` with underline enabled or disabled by `enable`.
FMT_NODISCARD FMT_CONSTEXPR inline
auto underline(text_style style, bool enable = true) noexcept -> text_style
{ return style.underline(enable); }

/**
 * Returns a style with only blink enabled when `enable` is true (the default),
 * or an empty style otherwise. Both color channels are unspecified.
 */
FMT_NODISCARD constexpr inline
auto blink(bool enable = true) noexcept -> text_style
{ return text_style{enable ? emphasis::blink : emphasis{}}; }

/// Returns a copy of `style` with blink enabled or disabled by `enable`.
FMT_NODISCARD FMT_CONSTEXPR inline
auto blink(text_style style, bool enable = true) noexcept -> text_style
{ return style.blink(enable); }

/**
 * Returns a style with only reverse enabled when `enable` is true (the default),
 * or an empty style otherwise. Both color channels are unspecified.
 */
FMT_NODISCARD constexpr inline
auto reverse(bool enable = true) noexcept -> text_style
{ return text_style{enable ? emphasis::reverse : emphasis{}}; }

/// Returns a copy of `style` with reverse enabled or disabled by `enable`.
FMT_NODISCARD FMT_CONSTEXPR inline
auto reverse(text_style style, bool enable = true) noexcept -> text_style
{ return style.reverse(enable); }

/**
 * Returns a style with only conceal enabled when `enable` is true (the default),
 * or an empty style otherwise. Both color channels are unspecified.
 */
FMT_NODISCARD constexpr inline
auto conceal(bool enable = true) noexcept -> text_style
{ return text_style{enable ? emphasis::conceal : emphasis{}}; }

/// Returns a copy of `style` with conceal enabled or disabled by `enable`.
FMT_NODISCARD FMT_CONSTEXPR inline
auto conceal(text_style style, bool enable = true) noexcept -> text_style
{ return style.conceal(enable); }

/**
 * Returns a style with only strike enabled when `enable` is true (the default),
 * or an empty style otherwise. Both color channels are unspecified.
 */
FMT_NODISCARD constexpr inline
auto strike(bool enable = true) noexcept -> text_style
{ return text_style{enable ? emphasis::strike : emphasis{}}; }

/// Returns a copy of `style` with strike enabled or disabled by `enable`.
FMT_NODISCARD FMT_CONSTEXPR inline
auto strike(text_style style, bool enable = true) noexcept -> text_style
{ return style.strike(enable); }

/**
 * Returns a copy of `base` with `overrides` applied, leaving both inputs
 * unchanged. Specified colors in `overrides` replace matching channels and
 * preserve their kinds; unspecified channels retain the base colors.
 * Emphasis bits are combined by OR. The result is absolute if either input is.
 */
FMT_NODISCARD FMT_CONSTEXPR inline
auto overlay(text_style base, text_style overrides) noexcept -> text_style
{ return base.overlay(overrides); }

template<>
struct underlying_type<text_style> { using type = text_style::value_type; };

FMT_NODISCARD constexpr inline auto to_underlying(text_style value) noexcept
  -> text_style::value_type
{ return value.style_; }

namespace detail {

template<typename T>
struct is_text_style : std::is_same<typename std::decay<T>::type, text_style> {};

template<typename... Styles> struct all_text_styles;
template<> struct all_text_styles<> : std::true_type {};
template<typename First, typename... Rest>
struct all_text_styles<First, Rest...> : std::integral_constant<bool,
    is_text_style<First>::value && all_text_styles<Rest...>::value> {};

constexpr auto checked_blend_style(const text_style& style) noexcept
    -> const text_style& {
  return !style.is_absolute() || (style.has_fg() && style.has_bg())
      ? style
      : (FMT_ASSERT(false,
                    "cannot blend an absolute style with "
                    "terminal-default colors"),
         style);
}

struct style_blend_state {
  blend_state foreground, background;
  emphasis attributes;

  constexpr style_blend_state(blend_state fg = blend_state{},
                             blend_state bg = blend_state{},
                             emphasis em = emphasis{}) noexcept
    : foreground{fg}, background{bg}, attributes{em} {}

  constexpr auto add_checked(const text_style& style) const noexcept
      -> style_blend_state {
    return style_blend_state{foreground.add(style.fg()),
                             background.add(style.bg()),
                             attributes | style.emph()};
  }

  constexpr auto add(const text_style& style) const noexcept
      -> style_blend_state
  { return add_checked(checked_blend_style(style)); }

  constexpr auto result() const noexcept -> text_style
  { return text_style{foreground.result(), background.result(), attributes}; }
};

constexpr auto blend_styles_accumulate(style_blend_state state) noexcept
  -> style_blend_state
{ return state; }

template<typename First, typename... Rest>
constexpr auto blend_styles_accumulate(style_blend_state state, First first,
                                      Rest... rest) noexcept -> style_blend_state
{ return blend_styles_accumulate(state.add(first), rest...); }

} // detail

/**
 * Blends two or more text styles, averaging each color channel independently
 * and combining emphasis bits by OR. Unspecified colors do not contribute.
 * Each channel preserves its kind with one specified color, holds RGB with
 * two or more, and remains unspecified with none. The arguments are unchanged.
 * The result is relative, even when an input is absolute.
 *
 * Precondition: each absolute input specifies both foreground and background;
 * terminal-default colors have unknown RGB values. Violations assert. When
 * assertions are disabled, unspecified channels contribute nothing and the
 * absolute flag is still discarded.
 * Supports constant evaluation in C++11 and later.
 */
#if FMT_CPLUSPLUS > 202002L && defined(__cpp_concepts) && \
    __cpp_concepts >= 201907L
template<typename First, typename Second, typename... Styles>
  requires (detail::is_text_style<First>::value &&
            detail::is_text_style<Second>::value &&
            (detail::is_text_style<Styles>::value && ...))
#else
template<typename First, typename Second, typename... Styles,
         typename std::enable_if<
             detail::all_text_styles<First, Second, Styles...>::value,
             int>::type = 0>
#endif
FMT_NODISCARD
constexpr auto blend(First first, Second second, Styles... rest) noexcept
    -> text_style {
#if FMT_CPLUSPLUS > 202002L && defined(__cpp_concepts) && \
    __cpp_concepts >= 201907L
  detail::style_blend_state state;
  const text_style styles[] = {first, second, rest...};
  for (auto style : styles) state = state.add(style);
  return state.result();
#else
  return detail::blend_styles_accumulate(
      detail::style_blend_state{}, first, second, rest...).result();
#endif
}

/**
 * Creates a style with the specified foreground (text) color, no background
 * color, and no emphasis. Accepts any color value through implicit conversion
 * to `color_t`.
 */
FMT_NODISCARD
constexpr inline auto fg(color_t foreground) noexcept -> text_style {
  return text_style{text_style::value_type{to_underlying(foreground)}
                    << text_style::fg_shift};
}

/**
 * Creates a style with the specified background (text) color, no foreground
 * color, and no emphasis. Accepts any color value through implicit conversion
 * to `color_t`.
 */
FMT_NODISCARD
constexpr inline auto bg(color_t background) noexcept -> text_style {
  return text_style{text_style::value_type{to_underlying(background)}
                    << text_style::bg_shift};
}

/// Upper bound on style SGR output length.
constexpr auto sgr_max_length   = std::size_t{60};

/// Length of the terminal-reset SGR sequence.
constexpr auto sgr_reset_length = std::size_t{ 4};

#if defined(__cpp_constexpr) && __cpp_constexpr >= 202211L

/**
 * Returns the ANSI SGR sequence that resets colors and emphasis to terminal
 * defaults. The returned view refers to static storage. `Char` defaults to
 * `char`.
 */
template<typename Char = char>
FMT_NODISCARD constexpr auto sgr_reset() noexcept -> basic_string_view<Char> {
  static constexpr Char reset[] = {'\x1b', '[', '0', 'm'};
  return {reset, sgr_reset_length};
}

#else

namespace detail {

template<typename Char>
struct sgr_reset_storage {
  static constexpr Char value[sgr_reset_length] = {'\x1b', '[', '0', 'm'};
};

#if FMT_CPLUSPLUS < 201703L
template<typename Char>
constexpr Char sgr_reset_storage<Char>::value[sgr_reset_length];
#endif

} // detail

/**
 * Returns the ANSI SGR sequence that resets colors and emphasis to terminal
 * defaults. The returned view refers to static storage. `Char` defaults to
 * `char`.
 */
template<typename Char = char>
FMT_NODISCARD constexpr auto sgr_reset() noexcept -> basic_string_view<Char>
{ return {detail::sgr_reset_storage<Char>::value, sgr_reset_length}; }

#endif

namespace detail {

#if FMT_COLOR_COMPACT_SGR

constexpr const char* decimal_strings[256] = {
    "0",   "1",   "2",   "3",   "4",   "5",   "6",   "7",
    "8",   "9",  "10",  "11",  "12",  "13",  "14",  "15",
   "16",  "17",  "18",  "19",  "20",  "21",  "22",  "23",
   "24",  "25",  "26",  "27",  "28",  "29",  "30",  "31",
   "32",  "33",  "34",  "35",  "36",  "37",  "38",  "39",
   "40",  "41",  "42",  "43",  "44",  "45",  "46",  "47",
   "48",  "49",  "50",  "51",  "52",  "53",  "54",  "55",
   "56",  "57",  "58",  "59",  "60",  "61",  "62",  "63",
   "64",  "65",  "66",  "67",  "68",  "69",  "70",  "71",
   "72",  "73",  "74",  "75",  "76",  "77",  "78",  "79",
   "80",  "81",  "82",  "83",  "84",  "85",  "86",  "87",
   "88",  "89",  "90",  "91",  "92",  "93",  "94",  "95",
   "96",  "97",  "98",  "99", "100", "101", "102", "103",
  "104", "105", "106", "107", "108", "109", "110", "111",
  "112", "113", "114", "115", "116", "117", "118", "119",
  "120", "121", "122", "123", "124", "125", "126", "127",
  "128", "129", "130", "131", "132", "133", "134", "135",
  "136", "137", "138", "139", "140", "141", "142", "143",
  "144", "145", "146", "147", "148", "149", "150", "151",
  "152", "153", "154", "155", "156", "157", "158", "159",
  "160", "161", "162", "163", "164", "165", "166", "167",
  "168", "169", "170", "171", "172", "173", "174", "175",
  "176", "177", "178", "179", "180", "181", "182", "183",
  "184", "185", "186", "187", "188", "189", "190", "191",
  "192", "193", "194", "195", "196", "197", "198", "199",
  "200", "201", "202", "203", "204", "205", "206", "207",
  "208", "209", "210", "211", "212", "213", "214", "215",
  "216", "217", "218", "219", "220", "221", "222", "223",
  "224", "225", "226", "227", "228", "229", "230", "231",
  "232", "233", "234", "235", "236", "237", "238", "239",
  "240", "241", "242", "243", "244", "245", "246", "247",
  "248", "249", "250", "251", "252", "253", "254", "255"
}; // decimal_strings

#else

constexpr const char* decimal_strings[256] = {
  "000", "001", "002", "003", "004", "005", "006", "007",
  "008", "009", "010", "011", "012", "013", "014", "015",
  "016", "017", "018", "019", "020", "021", "022", "023",
  "024", "025", "026", "027", "028", "029", "030", "031",
  "032", "033", "034", "035", "036", "037", "038", "039",
  "040", "041", "042", "043", "044", "045", "046", "047",
  "048", "049", "050", "051", "052", "053", "054", "055",
  "056", "057", "058", "059", "060", "061", "062", "063",
  "064", "065", "066", "067", "068", "069", "070", "071",
  "072", "073", "074", "075", "076", "077", "078", "079",
  "080", "081", "082", "083", "084", "085", "086", "087",
  "088", "089", "090", "091", "092", "093", "094", "095",
  "096", "097", "098", "099", "100", "101", "102", "103",
  "104", "105", "106", "107", "108", "109", "110", "111",
  "112", "113", "114", "115", "116", "117", "118", "119",
  "120", "121", "122", "123", "124", "125", "126", "127",
  "128", "129", "130", "131", "132", "133", "134", "135",
  "136", "137", "138", "139", "140", "141", "142", "143",
  "144", "145", "146", "147", "148", "149", "150", "151",
  "152", "153", "154", "155", "156", "157", "158", "159",
  "160", "161", "162", "163", "164", "165", "166", "167",
  "168", "169", "170", "171", "172", "173", "174", "175",
  "176", "177", "178", "179", "180", "181", "182", "183",
  "184", "185", "186", "187", "188", "189", "190", "191",
  "192", "193", "194", "195", "196", "197", "198", "199",
  "200", "201", "202", "203", "204", "205", "206", "207",
  "208", "209", "210", "211", "212", "213", "214", "215",
  "216", "217", "218", "219", "220", "221", "222", "223",
  "224", "225", "226", "227", "228", "229", "230", "231",
  "232", "233", "234", "235", "236", "237", "238", "239",
  "240", "241", "242", "243", "244", "245", "246", "247",
  "248", "249", "250", "251", "252", "253", "254", "255"
}; // decimal_strings

#endif

template<typename Char>
class ansi_color_escape {
  // Maximum color SGR is "\x1b[38;2;255;255;255m"
  // Masimum emph  SGR is "\x1b[1;2;3;4;5;7;8;9m
  Char buffer[20] = {};
  size_t size = 0;

  FMT_CONSTEXPR void append(char c) noexcept
  { buffer[size++] = narrow_cast<Char>(c); }

  FMT_CONSTEXPR void append(const char* s) noexcept {
    while (*s)
      append(*s++);
  }

  FMT_CONSTEXPR void append(const char* s, char c) noexcept {
    append(s);
    append(c);
  }

  FMT_CONSTEXPR void append(uint8_t n, char c) noexcept {
    append(decimal_strings[n]);
    append(c);
  }

  FMT_CONSTEXPR void append(no_color_t, bool) noexcept {}

  FMT_CONSTEXPR void append(term_color clr, bool is_bg) noexcept {
    constexpr auto BrightBlack
                    = static_cast<int>(to_underlying(term_color::bright_black));
    auto value = static_cast<int>(to_underlying(clr));
    value += (value < BrightBlack) ? 30 : 90 - BrightBlack;

    // Background ASCII codes are the same as the foreground ones + 10.
    if (is_bg) value += 10;
    if (value >= 100) {
      append('1');
      value %= 100;
    }
    append(narrow_cast<char>('0' + value / 10));
    append(narrow_cast<char>('0' + value % 10));
    append(';');
  }

  FMT_CONSTEXPR void append(rgb clr, bool is_bg) noexcept {
    append("38;2;");
    if (is_bg) buffer[0] = Char{'4'};
    append(clr.r, ';');
    append(clr.g, ';');
    append(clr.b, ';');
  }

  FMT_CONSTEXPR void append(cube_color clr, bool is_bg) noexcept {
    append("38;5;");
    if (is_bg) buffer[0] = Char{'4'};
    auto index = narrow_cast<uint8_t>(to_underlying(clr) + 16);
    append(index, ';');
  }

  FMT_CONSTEXPR void append(gray_color clr, bool is_bg) noexcept {
    append("38;5;");
    if (is_bg) buffer[0] = Char{'4'};
    auto index = narrow_cast<uint8_t>(to_underlying(clr) + 232);
    append(index, ';');
  }

  struct color_visitor {
    ansi_color_escape& escape;
    bool is_bg;

    // Avoid copying the empty union member during MSVC constant evaluation.
    FMT_CONSTEXPR void operator()(const no_color_t&) const noexcept {}

    template<typename Color>
    FMT_CONSTEXPR void operator()(Color clr) const noexcept {
      escape.append(clr, is_bg);
    }
  };

public:

  FMT_CONSTEXPR ansi_color_escape(color_t text_color, bool is_bg) noexcept {
    fmt::visit(color_visitor{*this, is_bg}, text_color);
  }

  FMT_CONSTEXPR ansi_color_escape(emphasis em) noexcept {
    unsigned bits = to_underlying(em);
    if (bits == 0)
      return;

    constexpr char sgr[] = { '1', '2', '3', '4', '5', '7', '8', '9' };

    for (int i = 0; i < 8; ++i, bits >>= 1) {
      if (!(bits & 1))
        continue;
      append(sgr[i]);
      append(';');
    }
  }

  FMT_NODISCARD constexpr auto empty() const noexcept -> bool
  { return (size == 0); }

  constexpr operator const Char*() const noexcept { return buffer; }

  FMT_NODISCARD constexpr auto begin() const noexcept -> const Char*
  { return buffer; }

  FMT_NODISCARD constexpr auto end() const noexcept -> const Char*
  { return buffer + size; }
}; // ansi_color_escape

template<typename Char>
FMT_CONSTEXPR auto make_foreground_color(color_t foreground) noexcept
  -> ansi_color_escape<Char>
{ return ansi_color_escape<Char>(foreground, false); }

template<typename Char>
FMT_CONSTEXPR auto make_background_color(color_t background) noexcept
  -> ansi_color_escape<Char>
{ return ansi_color_escape<Char>(background, true); }

template<typename Char>
FMT_CONSTEXPR auto make_emphasis(emphasis em) noexcept
  -> ansi_color_escape<Char>
{ return ansi_color_escape<Char>(em); }

template<typename Char>
FMT_CONSTEXPR20 inline void reset_color(buffer<Char>& buffer) {
  constexpr auto reset_color = sgr_reset<Char>();
  buffer.append(reset_color.begin(), reset_color.end());
}

template<typename T>
struct styled_arg : view {
  const T& value;
  text_style style;
  constexpr styled_arg(const T& v, text_style s) : value{v}, style{s} {}
}; // styled_arg

template<typename Char>
FMT_CONSTEXPR20 void append_sgr_raw(buffer<Char>& buf, text_style ts) {
  if (ts.empty()) return;
  if (ts.is_absolute()) {
    buf.push_back(Char('0'));
    buf.push_back(Char(';'));
  }
  auto emphasis = make_emphasis<Char>(ts.emph());
  buf.append(emphasis.begin(), emphasis.end());
  auto foreground = make_foreground_color<Char>(ts.fg());
  buf.append(foreground.begin(), foreground.end());
  auto background = make_background_color<Char>(ts.bg());
  buf.append(background.begin(), background.end());
} // append_sgr_raw

#if FMT_COLOR_COMPACT_SGR

template<typename Char>
FMT_CONSTEXPR20 void append_sgr(buffer<Char>& buf, text_style ts) {
  if (ts.empty()) return;
  buf.push_back(static_cast<Char>('\x1b'));
  buf.push_back(static_cast<Char>('['));
  append_sgr_raw(buf, ts);
  buf.data()[buf.size()-1] = static_cast<Char>('m');
} // append_sgr

#else

template<typename Char>
FMT_CONSTEXPR20
void append_sgr(buffer<Char>& buf, const ansi_color_escape<Char>& sgr) {
  if (sgr.empty()) return;
  buf.push_back(static_cast<Char>('\x1b'));
  buf.push_back(static_cast<Char>('['));
  buf.append(sgr.begin(), sgr.end());
  buf.data()[buf.size()-1] = static_cast<Char>('m');
} // append_sgr

template<typename Char>
FMT_CONSTEXPR20 void append_sgr(buffer<Char>& buf, text_style ts) {
  if (ts.empty()) return;
  if (ts.is_absolute()) reset_color(buf);
  auto emphasis = make_emphasis<Char>(ts.emph());
  append_sgr(buf, emphasis);
  auto foreground = make_foreground_color<Char>(ts.fg());
  append_sgr(buf, foreground);
  auto background = make_background_color<Char>(ts.bg());
  append_sgr(buf, background);
} // append_sgr

#endif

// Convert the context's private packed value without exposing an integer
// constructor on text_style. Context state is always a complete, relative value.
struct style_context {

  template<typename Context>
  static FMT_CONSTEXPR auto get(const Context& ctx) noexcept -> text_style {
    auto result = text_style{};
    result.style_ = color_context::get(ctx);
    return result;
  }

  template<typename Context>
  static FMT_CONSTEXPR void set(Context& ctx, text_style style) noexcept
  { color_context::set(ctx, to_underlying(style.absolute(false))); }

}; // style_context

template<typename Context> class style_scope {
  Context& _context;
  uint64_t _saved;

 public:
  FMT_CONSTEXPR explicit style_scope(Context& ctx) noexcept
    : _context{ctx}, _saved{color_context::get(ctx)} {}

  // Restore context metadata on exceptions; partial output may remain.
  FMT_CONSTEXPR20 ~style_scope() { color_context::set(_context, _saved); }

  style_scope(const style_scope&) = delete;
  void operator=(const style_scope&) = delete;
}; // style_scope

template<typename Char>
void vformat_to(buffer<Char>& buf, text_style ts, basic_string_view<Char> fmt,
                basic_format_args<buffered_context<Char>> args)
{
  append_sgr(buf, ts);
  vformat_to(buf, fmt, args, {},
             to_underlying(text_style{ts}.absolute(false)));
  if (ts != text_style()) reset_color<Char>(buf);
} // vformat_to

}  // detail

/**
 * Returns the ANSI SGR escape sequences for the specified foreground,
 * background, and emphasis in `ts`. Absolute styles begin with a terminal
 * reset: compact mode includes `0;` in the sequence, while noncompact mode
 * emits a separate reset sequence first. An absolute-only style emits a reset.
 *
 * Returns an empty string for an empty style.
 * Unspecified color channels emit no color parameters. For absolute styles,
 * the leading reset selects terminal defaults for those channels.
 */
template<typename Char=char>
FMT_NODISCARD inline
auto to_sgr(const text_style& ts) -> std::basic_string<Char> {
  if (ts.empty()) return {};
  auto buf = basic_memory_buffer<Char, sgr_max_length>{};
  detail::append_sgr(buf, ts);
  return { buf.data(), buf.size() };
}

#if FMT_COLOR_COMPACT_SGR

namespace detail {

template<typename Char>
FMT_CONSTEXPR20 void append_transition_code(buffer<Char>& buf, uint8_t code) {
  for (auto cp = decimal_strings[code]; *cp; ++cp)
    buf.push_back(Char(*cp));
  buf.push_back(Char(';'));
}

template<typename Char>
FMT_CONSTEXPR20
void append_transition(buffer<Char>& buf, text_style target, text_style initial)
{
  auto before = static_cast<unsigned>(to_underlying(initial.emph()));
  auto after  = static_cast<unsigned>(to_underlying(target.emph()));
  auto removed = before & ~after;
  // SGR 22 clears both intensity attributes; reapply any surviving bit.
  if ((removed & 3u) != 0) {
    append_transition_code(buf, 22);
    before &= ~3u;
  }
  constexpr uint8_t off[] = {23, 24, 25, 27, 28, 29};
  for (unsigned i = 0; i < 6; ++i) {
    if (removed & (4u << i))
      append_transition_code(buf, off[i]);
  }
  after &= ~before;
  if (after != 0) {
    auto sgr = make_emphasis<Char>(enum_cast<emphasis>(after));
    buf.append(sgr.begin(), sgr.end());
  }
  auto fg = target.fg();
  if (fg != no_color) {
    if (fg != initial.fg()) {
      auto sgr = make_foreground_color<Char>(fg);
      buf.append(sgr.begin(), sgr.end());
    }
  }
  else if (initial.has_fg()) {
    append_transition_code(buf, 39);
  }
  auto bg = target.bg();
  if (bg != no_color) {
    if (bg != initial.bg()) {
      auto sgr = make_background_color<Char>(bg);
      buf.append(sgr.begin(), sgr.end());
    }
  }
  else if (initial.has_bg()) {
    append_transition_code(buf, 49);
  }
} // append_transition

template<typename Char>
FMT_CONSTEXPR20
void append_sgr_transition(buffer<Char>& buf, text_style target,
                                              text_style initial)
{
  if (target.is_absolute()) return append_sgr(buf, target);
  initial.absolute(false);
  if (target == initial) return;
  auto relative = basic_memory_buffer<Char, sgr_max_length>{};
  append_transition(relative, target, initial);
  if (relative.size() == 0) return;
  auto absolute = basic_memory_buffer<Char, sgr_max_length>{};
  append_sgr_raw(absolute, text_style{target}.absolute());
  const auto& parameters = (absolute.size() < relative.size())
                         ? absolute : relative;
  buf.push_back(Char('\x1b'));
  buf.push_back(Char('['));
  buf.append(parameters.data(), parameters.data() + parameters.size());
  buf.data()[buf.size() - 1] = Char('m');
}

} // detail

/**
 * Returns the ANSI SGR transition from `initial` to `ts`, without a trailing
 * reset. Both arguments describe complete terminal states: empty color channels
 * mean terminal defaults, and absent emphasis means off. No colors are inherited.
 * The absolute flag on `initial` is ignored. An absolute target always resets
 * and delegates to the one-argument overload, regardless of the initial state.
 * Otherwise returns an empty string when no changes are needed.
 *
 * For relative targets, uses one unpadded sequence, independently of
 * `FMT_COLOR_COMPACT_SGR`, choosing the shorter of selective changes and reset
 * followed by the target attributes.
 * Equal lengths prefer selective changes. `Char` defaults to `char`.
 */
template<typename Char=char>
FMT_NODISCARD inline
auto to_sgr(const text_style& target, const text_style& initial)
  -> std::basic_string<Char>
{
  auto buf = basic_memory_buffer<Char, sgr_max_length>{};
  detail::append_sgr_transition(buf, target, initial);
  return {buf.data(), buf.size()};
}

#else

namespace detail {

template<typename Char>
FMT_CONSTEXPR20
void append_sgr_transition(buffer<Char>& buf, text_style target,
                                              text_style initial)
{
  if (!target.is_absolute()) {
    initial.absolute(false);
    if (target == initial) return;
    target.absolute(!initial.empty());
  }
  append_sgr(buf, target);
}

} // detail

template<typename Char=char>
FMT_NODISCARD inline auto to_sgr(text_style target, const text_style& initial)
  -> std::basic_string<Char>
{
  auto buf = basic_memory_buffer<Char, sgr_max_length>{};
  detail::append_sgr_transition(buf, target, initial);
  return {buf.data(), buf.size()};
}

#endif

/**
 * Returns a copy of `sv` wrapped in the SGR sequences for `ts` and a terminal
 * reset. Empty text returns an empty string; an empty style returns just the
 * text. The text is copied without formatting or interpreting embedded escapes.
 * The reset restores terminal defaults, not a previously active style.
 */
template<typename Char=char>
FMT_NODISCARD inline
auto wrap_sgr(const text_style& ts, basic_string_view<Char> sv)
  -> std::basic_string<Char>
{
  if (sv.size() == 0) return {};
  if (ts.empty()) return { sv.data(), sv.size() };
  constexpr auto reset = sgr_reset<Char>();
  auto result = std::basic_string<Char>{to_sgr<Char>(ts)};
  result.reserve(sgr_max_length + sgr_reset_length + sv.size());
  result.append(sv.data(), sv.size());
  result.append(reset.data(), reset.size());
  return result;
} // wrap_sgr

/// Narrow-character convenience overload of `wrap_sgr()`.
FMT_NODISCARD inline auto wrap_sgr(const text_style& ts, string_view sv)
  -> std::string
{ return wrap_sgr<char>(ts, sv); }

/**
 * The colored analog of
 * [std::vprint_nonunicode()](https://en.cppreference.com/w/cpp/io/vprint_nonunicode.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 */
inline void vprint(FILE* f, text_style ts, string_view fmt, format_args args) {
  auto buf = memory_buffer();
  detail::vformat_to(buf, ts, fmt, args);
  print(f, FMT_STRING("{}"), string_view(buf.begin(), buf.size()));
}

/**
 * The colored analog of
 * [std::print()](https://en.cppreference.com/w/cpp/io/print.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 *
 * **Example**:
 *
 *     fmt::print(stderr, fmt::bold().fg(fmt::color::red),
 *                "Elapsed time: {0:.2f} seconds", 1.23);
 */
template<typename... T>
void print(FILE* f, text_style ts, format_string<T...> fmt, T&&... args)
{ vprint(f, ts, fmt.str, vargs<T...>{{args...}}); }

/**
 * The colored analog of
 * [std::print()](https://en.cppreference.com/w/cpp/io/print.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 *
 * **Example**:
 *
 *     fmt::print(fmt::bold().fg(fmt::color::red),
 *                "Elapsed time: {0:.2f} seconds", 1.23);
 */
template<typename... T>
void print(text_style ts, format_string<T...> fmt, T&&... args)
{ return print(stdout, ts, fmt, std::forward<T>(args)...); }

/**
 * Behaves like `vprint(f, ts, fmt, args)`, with an appended newline.
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults. The newline follows the reset.
 */
inline void vprintln(FILE* f, text_style ts, string_view fmt, format_args args)
{
  auto buf = memory_buffer();
  detail::vformat_to(buf, ts, fmt, args);
  buf.push_back('\n');
  print(f, FMT_STRING("{}"), string_view(buf.begin(), buf.size()));
}

/**
 * The colored analog of
 * [std::println()](https://en.cppreference.com/w/cpp/io/println.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults. The newline follows the reset.
 *
 * **Example**:
 *
 *     fmt::println(stderr, fmt::bold().fg(fmt::color::red),
 *                  "Elapsed time: {0:.2f} seconds", 1.23);
 */
template<typename... T>
void println(FILE* f, text_style ts, format_string<T...> fmt, T&&... args)
{ vprintln(f, ts, fmt.str, vargs<T...>{{args...}}); }

/**
 * The colored analog of
 * [std::println()](https://en.cppreference.com/w/cpp/io/println.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults. The newline follows the reset.
 *
 * **Example**:
 *
 *     fmt::println(fmt::bold().fg(fmt::color::red),
 *                  "Elapsed time: {0:.2f} seconds", 1.23);
 */
template<typename... T>
void println(text_style ts, format_string<T...> fmt, T&&... args)
{ return println(stdout, ts, fmt, std::forward<T>(args)...); }

/**
 * The colored analog of
 * [std::vformat()](https://en.cppreference.com/w/cpp/utility/format/vformat.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 */
inline auto vformat(text_style ts, string_view fmt, format_args args)
  -> std::string
{
  auto buf = memory_buffer();
  detail::vformat_to(buf, ts, fmt, args);
  return fmt::to_string(buf);
}

/**
 * The colored analog of
 * [std::format()](https://en.cppreference.com/w/cpp/utility/format/format.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 * Reset does not restore a previously active style.
 *
 * **Example**:
 *
 * ```
 * #include <fmt/color.h>
 * std::string message = fmt::format(fmt::bold().fg(fmt::color::red),
 *                                   "The answer is {}", 42);
 * ```
 */
template<typename... T>
inline auto format(text_style ts, format_string<T...> fmt, T&&... args)
  -> std::string
{ return fmt::vformat(ts, fmt.str, vargs<T...>{{args...}}); }

/**
 * The colored analog of
 * [std::vformat_to()](https://en.cppreference.com/w/cpp/utility/format/vformat_to.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 */
template<typename OutputIt,
          FMT_ENABLE_IF(detail::is_output_iterator<OutputIt, char>::value)>
auto vformat_to(OutputIt out, text_style ts, string_view fmt, format_args args)
  -> OutputIt
{
  auto&& buf = detail::get_buffer<char>(out);
  detail::vformat_to(buf, ts, fmt, args);
  return detail::get_iterator(buf, out);
}

/**
 * The colored analog of
 * [std::format_to()](https://en.cppreference.com/w/cpp/utility/format/format_to.html).
 *
 * If `ts` is not empty, the formatted text is preceded by SGR sequences for its
 * specified foreground, background, and emphasis, and followed by an SGR reset
 * to terminal defaults.
 *
 * **Example**:
 *
 *     std::vector<char> out;
 *     fmt::format_to(std::back_inserter(out),
 *                    fmt::bold().fg(fmt::color::red), "{}", 42);
 */
template<typename OutputIt, typename... T,
          FMT_ENABLE_IF(detail::is_output_iterator<OutputIt, char>::value)>
inline auto format_to(OutputIt out, text_style ts, format_string<T...> fmt,
                      T&&... args) -> OutputIt
{ return vformat_to(out, ts, fmt.str, vargs<T...>{{args...}}); }

/// Formats a styled argument using the underlying type's format specifications.
template<typename T, typename Char>
struct formatter<detail::styled_arg<T>, Char> : formatter<T, Char> {
  template<typename FormatContext>
  FMT_CONSTEXPR auto format(const detail::styled_arg<T>& arg,
                            FormatContext& ctx) const -> decltype(ctx.out())
{
    const auto initial = detail::style_context::get(ctx);
    const auto effective = arg.style.is_absolute()
                         ? arg.style : fmt::overlay(initial, arg.style);
    detail::style_scope<FormatContext> scope{ctx};
    auto sgr = basic_memory_buffer<Char, sgr_max_length>{};
    detail::append_sgr_transition(sgr, effective, initial);
    auto out = detail::copy<Char>(sgr.data(), sgr.data() + sgr.size(), ctx.out());
    ctx.advance_to(out);
    detail::style_context::set(ctx, effective);
    out = formatter<T, Char>::format(arg.value, ctx);
    sgr.clear();
    detail::append_sgr_transition(sgr, initial, detail::style_context::get(ctx));
    return detail::copy<Char>(sgr.data(), sgr.data() + sgr.size(), out);
  }
}; // formatter

/** Returns a formatting argument that applies text style `ts` to `value` using
 * ANSI escape sequences. The underlying type's format specifications are
 * preserved, including width, alignment, and precision. Padding produced by the
 * underlying formatter is inside the styled span.
 *
 * The result borrows `value` and stores a copy of `ts`. It does not copy or own
 * `value`, which must remain alive whenever the result is formatted. A
 * temporary value can be used in an immediate formatting call, but storing the
 * result does not extend that temporary's lifetime.
 *
 * Relative styles inherit the enclosing formatting context's missing colors
 * and combine emphasis. Absolute styles use terminal defaults for unspecified
 * components. After the value, including padding, the enclosing style is
 * restored. Ordinary formatting starts from terminal defaults; styling already
 * active outside this formatting operation and raw escapes are not tracked.
 *
 * **Example**:
 *
 *     fmt::print("Elapsed time: {0:.2f} seconds",
 *                fmt::styled(1.23, fmt::fg(fmt::color::green)
 *                                      .bg(fmt::color::midnight_blue)));
 */
template<typename T>
constexpr auto styled(const T& value, text_style ts)
    -> detail::styled_arg<remove_cvref_t<T>>
{ return detail::styled_arg<remove_cvref_t<T>>{value, ts}; }

/**
 * Named RGB colors.
 *
 * This namespace contains constexpr `rgb` constants with fixed RGB values,
 * independently of the terminal palette. Pass a value to `fg()` or `bg()`.
 */
namespace color {
constexpr FMT_INLINE_VARIABLE rgb alice_blue              {240, 248, 255};
constexpr FMT_INLINE_VARIABLE rgb antique_white           {250, 235, 215};
constexpr FMT_INLINE_VARIABLE rgb aqua                    {  0, 255, 255};
constexpr FMT_INLINE_VARIABLE rgb aquamarine              {127, 255, 212};
constexpr FMT_INLINE_VARIABLE rgb azure                   {240, 255, 255};
constexpr FMT_INLINE_VARIABLE rgb beige                   {245, 245, 220};
constexpr FMT_INLINE_VARIABLE rgb bisque                  {255, 228, 196};
constexpr FMT_INLINE_VARIABLE rgb black                   {  0,   0,   0};
constexpr FMT_INLINE_VARIABLE rgb blanched_almond         {255, 235, 205};
constexpr FMT_INLINE_VARIABLE rgb blue                    {  0,   0, 255};
constexpr FMT_INLINE_VARIABLE rgb blue_violet             {138,  43, 226};
constexpr FMT_INLINE_VARIABLE rgb brown                   {165,  42,  42};
constexpr FMT_INLINE_VARIABLE rgb burly_wood              {222, 184, 135};
constexpr FMT_INLINE_VARIABLE rgb cadet_blue              { 95, 158, 160};
constexpr FMT_INLINE_VARIABLE rgb chartreuse              {127, 255,   0};
constexpr FMT_INLINE_VARIABLE rgb chocolate               {210, 105,  30};
constexpr FMT_INLINE_VARIABLE rgb coral                   {255, 127,  80};
constexpr FMT_INLINE_VARIABLE rgb cornflower_blue         {100, 149, 237};
constexpr FMT_INLINE_VARIABLE rgb cornsilk                {255, 248, 220};
constexpr FMT_INLINE_VARIABLE rgb crimson                 {220,  20,  60};
constexpr FMT_INLINE_VARIABLE rgb cyan                    {  0, 255, 255};
constexpr FMT_INLINE_VARIABLE rgb dark_blue               {  0,   0, 139};
constexpr FMT_INLINE_VARIABLE rgb dark_cyan               {  0, 139, 139};
constexpr FMT_INLINE_VARIABLE rgb dark_golden_rod         {184, 134,  11};
constexpr FMT_INLINE_VARIABLE rgb dark_gray               {169, 169, 169};
constexpr FMT_INLINE_VARIABLE rgb dark_green              {  0, 100,   0};
constexpr FMT_INLINE_VARIABLE rgb dark_khaki              {189, 183, 107};
constexpr FMT_INLINE_VARIABLE rgb dark_magenta            {139,   0, 139};
constexpr FMT_INLINE_VARIABLE rgb dark_olive_green        { 85, 107,  47};
constexpr FMT_INLINE_VARIABLE rgb dark_orange             {255, 140,   0};
constexpr FMT_INLINE_VARIABLE rgb dark_orchid             {153,  50, 204};
constexpr FMT_INLINE_VARIABLE rgb dark_red                {139,   0,   0};
constexpr FMT_INLINE_VARIABLE rgb dark_salmon             {233, 150, 122};
constexpr FMT_INLINE_VARIABLE rgb dark_sea_green          {143, 188, 143};
constexpr FMT_INLINE_VARIABLE rgb dark_slate_blue         { 72,  61, 139};
constexpr FMT_INLINE_VARIABLE rgb dark_slate_gray         { 47,  79,  79};
constexpr FMT_INLINE_VARIABLE rgb dark_turquoise          {  0, 206, 209};
constexpr FMT_INLINE_VARIABLE rgb dark_violet             {148,   0, 211};
constexpr FMT_INLINE_VARIABLE rgb deep_pink               {255,  20, 147};
constexpr FMT_INLINE_VARIABLE rgb deep_sky_blue           {  0, 191, 255};
constexpr FMT_INLINE_VARIABLE rgb dim_gray                {105, 105, 105};
constexpr FMT_INLINE_VARIABLE rgb dodger_blue             { 30, 144, 255};
constexpr FMT_INLINE_VARIABLE rgb fire_brick              {178,  34 , 34};
constexpr FMT_INLINE_VARIABLE rgb floral_white            {255, 250, 240};
constexpr FMT_INLINE_VARIABLE rgb forest_green            {34, 139,   34};
constexpr FMT_INLINE_VARIABLE rgb fuchsia                 {255,   0, 255};
constexpr FMT_INLINE_VARIABLE rgb gainsboro               {220, 220, 220};
constexpr FMT_INLINE_VARIABLE rgb ghost_white             {248, 248, 255};
constexpr FMT_INLINE_VARIABLE rgb gold                    {255, 215,   0};
constexpr FMT_INLINE_VARIABLE rgb golden_rod              {218, 165,  32};
constexpr FMT_INLINE_VARIABLE rgb gray                    {128, 128, 128};
constexpr FMT_INLINE_VARIABLE rgb green                   {  0, 128  , 0};
constexpr FMT_INLINE_VARIABLE rgb green_yellow            {173, 255,  47};
constexpr FMT_INLINE_VARIABLE rgb honey_dew               {240, 255, 240};
constexpr FMT_INLINE_VARIABLE rgb hot_pink                {255, 105, 180};
constexpr FMT_INLINE_VARIABLE rgb indian_red              {205,  92,  92};
constexpr FMT_INLINE_VARIABLE rgb indigo                  {75,    0, 130};
constexpr FMT_INLINE_VARIABLE rgb ivory                   {255, 255, 240};
constexpr FMT_INLINE_VARIABLE rgb khaki                   {240, 230, 140};
constexpr FMT_INLINE_VARIABLE rgb lavender                {230, 230, 250};
constexpr FMT_INLINE_VARIABLE rgb lavender_blush          {255, 240, 245};
constexpr FMT_INLINE_VARIABLE rgb lawn_green              {124, 252,   0};
constexpr FMT_INLINE_VARIABLE rgb lemon_chiffon           {255, 250, 205};
constexpr FMT_INLINE_VARIABLE rgb light_blue              {173, 216, 230};
constexpr FMT_INLINE_VARIABLE rgb light_coral             {240, 128, 128};
constexpr FMT_INLINE_VARIABLE rgb light_cyan              {224, 255, 255};
constexpr FMT_INLINE_VARIABLE rgb light_golden_rod_yellow {250, 250, 210};
constexpr FMT_INLINE_VARIABLE rgb light_gray              {211, 211, 211};
constexpr FMT_INLINE_VARIABLE rgb light_green             {144, 238, 144};
constexpr FMT_INLINE_VARIABLE rgb light_pink              {255, 182, 193};
constexpr FMT_INLINE_VARIABLE rgb light_salmon            {255, 160, 122};
constexpr FMT_INLINE_VARIABLE rgb light_sea_green         { 32, 178, 170};
constexpr FMT_INLINE_VARIABLE rgb light_sky_blue          {135, 206, 250};
constexpr FMT_INLINE_VARIABLE rgb light_slate_gray        {119, 136, 153};
constexpr FMT_INLINE_VARIABLE rgb light_steel_blue        {176, 196, 222};
constexpr FMT_INLINE_VARIABLE rgb light_yellow            {255, 255, 224};
constexpr FMT_INLINE_VARIABLE rgb lime                    {  0, 255,   0};
constexpr FMT_INLINE_VARIABLE rgb lime_green              { 50, 205,  50};
constexpr FMT_INLINE_VARIABLE rgb linen                   {250, 240, 230};
constexpr FMT_INLINE_VARIABLE rgb magenta                 {255,   0, 255};
constexpr FMT_INLINE_VARIABLE rgb maroon                  {128,   0,   0};
constexpr FMT_INLINE_VARIABLE rgb medium_aquamarine       {102, 205, 170};
constexpr FMT_INLINE_VARIABLE rgb medium_blue             {  0,   0, 205};
constexpr FMT_INLINE_VARIABLE rgb medium_orchid           {186,  85, 211};
constexpr FMT_INLINE_VARIABLE rgb medium_purple           {147, 112, 219};
constexpr FMT_INLINE_VARIABLE rgb medium_sea_green        { 60, 179, 113};
constexpr FMT_INLINE_VARIABLE rgb medium_slate_blue       {123, 104, 238};
constexpr FMT_INLINE_VARIABLE rgb medium_spring_green     {  0, 250, 154};
constexpr FMT_INLINE_VARIABLE rgb medium_turquoise        { 72, 209, 204};
constexpr FMT_INLINE_VARIABLE rgb medium_violet_red       {199,  21, 133};
constexpr FMT_INLINE_VARIABLE rgb midnight_blue           { 25,  25, 112};
constexpr FMT_INLINE_VARIABLE rgb mint_cream              {245, 255, 250};
constexpr FMT_INLINE_VARIABLE rgb misty_rose              {255, 228, 225};
constexpr FMT_INLINE_VARIABLE rgb moccasin                {255, 228, 181};
constexpr FMT_INLINE_VARIABLE rgb navajo_white            {255, 222, 173};
constexpr FMT_INLINE_VARIABLE rgb navy                    {  0,   0, 128};
constexpr FMT_INLINE_VARIABLE rgb old_lace                {253, 245, 230};
constexpr FMT_INLINE_VARIABLE rgb olive                   {128, 128  , 0};
constexpr FMT_INLINE_VARIABLE rgb olive_drab              {107, 142,  35};
constexpr FMT_INLINE_VARIABLE rgb orange                  {255, 165,   0};
constexpr FMT_INLINE_VARIABLE rgb orange_red              {255,  69,   0};
constexpr FMT_INLINE_VARIABLE rgb orchid                  {218, 112, 214};
constexpr FMT_INLINE_VARIABLE rgb pale_golden_rod         {238, 232, 170};
constexpr FMT_INLINE_VARIABLE rgb pale_green              {152, 251, 152};
constexpr FMT_INLINE_VARIABLE rgb pale_turquoise          {175, 238, 238};
constexpr FMT_INLINE_VARIABLE rgb pale_violet_red         {219, 112, 147};
constexpr FMT_INLINE_VARIABLE rgb papaya_whip             {255, 239, 213};
constexpr FMT_INLINE_VARIABLE rgb peach_puff              {255, 218, 185};
constexpr FMT_INLINE_VARIABLE rgb peru                    {205, 133,  63};
constexpr FMT_INLINE_VARIABLE rgb pink                    {255, 192, 203};
constexpr FMT_INLINE_VARIABLE rgb plum                    {221, 160, 221};
constexpr FMT_INLINE_VARIABLE rgb powder_blue             {176, 224, 230};
constexpr FMT_INLINE_VARIABLE rgb purple                  {128,   0, 128};
constexpr FMT_INLINE_VARIABLE rgb rebecca_purple          {102,  51, 153};
constexpr FMT_INLINE_VARIABLE rgb red                     {255,   0,   0};
constexpr FMT_INLINE_VARIABLE rgb rosy_brown              {188, 143, 143};
constexpr FMT_INLINE_VARIABLE rgb royal_blue              { 65, 105, 225};
constexpr FMT_INLINE_VARIABLE rgb saddle_brown            {139,  69,  19};
constexpr FMT_INLINE_VARIABLE rgb salmon                  {250, 128, 114};
constexpr FMT_INLINE_VARIABLE rgb sandy_brown             {244, 164,  96};
constexpr FMT_INLINE_VARIABLE rgb sea_green               { 46, 139,  87};
constexpr FMT_INLINE_VARIABLE rgb sea_shell               {255, 245, 238};
constexpr FMT_INLINE_VARIABLE rgb sienna                  {160,  82,  45};
constexpr FMT_INLINE_VARIABLE rgb silver                  {192, 192, 192};
constexpr FMT_INLINE_VARIABLE rgb sky_blue                {135, 206, 235};
constexpr FMT_INLINE_VARIABLE rgb slate_blue              {106,  90, 205};
constexpr FMT_INLINE_VARIABLE rgb slate_gray              {112, 128, 144};
constexpr FMT_INLINE_VARIABLE rgb snow                    {255, 250, 250};
constexpr FMT_INLINE_VARIABLE rgb spring_green            {  0, 255, 127};
constexpr FMT_INLINE_VARIABLE rgb steel_blue              { 70, 130, 180};
constexpr FMT_INLINE_VARIABLE rgb tan                     {210, 180, 140};
constexpr FMT_INLINE_VARIABLE rgb teal                    {  0, 128, 128};
constexpr FMT_INLINE_VARIABLE rgb thistle                 {216, 191, 216};
constexpr FMT_INLINE_VARIABLE rgb tomato                  {255,  99,  71};
constexpr FMT_INLINE_VARIABLE rgb turquoise               { 64, 224, 208};
constexpr FMT_INLINE_VARIABLE rgb violet                  {238, 130, 238};
constexpr FMT_INLINE_VARIABLE rgb wheat                   {245, 222, 179};
constexpr FMT_INLINE_VARIABLE rgb white                   {255, 255, 255};
constexpr FMT_INLINE_VARIABLE rgb white_smoke             {245, 245, 245};
constexpr FMT_INLINE_VARIABLE rgb yellow                  {255, 255,   0};
constexpr FMT_INLINE_VARIABLE rgb yellow_green            {154, 205,  50};
} // color

FMT_END_EXPORT
FMT_END_NAMESPACE

FMT_BEGIN_EXPORT
namespace std {

template<> struct hash<fmt::rgb> {
  auto operator()(fmt::rgb value) const noexcept -> size_t {
    return hash<fmt::rgb::value_type>{}(fmt::to_underlying(value));
  }
};

template<> struct hash<fmt::color_t> {
  auto operator()(fmt::color_t value) const noexcept -> size_t {
    return hash<fmt::color_t::value_type>{}(fmt::to_underlying(value));
  }
};

template<> struct hash<fmt::text_style> {
  auto operator()(fmt::text_style value) const noexcept -> size_t {
    return hash<fmt::text_style::value_type>{}(fmt::to_underlying(value));
  }
};

} // std
FMT_END_EXPORT

#undef FMT_COLOR_COMPARISONS

#endif  // FMT_COLOR_H_
