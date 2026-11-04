# Colors

Choose among four color types for foreground and background styling:

- [`term_color`](#terminal-palette-colors) selects the terminal's 16 named palette colors and follows its
  theme when rendered directly.
- [`rgb`](#rgb-values) provides named or custom truecolor values with independent red, green,
  and blue channels.
- [`gray_color`](#gray-palette-colors) offers 24 fixed grayscale levels between black and white.
- [`cube_color`](#cube-palette-colors) selects from the 216-color cube: a compact alternative when SGR
  sequence size matters or a terminal supports 256 colors but lacks RGB mode.

Use these values with a [`text_style` constructor or fluent setters](text-style.md).
This page describes their ranges, RGB conversions, blending, and the public
[`color_t`](#color-variants) variant, including unspecified colors. See the [overview](index.md)
for examples and [Named Colors](named-colors.md) for swatches.

## Terminal palette colors

See the [normal and bright terminal-color swatches](named-colors.md#terminal-colors)
for their fixed Xterm-default RGB equivalents.

::: term_color
    options:
      color_reference: true

!!! note "Backward compatibility"

    `fmt::terminal_color` remains an alias for `fmt::term_color`.
    Both names denote the same type and provide the same color constants.

Terminal colors use the terminal's palette rather than fixed RGB values.
Their enum values are contiguous palette indices, separate from the ANSI
SGR codes emitted for foreground and background colors. The rightmost
column gives the fixed Xterm-default RGB equivalent, which may differ from
a customized terminal theme:

| Color            | Value | FG SGR | BG SGR | RGB equivalent    |
| ---------------- | ----: | -----: | -----: | ----------------- |
| `black`          |     0 |     30 |     40 | `{  0,   0,   0}` |
| `red`            |     1 |     31 |     41 | `{205,   0,   0}` |
| `green`          |     2 |     32 |     42 | `{  0, 205,   0}` |
| `yellow`         |     3 |     33 |     43 | `{205, 205,   0}` |
| `blue`           |     4 |     34 |     44 | `{  0,   0, 238}` |
| `magenta`        |     5 |     35 |     45 | `{205,   0, 205}` |
| `cyan`           |     6 |     36 |     46 | `{  0, 205, 205}` |
| `white`          |     7 |     37 |     47 | `{229, 229, 229}` |
| `bright_black`   |     8 |     90 |    100 | `{127, 127, 127}` |
| `bright_red`     |     9 |     91 |    101 | `{255,   0,   0}` |
| `bright_green`   |    10 |     92 |    102 | `{  0, 255,   0}` |
| `bright_yellow`  |    11 |     93 |    103 | `{255, 255,   0}` |
| `bright_blue`    |    12 |     94 |    104 | `{ 92,  92, 255}` |
| `bright_magenta` |    13 |     95 |    105 | `{255,   0, 255}` |
| `bright_cyan`    |    14 |     96 |    106 | `{  0, 255, 255}` |
| `bright_white`   |    15 |     97 |    107 | `{255, 255, 255}` |

```cpp
auto style     = fmt::fg(fmt::term_color::black);
bool specified = style.has_fg(); // true
```

!!! note "Changed behavior"

    Previously, `terminal_color` values were foreground SGR codes 30-37 and
    90-97.  They are now palette indices 0-15. Symbolic names and emitted SGR
    sequences are unchanged, but numeric conversions and stored terminal-color
    values differ.

Use symbolic terminal-color names in styling code. Their numeric values and
the packed style encoding are not stable serialization formats.

`term_color` has the signed underlying type `int8_t`.
`fmt::underlying_type<fmt::term_color>::type` and
`fmt::to_underlying(term_color)` also use `int8_t`. The named values are
indices `0..15`; the signed type does not introduce additional palette colors.
Conversion to RGB asserts that the index is within that range before accessing
the fixed palette. Prefer the named values rather than casting arbitrary integers.

### Converting terminal colors to RGB

`term_color` converts implicitly to `rgb` using the fixed Xterm-default
palette. This conversion does not query the terminal's current theme.
For example, terminal red converts to `{205, 0, 0}`, while bright red converts
to `{255, 0, 0}`.

```cpp
fmt::rgb red   = fmt::term_color::red; // {205, 0, 0}
auto fixed_red = fmt::color::term_red; // The same RGB value.
```

The `fmt::color::term_*` constants provide fixed RGB counterparts for all
16 terminal colors, including `term_bright_*` for bright colors. They render
as truecolor. A `term_color` renders using the terminal's palette, so its
appearance can differ from its fixed RGB counterpart.

## RGB values

::: rgb
    options:
      color_reference: true

The three-channel constructor accepts `int` arguments and checks each against
`0..255` before storing it as `uint8_t`. Invalid channels trigger `FMT_ASSERT`.
When assertions are disabled, negative channels clamp to 0 and channels above
255 clamp to 255. This constructor is `noexcept`; valid inputs are usable
in C++11 constant expressions. Byte-valued arguments can be mixed with `int`
arguments; all three use the same checked constructor. The public `r`, `g`,
and `b` members and `chan_type` are `uint8_t`.
The packed `rgb(uint32_t)` constructor treats its argument as a bit
pattern and uses its low 24 bits; it is `noexcept`.

```cpp
int red = 30;
uint8_t green = 144;
auto accent = fmt::rgb{red, green, 255};
// With assertions disabled, fmt::rgb{-1, 144, 300} becomes {0, 144, 255}.
```

### Converting colors to RGB

::: to_rgb(T)
    options:
      color_reference: true

::: to_rgb(no_color_t)
    options:
      color_reference: true

::: to_rgb(color_t)
    options:
      color_reference: true

Use `fmt::to_rgb(value)` to request a fixed RGB equivalent explicitly. The
overload is selected by the input type:

| Input                                           | Result       | Behavior                                                                                 |
| ----------------------------------------------- | ------------ | ---------------------------------------------------------------------------------------- |
| `rgb`, `term_color`, `gray_color`, `cube_color` | `rgb`        | RGB is unchanged; palette colors use their fixed RGB equivalents.                        |
| `no_color_t`                                    | `no_color_t` | Remains unspecified, distinct from RGB black.                                            |
| `color_t`                                       | `color_t`    | A specified color becomes the RGB alternative; an unspecified color remains unspecified. |

```cpp
constexpr auto red   = fmt::to_rgb(fmt::term_color::red); // rgb{205, 0, 0}
constexpr auto empty = fmt::to_rgb(fmt::no_color);        // no_color_t
constexpr auto shade = fmt::to_rgb(fmt::color_t{fmt::gray_color{12}});
// shade holds rgb{128, 128, 128}.
```

All overloads leave their arguments unchanged, are `noexcept`, and support
constant evaluation in C++11 and later. The plain-color overload accepts only
the four color types. Terminal conversion uses the fixed Xterm-default palette,
not the terminal's current theme. See [Color variants](#color-variants) for
accessing an RGB value held in a `color_t`.

### Combining RGB values

`operator|` returns a new `rgb` by taking the bitwise OR of corresponding red,
green, and blue components. `operator|=` updates its left operand and returns
a reference to it. These operations do not average colors or perform alpha
compositing. Both operations are `noexcept`. `operator|` supports constant
evaluation in C++11; the mutating `operator|=` supports it in C++14 and later
on supported compilers.

```cpp
auto magenta = fmt::rgb{255, 0, 0} | fmt::rgb{0, 0, 255};  // {255, 0, 255}
auto mixed   = fmt::rgb{0xC0F000};
mixed |= fmt::rgb{0x000FEE};  // {192, 255, 238}, packed 0x00C0FFEE
```

### Averaging RGB values

::: blend
    options:
      color_reference: true

When every argument is a plain color (`rgb`, `term_color`, `cube_color`, or
`gray_color`), `fmt::blend` returns a new `rgb` containing their equally
weighted channel averages. Palette arguments convert to their fixed RGB
equivalents first. Fractional channel values are rounded down after all
arguments are summed. At least two arguments are required.
This averages the stored RGB components without gamma correction or alpha
compositing.

```cpp
auto gray = fmt::blend(fmt::rgb{255,   0,   0},
                       fmt::rgb{  0, 255,   0},
                       fmt::rgb{  0,   0, 255});  // {85, 85, 85}

auto light_gray = fmt::blend(fmt::rgb{},
                             fmt::rgb{255, 255, 255},
                             fmt::rgb{255, 255, 255});  // {170, 170, 170}

auto mixed = fmt::blend(fmt::term_color::red, fmt::cube_color{1, 2, 3},
                        fmt::gray_color{12}); // rgb{142, 87, 101}
```

Blending requires at least two arguments and rejects non-color arguments during
overload resolution. It is `noexcept` and supports constant evaluation in
C++11 and later. In the C++23 implementation, the `fmt::Color` concept
identifies the four plain color types, including const and reference forms;
it excludes `color_t` and `no_color_t`. Earlier modes use equivalent trait
constraints.

The free function leaves its arguments unchanged. Passing a `color_t` or
`no_color` selects [variant blending](#blending-color-variants) instead.

To update an existing RGB color, assign the result of the free function:

```cpp
auto color = fmt::rgb{};
color = fmt::blend(color, fmt::rgb{255, 255, 255}); // rgb{127, 127, 127}
```

### Named RGB colors

::: color
    options:
      color_reference: true

See [Named Colors](named-colors.md) for the available names and their colors.

Each name denotes a constexpr `fmt::rgb` value. Use `fmt::rgb` to store named
and custom colors in the same variable:

```cpp
auto accent = fmt::color::coral;
accent = fmt::rgb{30, 144, 255};
auto style = fmt::fg(accent);
```

### Comparisons

`rgb` values support equality and ordering by their packed `0xRRGGBB` values:
red first, then green, then blue. This is numeric ordering, rather than an
ordering by brightness. They can be used as keys in `std::map` and `std::set`.
In C++20 and later, `operator<=>` returns `std::strong_ordering`.

`std::hash<fmt::rgb>` hashes the packed `uint32_t` representation, allowing RGB
values to be keys in `std::unordered_map` and `std::unordered_set`. Equal RGB
values have equal hashes. Hash values are not a stable serialization format.

!!! note "Changed behavior"

    `fmt::color` was previously an enum type. It is now a namespace containing
    RGB constants. The color names and RGB values are unchanged; `fmt::color`
    is no longer a type, and its named values are no longer enum values.

### Integer representation

`fmt::underlying_type<fmt::rgb>::type` names the integer representation type.
`fmt::to_underlying(fmt::rgb value)` returns that representation:

```cpp
using packed_t = fmt::underlying_type<fmt::rgb>::type;
constexpr auto coral  = fmt::color::coral;
constexpr auto packed = fmt::to_underlying(coral);  // 0x00FF7F50
static_assert(std::is_same<const packed_t, decltype(packed)>::value,
              "packed RGB type");
static_assert(fmt::rgb{packed} == coral, "packed RGB round trip");
```
Packed integer conversions are explicit in both directions. Direct construction
accepts `0xRRGGBB` and ignores the high byte; conversion back produces
`0x00RRGGBB`:

```cpp
auto sample = fmt::rgb{0xab123456};
auto packed = static_cast<uint32_t>(sample); // 0x00123456
auto named  = static_cast<uint32_t>(fmt::color::coral); // 0x00FF7F50
```

!!! note "Changed behavior"

    Packed integer construction of `rgb` was previously implicit. It is now
    explicit; conversion from `rgb` to a packed integer is also explicit.

## Gray palette colors

See the [gray palette swatches](named-colors.md#gray-palette-colors) for all
24 levels from darkest to lightest.

::: gray_color
    options:
      color_reference: true

`gray_color` selects one of the terminal's 24 gray levels. Its explicit
constructor accepts an `int` zero-based index in the range 0–23; default
construction selects index zero. Invalid indices trigger `FMT_ASSERT`. When
assertions are disabled, negative indices clamp to 0 and indices above 23
clamp to 23. The index constructor is `noexcept`; valid inputs support
constant evaluation in C++11 and later.

```cpp
auto shade = fmt::gray_color{12};
auto style = fmt::bg(shade);
fmt::rgb converted = shade; // {128, 128, 128}
```

Implicit conversion to `rgb` sets every channel to `8 + 10 * index`, from
`{8, 8, 8}` through `{238, 238, 238}`. `value_type` and
`underlying_type<gray_color>::type` are `uint8_t`;
`to_underlying(color)` returns the zero-based gray index.
For terminal output, the library adds 232 to select palette entries 232–255,
using SGR `38;5;n` for foreground and `48;5;n` for background.

## Cube palette colors

See the [cube palette table](named-colors.md#cube-palette-colors) for all
216 colors, arranged by red, green, and blue indices.

::: cube_color
    options:
      color_reference: true

`cube_color` selects one of the 216 colors in the terminal's 6×6×6 color cube.
Construct it from three `int` channel indices in the range 0–5, or explicitly
from a single `unsigned` zero-based index in the range 0–215. Invalid inputs
trigger `FMT_ASSERT`. When assertions are disabled, each channel clamps to
`0..5`, and a single index above 215 clamps to 215 before channel decomposition.
The single index is `36 * r + 6 * g + b`. Default construction selects index zero.

```cpp
auto accent = fmt::cube_color{1, 2, 3};
auto same   = fmt::cube_color{51}; // 36 * 1 + 6 * 2 + 3
auto style  = fmt::fg(accent);
accent.r(5).g(0).b(4); // Channel setters return cube_color&.
```

The `r`, `g`, and `b` getters return channel indices as `int`. Their
setters accept `int`, assert that the input is in `0..5`, and return
`cube_color&` for chaining. With assertions disabled, an out-of-range value
sets that channel to the nearest endpoint while preserving the other channels.
Constructors and setters are `noexcept`. Valid construction supports constant
evaluation in C++11 and later; setters support it in C++14 and later on
supported compilers.

The default assertion handler terminates on invalid input in assertion-enabled
builds. Defining `NDEBUG` disables these assertions and enables the clamping
fallback. This behavior is independent of exception support: these range checks
never throw, including in builds with exceptions disabled.

Implicit conversion to `rgb` maps channel indices 0–5 to the fixed levels
`{0, 95, 135, 175, 215, 255}`:

```cpp
fmt::rgb converted = fmt::cube_color{1, 2, 3}; // {95, 135, 175}
```

`value_type` and `underlying_type<cube_color>::type` are `uint8_t`.
`to_underlying(color)` returns the zero-based cube index as `uint8_t`.
Channel storage and its `chan_type` alias are private implementation details.
For terminal output, the library adds 16 to select palette entries 16–231.
Foreground uses SGR `38;5;n`; background uses `48;5;n`.

Both palette types support equality and ordering by their zero-based indices.
Their conversions to RGB are `noexcept` and support constant evaluation in
C++11, as does terminal-to-RGB conversion.

Palette rendering uses the terminal's palette entries. RGB conversion uses
fixed values, so a customized terminal palette can produce a different
appearance. With `FMT_COLOR_COMPACT_SGR` undefined or `0`, palette indices in
escape sequences use three decimal digits, including leading zeroes: cube
index zero emits `38;5;016` for a foreground color. With the definition set to
`1`, it emits `38;5;16`. See [SGR output modes](format.md#to_sgr).

## Unspecified color

::: no_color_t
    options:
      color_reference: true

::: no_color
    options:
      color_reference: true

An unspecified color adds no foreground or background styling. It is distinct
from the color black.

```cpp
auto unspecified    = fmt::fg(fmt::no_color);          // No foreground.
auto rgb_black      = fmt::fg(fmt::rgb{});             // Black foreground.
auto terminal_black = fmt::fg(fmt::term_color::black); // Black foreground.
```

Both `fmt::fg(fmt::no_color)` and `fmt::bg(fmt::no_color)` produce empty styles.
They do not clear a color already present in another style when combined with
`|` or `|=`, and they do not emit a terminal reset.

## Color variants

`fmt::color_t` is a public value type for storing different kinds of color in
one variable. It is available in C++11 and later and uses its own union-based
implementation in every supported language mode; it does not require
`std::variant`.

::: color_t
    options:
      color_reference: true

### Construction and assignment

Default construction holds `no_color_t`. Construction from `no_color`, an
`rgb` (including a named RGB constant), `term_color`, `cube_color`, or
`gray_color` is implicit and `noexcept`. Assignment replaces the active
alternative and is also `noexcept`.
Copying or moving a `color_t` preserves its kind and value.

```cpp
fmt::color_t accent;                    // Unspecified.
accent = fmt::color::coral;             // Holds rgb.
accent = fmt::term_color::red;          // Holds term_color.
accent = fmt::cube_color{1, 2, 3};      // Holds cube_color.
accent = fmt::gray_color{12};           // Holds gray_color.
auto style = fmt::text_style{accent};
style.bg(fmt::color_t{fmt::color::blue});
accent = fmt::no_color;                 // Unspecified again; style is unchanged.
```

`text_style` constructors, color setters, and free `fg`/`bg` accept
`color_t`. The style's `fg` and `bg` getters return independent `color_t`
values. Changing a stored color or an extracted color leaves the style
unchanged; use a style setter to replace a channel.

### Alternatives and indices

`index` returns the active alternative's index as `std::size_t`:

| Index | `color_t::kind` | Alternative  | Meaning                |
| ----: | :-------------: | ------------ | ---------------------- |
|     0 |      `none`     | `no_color_t` | Unspecified color      |
|     1 |      `rgb`      | `rgb`        | Three RGB channels     |
|     2 |      `term`     | `term_color` | Terminal palette color |
|     3 |      `cube`     | `cube_color` | 6×6×6 palette color    |
|     4 |      `gray`     | `gray_color` | 24-level gray color    |

RGB black and terminal black remain specified alternatives, distinct from
`no_color_t`.

### Accessing a color

::: holds_alternative
    options:
      color_reference: true

::: get(color_t&)
    options:
      color_reference: true

::: get_if(color_t*)
    options:
      color_reference: true

`holds_alternative<T>(color)` reports whether the color holds `T`.
`get<T>(color)` returns a reference to that alternative and throws
`format_error` if another alternative is active. `get_if<T>(&color)` returns
a pointer to the alternative, or `nullptr` for a different kind or a null
input pointer. Classification and pointer access are `noexcept`.

Both access functions also accept an index: `get<1>(color)` and
`get_if<1>(&color)` access RGB. `T` must be one of the listed alternatives;
an index must be in the range 0–4. An unsupported type or index is a compile-time
error. Use `color_t::kind` values with `to_underlying` or `enum_cast` when
selecting an index by name.

The `get` overloads preserve the argument's constness and value category:

| Color argument    | Type-based result | Index-based result                                |
| ----------------- | ----------------- | ------------------------------------------------- |
| `color_t&`        | `T&`              | Reference to the indexed alternative              |
| `const color_t&`  | `const T&`        | Const reference to the indexed alternative        |
| `color_t&&`       | `T&&`             | Rvalue reference to the indexed alternative       |
| `const color_t&&` | `const T&&`       | Const rvalue reference to the indexed alternative |

`get_if` returns `T*` for a mutable color pointer and `const T*` for a const
color pointer. Mutable references and pointers allow changes to the active
color value.

```cpp
fmt::color_t accent = fmt::color::coral;
if (auto* channels = fmt::get_if<fmt::rgb>(&accent))
  channels->g = 128;

bool is_rgb = fmt::holds_alternative<fmt::rgb>(accent); // true
constexpr auto rgb_index = fmt::enum_cast<std::size_t>(fmt::color_t::kind::rgb);
auto red = fmt::get<rgb_index>(accent).r;
```

References and pointers refer to storage inside the `color_t`; they must not
outlive it or an alternative change. Style color getters return temporary
values, so copy an alternative when extracting it directly:

```cpp
auto style = fmt::text_style{fmt::color::coral};
auto channels = fmt::get<fmt::rgb>(style.fg()); // Copies RGB before the temporary dies.
```

Const and mutable access, classification, and `index` support constant
evaluation in C++11. Modifying a color through mutable access or assigning
an alternative requires C++14 or later for constant evaluation on supported
compilers. The free `fg` and `bg` style factories support constant
evaluation in C++11.

### Visitation

`fmt::visit(visitor, color)` calls the visitor with the active alternative of
one `color_t`. It preserves the color argument's constness and value category
and forwards the visitor. The visitor must accept all five alternatives
with exactly the same return type. Value, reference, and `void` results are
supported; visitor exceptions propagate.

A function object with overloads works in C++11:

```cpp
struct describe_color {
  const char* operator()(fmt::no_color_t) const { return "unspecified";      }
  const char* operator()(fmt::rgb)        const { return "RGB";              }
  const char* operator()(fmt::term_color) const { return "terminal palette"; }
  const char* operator()(fmt::cube_color) const { return "cube palette";     }
  const char* operator()(fmt::gray_color) const { return "gray palette";     }
};

fmt::color_t accent = fmt::color::coral;
const char* description = fmt::visit(describe_color{}, accent); // "RGB"
```

A visitor can instead use a templated call operator that accepts every
alternative. In C++14 and later, a generic lambda is another option:

```cpp
auto payload = fmt::visit([](const auto& value) -> uint32_t {
  return fmt::to_underlying(value);
}, accent);
```

The explicit return type gives every alternative the same result type.
This example returns only the numeric payload, without the kind discriminator.
Visitation supports C++11 constant evaluation when the visitor and color access do.

### Blending color variants

::: blend(color_t, Second, Colors...)
    options:
      color_reference: true

If any argument is a `color_t` or `no_color`, `fmt::blend` returns a
`color_t`. At least two arguments are required. Plain colors and variants can
appear in any argument position.
Unspecified colors are ignored: they contribute neither RGB components nor
weight to the average.

- With no specified colors, the result holds `no_color_t`.
- With one specified color, the result preserves its kind and value.
- With two or more specified colors, the result holds their equal-weight
  RGB average, even when the original colors have the same palette kind.

```cpp
constexpr auto empty = fmt::blend(fmt::no_color, fmt::color_t{});
// Unspecified.
constexpr auto shade = fmt::blend(fmt::no_color, fmt::gray_color{12});
// Holds gray_color{12}.
constexpr auto mixed = fmt::blend(fmt::cube_color{1, 2, 3},
                                  fmt::color_t{fmt::gray_color{12}});
// Holds rgb{111, 131, 151}.
```

Palette conversion uses fixed RGB values, including the Xterm-default terminal
palette. Channels are summed before division, with fractional results rounded
down. The operation is `noexcept`, leaves its arguments unchanged, and supports
constant evaluation in C++11 and later. It requires at least two arguments and
rejects non-color arguments during overload resolution.

### Composition, comparisons, and numeric representation

`color_t` supports `|` and `|=`. An unspecified color takes the other operand's
value and preserves its kind. If both colors are specified, both convert to
RGB and their corresponding channel bits combine by OR. The result holds
`rgb`, even when both operands originally have the same palette kind and value.
Terminal conversion uses the fixed Xterm-default palette described above.

`|` leaves both operands unchanged. `|=` replaces the left operand and returns
`color_t&`; the right operand is unchanged. Both operations are `noexcept`.
They do not average colors or perform alpha compositing.

```cpp
fmt::color_t accent = fmt::cube_color{1, 2, 3}; // RGB equivalent {95, 135, 175}.
accent |= fmt::gray_color{12}; // RGB {223, 135, 175}.
bool is_rgb = fmt::holds_alternative<fmt::rgb>(accent); // true
```

!!! note "Changed behavior"

    Overlapping colors involving a terminal color previously reported an error.
    They now combine after conversion to RGB.

Equality and ordering include both kind and payload: colors of different
kinds remain distinct even when their RGB equivalents are equal. Ordering
follows the unsigned packed value, in kind order `none`, `rgb`, `term`,
`cube`, then `gray`, and by payload within each kind. C++20
and later provide `std::strong_ordering` through `operator<=>`.
`std::hash<fmt::color_t>` permits use as an unordered-container key.

`color_t::value_type` and `underlying_type<color_t>::type` are `uint32_t`.
`fmt::to_underlying(color)` returns the packed representation used by
`text_style`, including the alternative's discriminator and payload. This
encoding and its hash values are not stable serialization formats.
