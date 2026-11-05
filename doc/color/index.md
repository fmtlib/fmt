# Terminal Colors and Text Styles

Include `<fmt/color.h>` to combine foreground and background colors with
emphasis attributes, style individual formatting arguments, and produce
formatted strings or output.

The color and styling API supports C++11 and later. `fmt::color_t` stores an
unspecified color, an RGB value, or a terminal, cube, or gray palette color
in one public type.

Convenience is a major design goal. The API deliberately provides several ways
to express common styling operations: constructors for complete styles, fluent
member functions for updating values, and free functions for creating or deriving
styles. These overlapping forms let you choose the clearest expression for each
use, from concise one-off styles to attributes controlled by user configuration.

```cpp
#include <fmt/color.h>

int main() {
  auto style = fmt::fg(fmt::color::forest_green).bold();
  fmt::println(style, "The result is {}.", 42);
  fmt::println("The result is {}.",
               fmt::styled(42, fmt::fg(fmt::term_color::red)));
}
```

<pre class="color-output"><span style="color: #228B22; font-weight: bold">The result is 42.</span>
The result is <span style="color: #CD0000">42</span>.</pre>

Illustrative output using Xterm's default red (`#CD0000`) for the terminal color.

The first call styles the entire message. The second styles only `42`.

## Reference

| Page                           | Contents                                                                                                                           |
| ------------------------------ | ---------------------------------------------------------------------------------------------------------------------------------- |
| [Colors](colors.md)            | RGB, terminal, cube, and gray colors; `no_color`; public `color_t` values, access and visitation, and integer representations      |
| [Emphasis](emphasis.md)        | Text attributes and their combination                                                                                              |
| [Text styles](text-style.md)   | `text_style`, its members and operators, and `fg`/`bg`                                                                             |
| [Styled arguments](styled.md)  | Structured styled text, SGR parsing, ANSI stripping, backslash escaping/unescaping, `styled`, format specifications, and lifetimes |
| [Styled output](print.md)      | `print`, `println`, `vprint`, and `vprintln`                                                                                       |
| [Styled formatting](format.md) | `format`, `format_to`, and their `v` variants                                                                                      |

## Choosing and combining styles

