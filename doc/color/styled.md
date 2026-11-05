# Styled Arguments

## Structured styled text

Use `styled_text` to compose text with independently styled parts. Include
`<fmt/styled_text.h>` and format the result with `{}`.

### Concatenation preserves each part's style

Both `+` and `+=` concatenate complete styled spans. `+` returns a new value;
`+=` appends to the value on its left. Neither operation extends an existing
span's style over the text added after it, even if that span contains several
styled parts.

```cpp
auto red_text = fmt::overlay(fmt::fg(fmt::color::red),
    "red text with " + fmt::bold("nested bold") + " still red");
auto line = fmt::styled_text{"plain | "} + red_text + " | plain again";
fmt::println("{}", line);
```

Expected output (followed by a newline):

<pre class="color-output">plain | <span style="color: #FF0000">red text with <strong>nested bold</strong> still red</span> | plain again</pre>

Only the middle phrase is red, and only `nested bold` is also bold. The prefix
and suffix use terminal defaults. The same boundary is preserved when appending
in place:

```cpp
auto extended = red_text;
extended += " | plain again";
```

The appended text is outside the red span. A plain string has no style of its
own; it inherits a surrounding group style only when you apply one to the
combined text. Empty text adds nothing.

### Use `overlay` to style a group

Concatenate the parts first, then use `overlay(style, text)` to give the whole
group a surrounding style. Existing child styles are preserved. A child inherits
colors it does not specify, and emphasis such as bold and underline combines
with the group's emphasis. A child that specifies a foreground or background
uses that color instead of the corresponding group color.

```cpp
auto words = fmt::overlay(fmt::fg(fmt::color::red), "red")
    + " " + fmt::overlay(fmt::fg(fmt::color::green), "green")
    + " " + fmt::overlay(fmt::fg(fmt::color::blue), "blue");
auto group = fmt::overlay(fmt::bg(fmt::color::light_gray), words);
fmt::println("{}", group);
```

Expected output (followed by a newline):

<pre class="color-output"><span style="background-color: #D3D3D3"><span style="color: #FF0000">red</span> <span style="color: #008000">green</span> <span style="color: #0000FF">blue</span></span></pre>

Each word keeps its foreground color. The light-gray background covers all
three words and the spaces between them.

The free form `overlay(style, text)` returns a new value and leaves the input
unchanged. The member form updates the group in place:

```cpp
words.overlay(fmt::bg(fmt::color::light_gray));
```

Overlay adds the specified colors and emphasis to the group's own style;
it does not replace styles explicitly chosen for its parts. To include more
text in a group's style, concatenate before applying that style:

```cpp
auto all_red = fmt::overlay(fmt::fg(fmt::color::red),
    red_text + " and this is red too");
```

Appending to `all_red` later still adds a separate span. Apply `overlay` to the
new combined value if the surrounding style should include the appended text.

### Absolute spans ignore the surrounding style

Use `absolute(text)` to make a span independent of its enclosing style. It
preserves the span's colors, emphasis, and children, and marks only its root
absolute. Like `overlay`, it returns an owning value: named inputs are copied
and temporaries can be moved. Narrow and wide strings are also accepted.
For a style value, `absolute(style)` returns an absolute copy of that style.

An absolute style describes the complete appearance of its span. Missing
foreground and background colors mean terminal defaults, and absent emphasis
means off. Children inside an absolute group can inherit from that group as
usual. After the absolute span, the surrounding appearance resumes.

An absolute node emits an SGR reset before applying its style, unless it
produces no output. This applies in both compact and noncompact modes, even when
the recorded terminal state already has the same attributes. Relative descendants
inherit the resulting style without repeating the group's absolute reset. An
explicitly absolute descendant establishes its own reset boundary.

```cpp
auto underline_only = fmt::absolute(fmt::underline("underline only"));
auto sentence = fmt::styled_text{"before "} + underline_only + " after";
auto decorated = fmt::strike(
    fmt::overlay(fmt::bg(fmt::gray_color{12}), sentence));
fmt::println("{}", decorated);
```

Expected output (followed by a newline):

<pre class="color-output"><span style="background-color: #808080; text-decoration: line-through">before </span><span style="text-decoration: underline">underline only</span><span style="background-color: #808080; text-decoration: line-through"> after</span></pre>

`before ` and ` after` have a gray background and strikethrough. The absolute
span is underlined, without strikethrough or the gray background. The text after
it resumes both surrounding properties.

### Formatting and plain text

Formatting leaves the value unchanged. Relative roots inherit the enclosing
formatting context's colors and emphasis. After the complete replacement field,
the enclosing style is restored. Without an outer style, the context starts at
terminal defaults.

