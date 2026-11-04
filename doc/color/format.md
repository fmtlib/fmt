# Styled Formatting

The formatting overloads produce text containing styling sequences.
See [Text style rendering](index.md#TextStyleRendering) for the shared rules.

## `to_sgr`

::: to_sgr
    options:
      color_reference: true

`fmt::to_sgr<Char>(style)` renders a style as a `std::basic_string<Char>`
containing ANSI SGR escape sequences. `Char` defaults to `char`, so
`fmt::to_sgr(style)` returns `std::string`; `fmt::to_sgr<wchar_t>(style)`
returns `std::wstring`. It leaves the style unchanged and includes no text or
trailing reset. An empty relative style produces an empty string. Unspecified
color channels emit no parameters. A relative style leaves those colors
unchanged; an absolute style begins with a reset, selecting terminal defaults
for them. An absolute-only style emits `"\x1b[0m"`.

```cpp
const auto style = fmt::text_style{fmt::emphasis::bold,
                                   fmt::rgb{1, 2, 3},
                                   fmt::term_color::black};
auto sgr      = fmt::to_sgr(style);
auto wide_sgr = fmt::to_sgr<wchar_t>(style);
```

The output depends on the `FMT_COLOR_COMPACT_SGR` compiler definition:

| Definition | Output for the example                  |
| ---------- | --------------------------------------- |
| `0`        | `\x1b[1m\x1b[38;2;001;002;003m\x1b[40m` |
| `1`        | `\x1b[1;38;2;1;2;3;40m`                 |

`0` is the default.

Here `\x1b` denotes the actual ESC character. Set the definition consistently
when compiling the application and its libraries. Both modes support C++11
and later. The function can allocate and is not `noexcept`.

Use `to_sgr` when managing terminal state yourself. Use the styled formatting
or printing functions to wrap text in styling followed by a terminal reset.

### Transitions from a known style

`fmt::to_sgr(final_style, initial_style)` returns the SGR needed to change from
the known initial style to the final style, without a trailing reset. The
second argument is required. Both arguments describe complete terminal states:
an empty foreground or background means the corresponding terminal-default
color, and absent emphasis means off. No inheritance occurs here. Resolve any
enclosing style before calling this overload. The absolute flag on the initial
style is ignored. Neither input is modified.

An absolute final style always emits a full reset followed by its own
attributes, even when the initial state has the same attributes. This case
delegates to `fmt::to_sgr(final_style)` and follows its SGR output mode.

```cpp
const auto initial = fmt::bold(fmt::fg(fmt::term_color::red));
const auto final_style = fmt::underline(fmt::fg(fmt::term_color::red));
auto transition = fmt::to_sgr(final_style, initial);
// Compact:    "\x1b[22;4m".
// Noncompact: "\x1b[0m\x1b[4m\x1b[31m".
// Both produce underlined red text without bold.
```

For a relative final style, identical complete states emit nothing in either
output mode. Changed states follow `FMT_COLOR_COMPACT_SGR`:

- Noncompact mode (`0`, the default) resets when the initial state is nondefault,
  then emits the final style's specified colors and emphasis, using separate
  sequences and padded color parameters. It omits the reset when the initial
  state is already defaults. An explicitly absolute target always resets.
- Compact mode (`1`) chooses the shorter of selective changes and
  reset-and-rebuild, preferring selective changes on a tie. Its output is one
  combined sequence with unpadded parameters.

Compact selective changes can restore default foreground or background with
SGR `39` or `49`. Bold and faint share SGR `22` for removal, so a surviving
intensity attribute is reapplied when necessary. Colors are compared by kind
and value, preserving terminal palette semantics. Noncompact mode instead
rebuilds the full requested state, with a reset when needed. Both modes produce the
requested appearance.

Both overloads support C++11 and wide characters; transitions can allocate and
are not `noexcept`.

## `sgr_reset`

::: sgr_reset
    options:
      color_reference: true

`fmt::sgr_reset<Char>` returns a `fmt::basic_string_view<Char>` containing
the four-character sequence `\x1b[0m`. `Char` defaults to `char`; specify
`wchar_t` for a wide-character view. The view refers to static storage and
remains valid for the lifetime of the program. The function allocates no
memory, is `noexcept`, and supports constant evaluation in C++11 and later.
Its output is the same in both SGR modes.

Use the reset after manually styled text to restore terminal-default colors
and emphasis. It does not restore a previously active enclosing style.

```cpp
auto message     = fmt::to_sgr(style) + "message";
const auto reset = fmt::sgr_reset();
message.append(reset.data(), reset.size());

auto wide_message     = fmt::to_sgr<wchar_t>(style) + L"message";
const auto wide_reset = fmt::sgr_reset<wchar_t>();
wide_message.append(wide_reset.data(), wide_reset.size());
```

## `wrap_sgr`

`fmt::wrap_sgr(style, text)` returns a string containing the style's opening
SGR sequences, the text, and a terminal reset. The style comes first. Unlike
`format`, it copies the text literally: braces, embedded nulls, and escape
sequences are preserved without interpretation.

```cpp
const auto style = fmt::bold();
auto message     = fmt::wrap_sgr(style, "Result: {42}");
// "\x1b[1mResult: {42}\x1b[0m" in either mode.
```

Empty text produces an empty string, even with a nonempty style. An empty
style returns only the text. Otherwise, the opening sequences follow the
selected `FMT_COLOR_COMPACT_SGR` mode, and the trailing reset restores terminal
defaults rather than an enclosing style. The function leaves its inputs
unchanged, can allocate, and is not `noexcept`.

The character template accepts `fmt::basic_string_view<Char>` and returns
`std::basic_string<Char>`. A narrow convenience overload accepts arguments
convertible to `fmt::string_view`, including string literals and `std::string`.
For wide text, supply a view or specify the character type explicitly:

```cpp
auto wide_message = fmt::wrap_sgr<wchar_t>(style, L"Result: {42}");
auto wide_view    = fmt::basic_string_view<wchar_t>{L"message"};
auto another      = fmt::wrap_sgr(style, wide_view);
```

## SGR length constants

Both constants are `constexpr std::size_t` values available in C++11 and later:

| Constant                | Value | Meaning                                                                     |
| ----------------------- | ----- | --------------------------------------------------------------------------- |
| `fmt::sgr_max_length`   | `60`  | Upper bound on the opening SGR output for one style, in either output mode. |
| `fmt::sgr_reset_length` | `4`   | Length of the reset sequence returned by `sgr_reset`.                       |

Lengths count `Char` elements, including ESC, brackets, parameters, and the
final `m`. They exclude any terminating null. `sgr_max_length` excludes the
text and trailing reset and is a capacity bound, not the actual output length.
Use the returned string's `size` for the actual opening length.

## `format`

::: format(text_style, format_string<T...>, T&&...)
    options:
      color_reference: true

## `format_to`

::: format_to(OutputIt, text_style, format_string<T...>, T&&...)
    options:
      color_reference: true

## `vformat`

::: vformat(text_style, string_view, format_args)
    options:
      color_reference: true

## `vformat_to`

::: vformat_to(OutputIt, text_style, string_view, format_args)
    options:
      color_reference: true

The `v` functions accept an argument pack created with `fmt::make_format_args`.
The referenced arguments must remain alive while the pack is used.

```cpp
int result   = 42;
auto args    = fmt::make_format_args(result);
auto message = fmt::vformat(fmt::fg(fmt::color::green),
                            "Result: {}", args);
```