Use constants in the [`color`](colors.md#named-rgb-colors) namespace for named RGB values, [`rgb`](colors.md#rgb-values) for explicit
RGB components, and [`term_color`](colors.md#terminal-palette-colors) for the terminal's 16 named palette colors.
Use [`cube_color`](colors.md#cube-palette-colors) for its 6×6×6 cube and [`gray_color`](colors.md#gray-palette-colors) for its 24 gray levels.
Pass a color to [`fg`](text-style.md#foreground-and-background) or [`bg`](text-style.md#foreground-and-background) to create a foreground or background style.

```cpp
auto style = fmt::text_style{fmt::rgb{30, 144, 255},
                             fmt::term_color::black,
                             fmt::emphasis::bold |
                             fmt::emphasis::underline};
```

A default-constructed [`text_style`](text-style.md#text_style) has no colors or emphasis. Combine styles
with [`blend`](text-style.md#blending-styles). Foreground and background are
handled independently, unspecified colors are ignored, and emphasis bits are
ORed. A channel specified by only one input preserves its color and kind.
When multiple inputs specify the same channel, their colors convert to RGB
and combine by averaging the color channels; that channel then holds RGB.

[`operator|`](text-style.md#combining-styles) and
[`operator|=`](text-style.md#combining-styles) offer a cheaper
bitwise operation, but use care when combining colors: they OR the channel
bits instead of averaging them. OR cannot lower a channel value, so combining
a bright color with a dim color will not dim the bright color. Use constructors
or fluent setters to select colors directly, and `blend` to average them.

The examples below compare both operations on foreground styles. Every input
name is a `fmt::term_color` value. Conversion uses the fixed Xterm-default RGB
mapping, which may differ from the user's terminal theme. **Both operations
produce RGB foregrounds**, rather than terminal palette colors.

```cpp
auto red  = fmt::text_style{fmt::term_color::bright_red};
auto blue = fmt::text_style{fmt::term_color::blue};
auto averaged = fmt::blend(red, blue); // RGB {127, 0, 119}.
auto ored = red | blue;                // RGB {255, 0, 238}.
```

<table class="color-comparison">
<thead><tr><th>Inputs (<a href="colors.html#terminal-palette-colors"><code>term_color</code></a>)</th><th><a href="text-style.html#blending-styles"><code>blend()</code></a> — average</th><th><a href="text-style.html#combining-styles"><code>operator|</code></a> — bitwise OR</th></tr></thead>
<tbody>
<tr>
<td><div class="color-inputs"><span class="color-chip" style="background-color:#FF0000;color:#000000"><a href="colors.html#terminal-palette-colors"><code>bright_red</code></a><small>255, 0, 0</small></span><span class="color-chip" style="background-color:#0000EE;color:#FFFFFF"><a href="colors.html#terminal-palette-colors"><code>blue</code></a><small>0, 0, 238</small></span></div></td>
<td><span class="color-chip" style="background-color:#7F0077;color:#FFFFFF"><code>rgb{127, 0, 119}</code><small>#7F0077</small></span></td>
<td><span class="color-chip" style="background-color:#FF00EE;color:#000000"><code>rgb{255, 0, 238}</code><small>#FF00EE</small></span></td>
</tr>
<tr>
<td><div class="color-inputs"><span class="color-chip" style="background-color:#FF0000;color:#000000"><a href="colors.html#terminal-palette-colors"><code>bright_red</code></a><small>255, 0, 0</small></span><span class="color-chip" style="background-color:#5C5CFF;color:#FFFFFF"><a href="colors.html#terminal-palette-colors"><code>bright_blue</code></a><small>92, 92, 255</small></span></div></td>
<td><span class="color-chip" style="background-color:#AD2E7F;color:#FFFFFF"><code>rgb{173, 46, 127}</code><small>#AD2E7F</small></span></td>
<td><span class="color-chip" style="background-color:#FF5CFF;color:#000000"><code>rgb{255, 92, 255}</code><small>#FF5CFF</small></span></td>
</tr>
<tr>
<td><div class="color-inputs"><span class="color-chip" style="background-color:#CD0000;color:#FFFFFF"><code>red</code><small>205, 0, 0</small></span><span class="color-chip" style="background-color:#0000EE;color:#FFFFFF"><a href="colors.html#terminal-palette-colors"><code>blue</code></a><small>0, 0, 238</small></span></div></td>
<td><span class="color-chip" style="background-color:#660077;color:#FFFFFF"><code>rgb{102, 0, 119}</code><small>#660077</small></span></td>
<td><span class="color-chip" style="background-color:#CD00EE;color:#000000"><code>rgb{205, 0, 238}</code><small>#CD00EE</small></span></td>
</tr>
<tr>
<td><div class="color-inputs"><span class="color-chip" style="background-color:#CD0000;color:#FFFFFF"><code>red</code><small>205, 0, 0</small></span><span class="color-chip" style="background-color:#5C5CFF;color:#FFFFFF"><a href="colors.html#terminal-palette-colors"><code>bright_blue</code></a><small>92, 92, 255</small></span></div></td>
<td><span class="color-chip" style="background-color:#942E7F;color:#FFFFFF"><code>rgb{148, 46, 127}</code><small>#942E7F</small></span></td>
<td><span class="color-chip" style="background-color:#DD5CFF;color:#000000"><code>rgb{221, 92, 255}</code><small>#DD5CFF</small></span></td>
</tr>
</tbody>
</table>

## Text style rendering

<a id="TextStyleRendering"></a>

If a style is not empty, the formatted text is preceded by [SGR] (Select Graphic
Rendition) sequences for its specified foreground, background, and emphasis,
and followed by an SGR reset to terminal defaults. An empty style adds no
sequences of its own. `println` and `vprintln` append their newline after
the reset.

The formatting context carries the enclosing style through replacement fields.
Relative styled arguments and structured text inherit missing colors and combine
emphasis with that style. Each field restores the enclosing style afterward:

```cpp
fmt::print(fmt::emphasis::underline,
           "The result is {} after rounding.",
           fmt::styled(42, fmt::fg(fmt::term_color::bright_red)));
```

<pre class="color-output"><span style="text-decoration: underline">The result is <span style="color: #FF0000">42</span> after rounding.</span></pre>

Illustrative output using our fixed terminal-to-RGB palette: bright red is
`rgb{255, 0, 0}` (`#FF0000`). The terminal theme may display it differently.

| Text               | Appearance                |
| ------------------ | ------------------------- |
| `The result is `   | Underlined                |
| `42`               | Bright red and underlined |
| ` after rounding.` | Underlined                |

After `42`, the foreground returns to its enclosing value and underline remains
active. The outer call resets to terminal defaults at the end. An absolute
replacement field temporarily uses its own complete style, then also restores
the enclosing style.

The same rules apply to [structured styled text](styled.md#structured-styled-text):
concatenate complete spans with `+` or `+=`, and use `overlay` to style a group.

Style state belongs to one formatting operation. A separate `format` or
`format_to` call starts with its own context, even when given the same output
iterator. Custom formatters that directly delegate to another formatter using
their existing context preserve its style. Raw SGR embedded in strings and
terminal styling established outside the operation are not tracked.

The [output functions](print.md) emit escape sequences even when output is
redirected to a file. The [formatting functions](format.md) store those sequences
in their result. For format string syntax, see the [syntax reference](../syntax.md).

[SGR]: https://en.wikipedia.org/wiki/ANSI_escape_code#Select_Graphic_Rendition_parameters