The formatter tracks the last style emitted to the terminal separately from
the enclosing group style. Relative groups emit no SGR on entry or exit.
Immediately before writing a leaf's text or padding, it emits the transition
from the recorded terminal state to the required style. Successive leaves
therefore transition directly without first restoring intermediate group
styles. At field completion, it restores the enclosing formatting context's
style from the last emitted state.

```cpp
fmt::print(fmt::underline(), "The result is {} after rounding.",
           fmt::overlay(fmt::fg(fmt::term_color::bright_red), "42"));
```

Expected output (without a trailing newline):

<pre class="color-output"><span style="text-decoration: underline">The result is <span style="color: #FF0000">42</span> after rounding.</span></pre>

The whole sentence is underlined, and `42` is also bright red. An absolute
root temporarily replaces the enclosing style, then restores it on completion.

Styled text accepts ordinary string fill, alignment (`<`, `>`, `^`), width,
and precision, with optional `s` presentation. Width and precision may also be
dynamic arguments. Debug presentation (`?`) is not supported.

```cpp
fmt::println("{:*>12.9}", fmt::bold("hello " + overlay(fg(blue), "world")));
// Three '*' characters followed by "hello wor", all bold; "wor" is blue.
```

Expected output (followed by a newline):

<pre class="color-output"><span style="font-weight: bold">***hello <span style="color: #0000FF">wor</span></span></pre>

Precision limits the whole tree rather than each leaf. Narrow text uses
{fmt}'s UTF-8 display-width estimate without splitting code points; wide text
uses code-unit counts. If the next code point does not fit, later text is not
printed. Width supplies padding after truncation. Padding receives the root
style, including for empty text or precision zero; child styles affect only
their own text. Relative transitions are emitted only before text or padding.
Absolute nodes reset before applying their styles, unless there is no output.
Truncation stops traversal of later text and nodes. It does not emit intermediate
group restorations; after any right padding, the field transitions directly from
its last emitted style to the enclosing context style.

Use `plain_text` when you need an independent string containing just the stored
text, without applying the styles:

```cpp
auto plain = line.plain_text();
// "plain | red text with nested bold still red | plain again"
```

Stored text is copied as supplied, including braces, embedded nulls, and raw
terminal escapes. Raw escapes are not interpreted or removed and can interfere
with the expected appearance. Express styling through the API when composing
styled text. Terminal support and themes can affect the displayed appearance.

For wide text, use `wstyled_text` and include `<fmt/xchar.h>` for wide formatting
entry points. The same composition and grouping rules apply.

### Measuring text width

`text.display_width()` sums the widths of the leaf strings using the same rules
as ordinary {fmt} string alignment. Narrow strings are decoded as UTF-8: for
example, `"caf\xc3\xa9"` has width 4 and `"\xe7\x95\x8c"` has width 2.
Wide strings use code-unit counts, matching this version's wide-string formatter.
Styles add no width, and empty text has width zero.

This is {fmt}'s width estimate, not full terminal or grapheme measurement.
Each leaf is measured independently; keep UTF-8 code points within a single
leaf. Raw escape sequences inside text are not recognized or excluded.
Each leaf must contain complete UTF-8 code points for width and precision to
match formatting the equivalent ordinary concatenated string.

## Parsing SGR text

Include `<fmt/parse_styled_text.h>` and call `parse_styled_text` to turn text
containing ordinary ANSI SGR into an owned `styled_text` (or `wstyled_text` for
wide input):

```cpp
auto message = fmt::parse_styled_text("plain \x1b[31mred\x1b[0m plain");
fmt::println("{}", message);
```

The parser follows the SGR state changes; it does not add an initial reset.
Before the first reset, parsed spans are relative and can inherit an enclosing
formatting style. A reset establishes terminal defaults for **all following
text**, not just the next span.

If a reset occurs before any text, the returned root is absolute. Otherwise,
earlier text remains relative and the suffix beginning at the reset forms an
absolute group. Its children have relative styles based on terminal defaults.
Later resets clear the current style within that same group; they do not create
additional nested groups. A single run can be represented directly by a leaf.

For example, a reset prevents an enclosing underline from returning on later
text, even when that text enables another attribute:

```cpp
auto parsed = fmt::parse_styled_text("\x1b[1mA\x1b[mB\x1b[1mC");
fmt::println(fmt::underline(), "{}", parsed);
```

Expected output (followed by a newline):

<pre class="color-output"><span style="font-weight: bold; text-decoration: underline">A</span>B<span style="font-weight: bold">C</span></pre>

`A` inherits underline and adds bold. The reset before `B` establishes defaults,
so `B` is plain and `C` adds only bold. Formatting still restores the enclosing
style after the complete parsed value.

As with other relative styles, an unset attribute before the first reset does
not cancel an inherited attribute; selective clearing of an unknown enclosing
state cannot be represented by these relative style values. Use an explicit
reset when the input must establish a complete state independent of its context.

