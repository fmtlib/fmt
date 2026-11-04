# Emphasis

Use emphasis values as styles or combine them with colors through
[`text_style`](text-style.md).

## Attributes

`fmt::emphasis::strike` selects crossed-out text (SGR 9).

::: emphasis
    options:
      color_reference: true
      enum_value_placeholder: unspecified value

!!! note "Backward compatibility"

    `fmt::emphasis::strikethrough` remains an alias for `fmt::emphasis::strike`.
    Both names select the same attribute bit and produce the same SGR sequence.

## Combining attributes

::: operator|(emphasis, emphasis)
    options:
      color_reference: true

The result is an `emphasis` value containing both sets of attribute bits.
An `emphasis` value converts implicitly to `text_style`.

```cpp
auto style = fmt::emphasis::bold
           | fmt::emphasis::underline;
fmt::println(style, "Important: {}", 42);
```

!!! note "Changed behavior"

    Combining two emphasis values now returns `emphasis`.
    Previously, it returned `text_style`.

```cpp
fmt::text_style style = fmt::emphasis::bold | fmt::emphasis::underline;
if (style.has_emph())
  fmt::println(style, "Important");
```

Bold and faint remain independent bits: combining them preserves both and
emits both SGR attributes. Their combined appearance depends on the terminal.

## Integer representation

The underlying representation of `fmt::emphasis` is `uint8_t`.

```cpp
using emphasis_t = fmt::underlying_type<fmt::emphasis>::type;
constexpr auto bold_bit
                   = fmt::to_underlying(fmt::emphasis::bold); // 1
constexpr auto attributes = fmt::emphasis::bold
                          | fmt::emphasis::faint;
constexpr auto bits = fmt::to_underlying(attributes);         // 3
static_assert(std::is_same_v<const emphasis_t, decltype(bits)>);
static_assert(bold_bit == 1);
static_assert(bits     == 3);
```
