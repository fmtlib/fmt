# Named Colors

These constexpr `fmt::rgb` constants belong to the `fmt::color` namespace.
Use them directly, for example `fmt::fg(fmt::color::coral)`.
The `term_*` names are fixed Xterm-default RGB counterparts of `term_color`
values, including `term_bright_*` for bright colors. They emit truecolor;
they do not follow changes to the terminal's palette.
See [Colors](colors.md) for the color types and [the overview](index.md) for
styling examples.

Named RGB colors are grouped by hue, with equal red, green, and blue components
in the neutral group. Within each group, colors are ordered by hue and brightness.
The terminal RGB constants and gray palette levels have separate sections below.
Each name uses black or white text, whichever gives better contrast.
Hover over a swatch to see its RGB hex value.

## Red

::: color
    options:
      color_reference: true
      color_swatch_group: red

## Green

::: color
    options:
      color_reference: true
      color_swatch_group: green

## Blue

::: color
    options:
      color_reference: true
      color_swatch_group: blue

## Yellow

::: color
    options:
      color_reference: true
      color_swatch_group: yellow

## Cyan

::: color
    options:
      color_reference: true
      color_swatch_group: cyan

## Magenta

::: color
    options:
      color_reference: true
      color_swatch_group: magenta

## Neutral

::: color
    options:
      color_reference: true
      color_swatch_group: neutral

## Terminal colors

These `fmt::color::term_*` constants use fixed Xterm-default RGB values.
They are shown in two responsive grids: the eight normal colors, followed
by the eight bright colors beneath them. For example, `fmt::color::term_red`
is RGB `{205, 0, 0}`. Using `fmt::term_color::red` instead selects the
terminal's palette entry, whose appearance depends on its theme.

### Normal terminal colors

::: color
    options:
      color_reference: true
      color_swatch_group: terminal-normal

### Bright terminal colors

::: color
    options:
      color_reference: true
      color_swatch_group: terminal-bright

## Gray palette colors

Use `fmt::gray_color{index}` to select one of these 24 gray levels, shown
from darkest to lightest. The labels use zero-based indices 0–23;
terminal palette entries are 232–255. Each swatch shows the fixed RGB
equivalent, with every channel equal to `8 + 10 * index`.

```cpp
auto style = fmt::fg(fmt::gray_color{12}); // RGB equivalent {128, 128, 128}.
```

::: gray_color
    options:
      color_reference: true
      color_swatch_group: gray

## Cube palette colors

The table shows all 216 cube colors. Red indices select rows; green indices
select columns. Within each cell, six three-character fields show blue indices
` 0  1  2  3  4  5 ` in a fixed-width font. Each field's background is its RGB
equivalent, with contrasting text. Hover over a field for the full
`cube_color{r, g, b}`, RGB value, and terminal palette index.

The table scrolls horizontally on narrow screens. See
[Cube palette colors](colors.md#cube-palette-colors) for construction and
conversion details.

::: cube_color
    options:
      color_reference: true
      color_swatch_group: cube