Consecutive SGR sequences update one current style. No empty leaves are emitted;
adjacent text with equal styles is combined within each group. A trailing SGR
without text creates no span or empty group. The result represents styled text,
not the original escape bytes or a trailing command's effect on terminal state.
Input text is copied, including embedded nulls when supplied through a sized
string or string view.

Recognized codes cover the library's eight emphasis attributes and their removal,
normal/bright terminal colors, indexed colors, truecolor, and default-color
restoration. Reset (`0`, empty parameters, or leading-zero forms) clears the
current style; `00` and `000` have no push/pop meaning. Indexed and RGB colors
accept semicolon forms and colon forms, including an omitted/default RGB color
space field. Unicode code units are preserved without decoding.

Non-SGR escapes, including uppercase `M` commands and opaque OSC/DCS payloads,
remain verbatim text. Malformed SGR (including incomplete/out-of-range supported
colors or numeric overflow) is preserved in full and does not change style.
Unknown valid parameters and unsupported colon subparameter groups are ignored.
For an unknown semicolon extended-color mode, the remainder of that SGR is
ignored because its payload boundary is unknown. Preserved raw terminal commands
can affect the terminal outside the style model; parsing is not sanitization.

## Stripping ANSI sequences

Include `<fmt/strip_ansi.h>` and call `strip_ansi` to obtain an owning string
with complete ESC-prefixed ANSI sequences removed:

```cpp
auto plain = fmt::strip_ansi("\x1b[31mred\x1b[0m"); // "red"
```

The character-generic function accepts `basic_string_view<Char>`; convenience
overloads accept narrow/wide literals and standard strings. It recognizes CSI,
OSC, DCS, SOS, PM, APC, and ordinary ESC sequences by syntax. Private and unknown
commands are removed too, without validating parameter meanings. OSC ends at
BEL or `ESC \`; the other control strings end at `ESC \`. Their payloads and
terminators are removed with them. Hyperlink labels outside OSC markers remain.

Standalone C0 controls (such as tabs, newlines, and embedded nulls) and single-byte
C1 forms are preserved. Malformed or incomplete escape prefixes are preserved;
scanning resumes at an interrupting character so a subsequent independent escape
can still be removed. Unterminated control strings are preserved in full,
including any apparent escapes inside their payload. CAN and SUB cancel a
control string: its preceding bytes and the cancellation character are retained,
and scanning resumes at that character. Text code units are copied without
Unicode decoding.

This function does not simulate cursor movement, erase commands, carriage
returns, or other terminal behavior, and is not a security sanitizer. Unlike
`parse_styled_text(...).plain_text()`, it removes non-SGR escapes too, without
constructing a styled tree.

The styled-text overload returns a new `basic_styled_text<Char>` with the same
tree structure, styles, and absolute boundaries. It strips ANSI sequences from
each leaf independently, retaining leaves that become empty and sharing unchanged
leaf strings. The input is unchanged. Sequences split across leaves are preserved.

```cpp
auto text = fmt::bold("\x1b[31mred\x1b[0m");
auto clean = fmt::strip_ansi(text); // Bold "red", with raw escapes removed.
auto plain = clean.plain_text();   // "red", without the tree's styling.
```

Both narrow and wide styled text are supported. To strip sequences after joining
all leaves, call `strip_ansi(text.plain_text())` instead; this also removes any
complete sequences formed across leaf boundaries.

## Backslash escaping and unescaping

Include `<fmt/backslash.h>` for `backslash_escape` and `backslash_unescape`.
These independent text utilities return owning strings and support narrow and
wide text, including embedded NULs when passed a string or a length-bearing view.
They require neither ANSI parsing nor styled text.

```cpp
#include <fmt/backslash.h>

auto readable = fmt::backslash_escape("\x1b[31mred\x1b[0m\n");
// readable contains the literal text: \e[31mred\e[0m\n
auto original = fmt::backslash_unescape(readable);
```

The decoder accepts the following escape sequences. The last column describes
the canonical spelling produced by `backslash_escape` and `backslash_escape_to`.

| Escape | Character or value   | ASCII  | Emits |
| ------ | -------------------- | ------ | ----- |
| `\a`   | Alert (bell)         | `0x07` | Yes   |
| `\b`   | Backspace            | `0x08` | Yes   |
| `\e`   | Escape (ESC)         | `0x1b` | Yes   |
| `\f`   | Form feed            | `0x0c` | Yes   |
| `\n`   | Line feed (newline)  | `0x0a` | Yes   |
| `\r`   | Carriage return      | `0x0d` | Yes   |
| `\t`   | Horizontal tab       | `0x09` | Yes   |
| `\v`   | Vertical tab         | `0x0b` | Yes   |
| `\\`   | Backslash            | `0x5c` | Yes   |
| `\"`   | Double quote         | `0x22` | Yes   |
| `\'`   | Apostrophe           | `0x27` | No    |
| `\?`   | Question mark        | `0x3f` | No    |
| `\x00` | NUL                  | `0x00` | Yes   |
| `\x7f` | Delete (DEL)         | `0x7f` | Yes   |
| `\xhh` | Exactly 2 hex digits | `0xhh` | Yes   |
| `\nnn` | 1 to 3 octal digits  |        | No    |

