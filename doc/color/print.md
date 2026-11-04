# Styled Output

Include `<fmt/color.h>` to use these output overloads. See
[Text style rendering](index.md#TextStyleRendering) for the shared styling rules.

## print to a stream

::: print(FILE*, text_style, format_string<T...>, T&&...)
    options:
      color_reference: true

## print to stdout

::: print(text_style, format_string<T...>, T&&...)
    options:
      color_reference: true

## println to a stream

::: println(FILE*, text_style, format_string<T...>, T&&...)
    options:
      color_reference: true

## println to stdout

::: println(text_style, format_string<T...>, T&&...)
    options:
      color_reference: true

## vprint

::: vprint(FILE*, text_style, string_view, format_args)
    options:
      color_reference: true

## vprintln

::: vprintln(FILE*, text_style, string_view, format_args)
    options:
      color_reference: true

The `v` functions accept an argument pack created with `fmt::make_format_args`.
The referenced arguments must remain alive while the pack is used.

```cpp
int result = 42;
auto args = fmt::make_format_args(result);
fmt::vprintln(stdout, fmt::fg(fmt::color::green), "Result: {}", args);
```

## Styled ostream output

Include `<fmt/color-ostream.h>` to use the styled `std::ostream` overloads.
This bridge includes both `<fmt/color.h>` and `<fmt/ostream.h>`; the two
existing headers remain independent.

```cpp
#include <fmt/color-ostream.h>
#include <sstream>

std::ostringstream os;
fmt::print(os, fmt::text_style{fmt::emphasis::bold}, "Result: {}", 42);
fmt::println(os, fmt::fg(fmt::term_color::red), "Status: {}", "failed");
```

The style applies to the entire formatted message. An empty style emits no
SGR sequences. A nonempty style ends with a reset to terminal defaults, which
does not restore an enclosing style. `println` places its newline after the
reset. Stream state and exception handling follow the existing ostream output
functions. These overloads support C++11 and later. `print` and `println`
provide separate overloads for `std::ostream&` and `std::wostream&`;
`vprint` and `vprintln` take `std::basic_ostream<Char>&`.
The format string and argument pack must match the stream character type.
Use narrow format strings for narrow streams and wide format strings for wide
streams. Including the bridge also includes `<fmt/xchar.h>` for wide formatting.


```cpp
std::wostringstream wide;
const auto style = fmt::text_style{fmt::emphasis::bold};
fmt::print(wide, style, L"Result: {:04x}", 42);
fmt::println(wide, style, L"Status: {}", L"done");

int result = 42;
auto args = fmt::make_wformat_args(result);
fmt::vprint(wide, style, L"Result: {}", args);
fmt::vprintln(wide, style, L"Result: {}", args);
```

The formatted-output functions use fmt format specifications and leave stream
formatting settings such as field width and numeric base unchanged. Wide and
narrow output follow the same styling, newline, and reset rules.

::: print(std::ostream&, text_style, format_string<T...>, T&&...)
    options:
      color_reference: true

::: print(std::wostream&, text_style, wformat_string<T...>, T&&...)
    options:
      color_reference: true

::: println(std::ostream&, text_style, format_string<T...>, T&&...)
    options:
      color_reference: true

::: println(std::wostream&, text_style, wformat_string<T...>, T&&...)
    options:
      color_reference: true

::: vprint(std::basic_ostream<Char>&, text_style, typename detail::color_ostream_types<Char>::string_view, typename detail::color_ostream_types<Char>::format_args)
    options:
      color_reference: true

::: vprintln(std::basic_ostream<Char>&, text_style, typename detail::color_ostream_types<Char>::string_view, typename detail::color_ostream_types<Char>::format_args)
    options:
      color_reference: true

## Styled stream insertion

With `<fmt/color-ostream.h>`, insert the result of `fmt::styled(value, style)`
directly into a stream. The operator returns the same stream by reference for
chaining and supports both `std::ostream` and `std::wostream` in C++11 and later.
The value must support insertion into that stream; a fmt formatter is not
required.

```cpp
#include <fmt/color-ostream.h>
#include <iomanip>
#include <sstream>

const auto style = fmt::text_style{fmt::emphasis::bold};
std::ostringstream os;
os << "Result: " << std::hex << std::setfill('0') << std::setw(4)
   << fmt::styled(42, style) << '\n';
// "Result: \x1b[1m002a\x1b[0m\n"

std::wostringstream wide;
wide << fmt::styled(L"message", style) << L'\n';
```

The underlying value uses normal stream formatting, including numeric base,
precision, fill, alignment, and field width. SGR sequences do not consume the
field width; any padding produced by the value's insertion is inside the
styled span. Stream formatting settings follow the value's ordinary insertion
behavior. Strings are inserted literally, without interpreting braces as format
fields.

A nonempty style emits opening SGR sequences selected by
[`FMT_COLOR_COMPACT_SGR`](format.md#to_sgr), followed by the value and a reset
to terminal defaults. An empty style simply inserts the value and emits no
reset. Inserting an empty string with a nonempty style still emits the opening
sequences and reset. A reset does not restore an enclosing style.

If inserting the value throws and exceptions are enabled, the operator attempts
to write a reset, ignores any exception from that recovery write, and rethrows
the original exception. A failed stream may prevent the reset from being
written. On the normal path, stream failures follow the stream's exception
mask, including failures while writing the opening sequences or final reset.
The operator also supports builds with exceptions disabled.

`styled` stores a reference to the value and a copy of the style. Immediate
insertion of a temporary value is valid for that expression. If you save the
wrapper, its referenced value must remain alive until insertion is complete.
