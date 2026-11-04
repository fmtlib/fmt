# Text Styles

## `text_style`

::: text_style
    options:
      color_reference: true
      exclude_members:
        - get_foreground
        - get_background
        - get_emphasis
        - has_foreground
        - has_background
        - has_emphasis
        - strikethrough
        - is_strikethrough

### Construction

Construct a style with colors first or emphasis first. Each color may be a
[`color_t`](colors.md#color-variants), or a named RGB constant, an `rgb`, a
`term_color`, `cube_color`, `gray_color`, or `no_color` implicitly converted
to `color_t`.

The color-first constructor requires a foreground color; background and
emphasis default to unspecified and empty. The emphasis-first constructor
accepts optional foreground and background colors, both defaulting to
`no_color`. Its emphasis defaults to `emphasis{}`, so `text_style{}` creates
an empty style. An emphasis value alone creates an emphasis-only style.

```cpp
auto foreground = fmt::text_style{fmt::color::red};
auto style = fmt::text_style{fmt::color::red, fmt::term_color::black,
                             fmt::emphasis::bold};
auto background = fmt::text_style{fmt::no_color, fmt::color::blue};
auto attributes_first = fmt::text_style{fmt::emphasis::bold, fmt::color::red,
                                        fmt::term_color::black};
auto underlined_background = fmt::text_style{
    fmt::emphasis::underline, fmt::no_color, fmt::color::blue};
auto palette = fmt::text_style{fmt::cube_color{1, 2, 3}, fmt::gray_color{12}};
```

### Absolute styles

Styles are relative by default: unspecified colors and unset emphasis leave
the terminal's existing attributes unchanged when emitted. Call `absolute()`
to reset the terminal before applying a style. Unspecified colors then select
terminal defaults, and unset emphasis means normal text.

```cpp
auto reset = fmt::text_style{}.absolute();
auto underlined = fmt::underline().absolute();
auto red = fmt::fg(fmt::color::red).absolute();
red.absolute(false); // Return to relative styling, retaining the foreground.
```

The free function `absolute(style)` returns an absolute copy, preserving both
colors and emphasis and leaving the input unchanged. It is `noexcept` and
supports constant evaluation under the same conditions as the member setter.
For example, `auto red = fmt::absolute(fmt::fg(fmt::color::red));`.

The member `absolute(bool enable = true)` returns `text_style&`; `is_absolute()` queries
the flag. The setter is `noexcept` and supports constant evaluation in C++14
and later on supported compilers; the query is `constexpr` in C++11 and later.
The flag uses bit 62 without increasing the style's size. Absolute styles can
still be explicitly modified using setters, overlays, and composition.

An absolute-only style is nonempty and emits `"\x1b[0m"`. Compact SGR output
combines the reset and attributes, such as `"\x1b[0;4m"`; noncompact output
emits the reset separately before the attribute sequences. Neither form
depends on a previously active terminal style.

### Overlaying styles

`overlay(other)` updates a style with another style's specified colors and
returns `text_style&` for chaining. The incoming foreground and background
replace their matching channels independently, preserving each color's kind.
An incoming `no_color` leaves the existing channel unchanged; a specified black
color replaces it. Emphasis bits combine by OR, so an overlay can add attributes
but cannot remove them. An empty overlay leaves the style unchanged.

The result is absolute if either input is absolute. This preserves the reset
requirement while allowing explicit changes to an absolute style's attributes.

```cpp
auto style = fmt::text_style{fmt::term_color::red, fmt::gray_color{12},
                             fmt::emphasis::bold};
const auto other = fmt::text_style{fmt::cube_color{0}, fmt::no_color,
                                   fmt::emphasis::underline};
style.overlay(other);
// Foreground: cube_color{0}; background: gray_color{12}.
// Emphasis: bold and underline. The value of other is unchanged.
```

The operation is `noexcept` and supports constant evaluation in C++14 and later
on supported compilers. It changes the style value without emitting ANSI output.
Use component setters to clear a color or remove attributes.

The free function `fmt::overlay(base, overrides)` returns a new `text_style`
with the same precedence rules, leaving both inputs unchanged. The second
argument supplies the overriding colors; reversing the arguments can change
the result. Like the member, it is `noexcept` and supports constant evaluation
in C++14 and later on supported compilers, with runtime use available in C++11.

```cpp
const auto base = fmt::text_style{fmt::term_color::red, fmt::gray_color{12},
                                  fmt::emphasis::bold};
const auto overrides = fmt::text_style{fmt::cube_color{0}, fmt::no_color,
                                       fmt::emphasis::underline};
auto combined = fmt::overlay(base, overrides);
// combined has cube foreground, gray background, bold, and underline.
// base and overrides retain their original values.
```

::: overlay(text_style, text_style)
    options:
      color_reference: true

### Combining styles

`|` returns a combined style and leaves both operands unchanged. `|=` updates
the left operand and returns `text_style&`. Both operations are `noexcept`.
The result is absolute if either operand is absolute.
Emphasis bits combine by OR; foreground and background combine independently:

- If a color channel is unspecified in one operand, it takes the other
  operand's color and preserves that color's kind.
- If both operands specify the same channel, both colors convert to RGB and
  their channel bits combine by OR. That channel then holds `rgb`, including
  when both original colors have the same palette kind and value.

```cpp
auto style = fmt::fg(fmt::cube_color{1, 2, 3}) | fmt::bg(fmt::gray_color{12});
// Foreground remains cube_color; background remains gray_color.
style |= fmt::fg(fmt::gray_color{12});
// Foreground is now RGB {223, 135, 175}; background is unchanged.
```

Conversion uses fixed RGB values for palette colors; terminal colors use
the Xterm-default mapping, which may differ from the terminal's current theme.
Composition does not average colors or perform alpha compositing. See
[color composition](colors.md#composition-comparisons-and-numeric-representation).
Use a component setter to replace a color rather than combine its bits.

### Blending styles

::: blend(First, Second, Styles...)
    options:
      color_reference: true

Pass two or more `text_style` values to `fmt::blend` to return a new style.
Foreground colors are averaged independently of background colors, using
[color blending](colors.md#blending-color-variants). Unspecified colors are
ignored separately in each channel. Emphasis bits from every input are ORed.

Each channel remains unspecified if no input supplies a color, preserves its
color kind if exactly one input supplies it, and holds an RGB average if two
or more inputs supply colors. Palette colors use their fixed RGB equivalents.
All specified colors in a channel have equal weight; division occurs after
summation, with fractional results rounded down.

```cpp
constexpr auto first = fmt::text_style{fmt::term_color::red, fmt::no_color,
                                       fmt::emphasis::bold};
constexpr auto second = fmt::text_style{fmt::gray_color{12},
                                        fmt::cube_color{1, 2, 3},
                                        fmt::emphasis::underline};
constexpr auto third = fmt::text_style{fmt::rgb{}};
constexpr auto mixed = fmt::blend(first, second, third);
// Foreground: rgb{111, 42, 42}; background: cube_color{1, 2, 3}.
// Emphasis: bold and underline.
```

The function leaves its inputs unchanged, is `noexcept`, and supports constant
evaluation in C++11 and later. The result is always relative. Blending only
empty styles returns an empty style. All arguments must be styles; use a
`text_style` constructor to combine colors and emphasis before blending.

Each absolute input must specify both foreground and background colors: a
terminal-default color has an unknown RGB value. `FMT_ASSERT` checks this
precondition for every input. With assertions disabled, unspecified channels
contribute nothing and the result still discards the absolute flag.

### Empty styles and clearing

`empty` returns `true` when both colors are unspecified, no emphasis is set,
and absolute is disabled. A specified black color or an absolute-only reset
makes a style nonempty.
The query is `const`, `noexcept`, and `constexpr` in C++11 and later.

`clear` removes both colors, all emphasis, and the absolute flag, restoring the
default-constructed state. It is `noexcept` and returns `text_style&` for
chaining. It supports constant evaluation in C++14 and later on supported
compilers. Clearing a style changes the value; it does not emit an ANSI reset.

```cpp
auto style = fmt::text_style{fmt::color::red, fmt::term_color::black,
                             fmt::emphasis::bold};
assert(!style.empty());
style.clear();
assert(style.empty());
style.clear().fg(fmt::gray_color{12}).underline();
```

### Component accessors

Each component has a getter, a replacement setter, and a presence query:

| Component    | Getter     | Setter               | Presence query   |
| ------------ | ---------- | -------------------- | ---------------- |
| Foreground   | `fg()`     | `fg(color)`          | `has_fg()`       |
| Background   | `bg()`     | `bg(color)`          | `has_bg()`       |
| Emphasis     | `emph()`   | `emph(attributes)`   | `has_emph()`     |

All these members are `noexcept`. Getters are `const` and return values.
The color getters return the public type [`fmt::color_t`](colors.md#color-variants),
preserving the RGB, terminal, cube, gray, or unspecified alternative. The result
can be passed to either color setter, either style constructor, or the free
`fmt::fg` and `fmt::bg` functions. Use `fmt::holds_alternative`, `fmt::get`,
`fmt::get_if`, or `fmt::visit` to inspect its alternative. The emphasis getter
returns `emphasis`.

Getters accept empty components: color getters return an unspecified color,
and `emph()` returns `emphasis{}`. Presence checks are optional.

The `has_fg`, `has_bg`, and `has_emph` queries return `bool`, preserve
the style, and are `const`, `constexpr`, and `noexcept`. An empty style
returns `false` for all three. RGB black and terminal black are specified
colors, so their channel queries return `true`. `has_emph` returns `true`
when any emphasis bit is set, including faint or strike alone.

Each setter replaces its entire component, preserves the other two, and
returns `text_style&` for chaining. Color setters accept a named RGB constant,
an `rgb`, a `term_color`, a `cube_color`, a `gray_color`, or `no_color`.
Passing `no_color` clears that channel to unspecified; it does not store a
terminal-reset instruction.
Passing `emphasis{}` clears all attributes.

```cpp
auto style = fmt::text_style{fmt::color::coral};
style.bg(fmt::term_color::black).emph(fmt::emphasis::bold);
style.fg(fmt::no_color);  // Background and bold remain.
// has_fg() is false; has_bg() and has_emph() are true.

const auto source = style;
auto copy = fmt::text_style{source.fg(), source.bg(), source.emph()};

auto foreground = source.fg();
auto has_foreground = source.has_fg(); // false
auto unspecified = fmt::holds_alternative<fmt::no_color_t>(foreground); // true
```

!!! note "Backward compatibility"

    The original `get_foreground`, `get_background`, and `get_emphasis`
    names are retained as aliases for `fg`, `bg`, and `emph`, respectively.
    Likewise, `has_foreground`, `has_background`, and
    `has_emphasis` are retained as aliases for `has_fg`, `has_bg`, and
    `has_emph`, respectively.

!!! note "Changed behavior"

    Previously, the foreground, background, and emphasis getters required their
    components to be present. They can now be called when those components are
    absent.

!!! note "Changed behavior"

    Foreground and background values now have the public type `fmt::color_t`.
    Their type was previously unspecified and named `detail::color_type`
    internally. Both the short getters and the original names return
    `color_t` by value.

### Attribute queries

```cpp
bool is_bold()      const noexcept;
bool is_faint()     const noexcept;
bool is_italic()    const noexcept;
bool is_underline() const noexcept;
bool is_blink()     const noexcept;
bool is_reverse()   const noexcept;
bool is_conceal()   const noexcept;
bool is_strike()    const noexcept;
```

Each query returns whether its attribute is enabled. Unset attributes return
`false`, including every attribute of an empty style. Queries preserve the
style. These queries and the component getters support constant evaluation
in C++11 and later.

### Attribute setters

```cpp
text_style& bold(bool enable = true) noexcept;
text_style& faint(bool enable = true) noexcept;
text_style& italic(bool enable = true) noexcept;
text_style& underline(bool enable = true) noexcept;
text_style& blink(bool enable = true) noexcept;
text_style& reverse(bool enable = true) noexcept;
text_style& conceal(bool enable = true) noexcept;
text_style& strike(bool enable = true) noexcept;
```

Each setter enables its attribute by default, or disables it when passed
`false`. It changes only that bit, preserves all other attributes and colors,
and returns `text_style&` for chaining. Bold and faint are independent:
enabling or disabling either leaves the other unchanged. Rendering emits both
when both are enabled; their combined appearance depends on the terminal.
Attribute and component setters support constant evaluation in C++14 and
later on supported compilers.

The aliases `strikethrough` and `is_strikethrough` forward to `strike`
and `is_strike`, respectively. Either setter changes the same attribute bit,
and either query reports its state. `strikethrough(bool enable = true)` returns
`text_style&` for chaining.

Use `emph(attributes)` to set exactly a group of attributes, then individual
setters to change selected bits. In C++20 and later, `using enum` makes the
attribute names available without repeating their type:

```cpp
using enum fmt::emphasis;

auto style = fmt::text_style{fmt::color::coral};
style.emph(bold | blink | strike);  // Exactly these three attributes.
style.blink(false).underline();     // Bold, underline, strike.
```

In older language modes, qualify names with `fmt::emphasis::` or use a type
alias such as `using em = fmt::emphasis;` and write `em::bold`.

### Attribute functions returning a new style

The free functions `bold`, `faint`, `italic`, `underline`, `blink`,
`reverse`, `conceal`, and `strike` return `text_style` values. Each has a
Boolean factory overload and an overload that derives a copy from a base style.
All are `noexcept`.

#### Creating an attribute-only style

Call a function without arguments, or with `true`, to create a style containing
only that emphasis attribute. Both color channels are unspecified. Passing
`false` returns an empty style with no colors or attributes. The Boolean
parameter defaults to `true`; these factories support constant evaluation in
C++11 and later.

```cpp
constexpr auto heading = fmt::bold();
constexpr auto plain   = fmt::bold(false); // Empty style.
bool use_bold = true;                      // For example, a user preference.
fmt::println("{}", fmt::styled(42, fmt::bold(use_bold)));
fmt::println("{}", fmt::styled("Heading", fmt::bold()));
```

::: bold(bool)
    options:
      color_reference: true

::: faint(bool)
    options:
      color_reference: true

::: italic(bool)
    options:
      color_reference: true

::: underline(bool)
    options:
      color_reference: true

::: blink(bool)
    options:
      color_reference: true

::: reverse(bool)
    options:
      color_reference: true

::: conceal(bool)
    options:
      color_reference: true

::: strike(bool)
    options:
      color_reference: true

#### Deriving a style from a base

Pass a style as the first argument to return a modified copy. The original
style is unchanged. Each function enables its attribute by default; pass
`false` as the second argument to disable it. All other attributes and both
color channels retain their values and kinds.

```cpp
const auto theme = fmt::text_style{fmt::color::coral, fmt::term_color::black};
auto underlined = fmt::underline(theme);
auto bold_underlined = fmt::bold(underlined);
auto bold_only = fmt::underline(bold_underlined, false);
// theme still has no emphasis attributes.
```

Use these overloads when deriving a style from a reusable base. The member
call `style.underline()` modifies `style`; `fmt::underline(style)` returns a
new value. The style-copy overloads support constant evaluation in C++14 and
later on supported compilers, with runtime use available in C++11.

::: bold(text_style, bool)
    options:
      color_reference: true

::: faint(text_style, bool)
    options:
      color_reference: true

::: italic(text_style, bool)
    options:
      color_reference: true

::: underline(text_style, bool)
    options:
      color_reference: true

::: blink(text_style, bool)
    options:
      color_reference: true

::: reverse(text_style, bool)
    options:
      color_reference: true

::: conceal(text_style, bool)
    options:
      color_reference: true

::: strike(text_style, bool)
    options:
      color_reference: true

### Comparisons

`text_style` values support equality and ordering by their packed `uint64_t`
representations, including color kinds, color values, and emphasis bits.
An empty style sorts before any nonempty style. Different color kinds
remain distinct even when their RGB equivalents are equal. Styles can be used
as keys in `std::map` and `std::set`. In C++20 and later, `operator<=>`
returns `std::strong_ordering`.

`std::hash<fmt::text_style>` hashes the packed `uint64_t` representation,
allowing styles to be keys in `std::unordered_map` and `std::unordered_set`.
Equal styles have equal hashes. Hash values are not a stable serialization
format.

### Integer representation

`fmt::underlying_type<fmt::text_style>::type` is `uint64_t`.
`fmt::to_underlying(style)` returns the packed foreground, background, and
emphasis representation.  An empty style produces zero.

```cpp
constexpr auto empty = fmt::to_underlying(fmt::text_style{});  // 0
auto style  = fmt::fg(fmt::color::red) | fmt::emphasis::bold;
auto packed = fmt::to_underlying(style);  // uint64_t
```

The packed encoding is not a stable serialization format.

## Combining emphasis attributes

`fmt::emphasis` values are independent bit masks. `operator|` returns their
union without modifying either operand. `operator|=` adds the right operand's
attributes to the left operand and returns `emphasis&` referring to that left
operand, allowing chaining. Existing attributes remain set; combining with
`emphasis{}` leaves the value unchanged.

```cpp
auto attributes = fmt::emphasis::bold;
attributes |= fmt::emphasis::underline;
(attributes |= fmt::emphasis::italic) |= fmt::emphasis::strike;
auto style = fmt::text_style{attributes};
```

Both operators are `noexcept`. `operator|` supports constant evaluation in
C++11 and later; the mutating `operator|=` supports it in C++14 and later on
supported compilers, with runtime use available in C++11.

## Foreground and background

`fg` and `bg` provide a convenient way to create a `text_style` without
calling a constructor directly. When passed a color value from namespace `fmt`,
argument-dependent lookup (ADL) finds these functions, so you can omit the
`fmt::` prefix: for example, `fg(fmt::color::red)` instead of
`fmt::text_style{fmt::color::red}`.

::: fg(color_t)
    options:
      color_reference: true

::: bg(color_t)
    options:
      color_reference: true

Pass `fmt::no_color` to either function to produce an empty style. This
specifies no color; combining it with an existing style does not clear that
style's color or emit a terminal reset.

See [Colors](colors.md) for the accepted color types and
[Text style rendering](index.md#TextStyleRendering) for formatting behavior.