`backslash_escape` uses `\a`, `\b`, `\e`, `\f`, `\n`, `\r`, `\t`, and `\v`
for the corresponding controls. `\e` represents ESC (`0x1b`); it is an
extension to the standard C++ escape spellings. It uses `\xhh` with two lowercase
hexadecimal digits for other ASCII C0 controls and DEL (`0x7f`), including `\x00` for NUL. Literal
backslashes become `\\` and double quotes become `\"`. Other code units,
including non-ASCII text and UTF-8 bytes, are preserved. No surrounding quotes
are added.

`backslash_unescape` recognizes those forms, plus `\'` for an apostrophe and
`\?` for a question mark. Ordinary apostrophes and question marks remain
unchanged when escaping. Hex digits may use either case. The decoder also
accepts one to three octal digits (`\0` through `\377`). Hex escapes consume
exactly two digits; octal escapes consume at most three. Subsequent digits
remain ordinary text.

Unknown or malformed escapes throw `fmt::format_error`. This includes `\q`,
`\x`, `\xG1`, a trailing backslash, and octal values above 255. Unicode and
braced numeric escapes are not supported and also throw. Decoding is one pass:
a decoded backslash does not start another escape. Surrounding quotes are
retained. If `backslash_unescape_to` throws, output written before the invalid
escape remains in the destination; the invalid escape itself writes nothing.

To write into an existing destination, use `backslash_escape_to(out, text)` or
`backslash_unescape_to(out, text)`. Both accept output iterators and return the
advanced iterator. They support narrow and wide string views, literals, and
strings without allocating an intermediate string.

```cpp
#include <iterator>
#include <string>

std::string log = "Received: ";
fmt::backslash_escape_to(std::back_inserter(log), "\x1b[31mred\n");
// log contains: Received: \e[31mred\n
```

The iterator functions support constant evaluation from C++14 when the output
iterator and input view support it. With this {fmt} implementation, use explicit
pointer-and-length views before C++20. The owning-string functions support
constant evaluation where {fmt} enables constexpr `std::basic_string` support
(C++20 and later with a supported compiler and standard library). Any dynamic
storage allocated during constant evaluation must be released within that
constant evaluation; these functions can produce temporary strings for a
compile-time check.

The output must not invalidate or overwrite unread input. In particular, do not
append to the string supplying the input view, since reallocation can invalidate
that view.

`backslash_unescape(backslash_escape(text))` recovers the original code units.
To inspect a styled value's text, pass `text.plain_text()`; to inspect its emitted
SGR sequences too, pass the formatted string. These functions do not interpret
ANSI sequences or remove explicit styles.

## Individual styled arguments

Style an individual replacement field with `styled`. Its format
specifications still apply to the underlying value.

::: styled(const T&, text_style)
    options:
      color_reference: true

Expected output for the example above (without a trailing newline):

<pre class="color-output">Elapsed time: <span style="color: #008000; background-color: #191970">1.23</span> seconds</pre>

```cpp
fmt::println("Amount: {:>8.2f}",
             fmt::styled(1.23, fmt::fg(fmt::color::green)));
```

Expected output (followed by a newline):

<pre class="color-output">Amount: <span style="color: #008000">    1.23</span></pre>

The width and precision apply to the number, including padding inside the styled
span. Relative styles inherit the enclosing context; absolute styles replace
it for this value and always emit an explicit reset in both SGR modes, even
when the enclosing state already has the same attributes. Both restore the
enclosing style afterward. For examples, see
[Text style rendering](index.md#TextStyleRendering).

To insert a styled value directly into an ostream, include
`<fmt/color-ostream.h>` and use `os << fmt::styled(value, style)`.
[Styled stream insertion](print.md#styled-stream-insertion) describes stream
formatting, wide streams, and the wrapper's reference lifetime.

```cpp
#include <numbers>  // C++20

auto style = fmt::fg(fmt::color::green).bold();
std::cout << "Pi: " << std::fixed << std::setprecision(2)
          << fmt::styled(std::numbers::pi, style) << '\n';
```

Expected output (followed by a newline):

<pre class="color-output">Pi: <span style="color: #008000; font-weight: bold">3.14</span></pre>

Only `3.14` is styled in bold green. The value is rounded to two decimal places
using the ostream's formatting settings.
