# Format String Syntax

The formatting functions in this library — most notably
[`fmt::format`](api.md#format) and [`fmt::print`](api.md#print) — accept
format strings written in the syntax described here.

A format string is plain text with embedded *replacement fields* delimited by
the braces `{` and `}`. Characters outside of any replacement field are
copied to the output unchanged. To emit a literal brace, double it: `{{`
yields a single `{` in the output, and `}}` yields a single `}`.

A replacement field is described by the grammar below.

<a id="replacement-field"></a>
<pre><code class="language-json"
>replacement_field ::= "{" [arg_id] [":" (<a href="#format-spec"
  >format_spec</a> | <a href="#chrono-format-spec">chrono_format_spec</a>)] "}"
arg_id            ::= integer | identifier
integer           ::= digit+
digit             ::= "0"..."9"
identifier        ::= id_start id_continue*
id_start          ::= "a"..."z" | "A"..."Z" | "_"
id_continue       ::= id_start | digit</code>
</pre>

An *arg_id* selects which argument to format. It may be a non-negative
integer (positional reference) or an identifier matching the name of an
argument passed via [`fmt::arg`](api.md#arg) (named reference). When *arg_id*
is omitted, arguments are consumed in left-to-right order; this *automatic*
indexing must be used uniformly throughout the format string — mixing
automatic and explicit numeric ids is a compile-time error (or a
`format_error` at runtime).

A *format_spec*, introduced by `:`, describes how the value should be
rendered. Its grammar is type-dependent; the form used by the standard
built-in types is documented in the next section.

For example:

```c++
fmt::format("hello, {}", "world");
// Result: "hello, world"

fmt::format("{1}, {0}!", "world", "hello");
// Result: "hello, world!"

fmt::format("{greeting}, {name}!",
            fmt::arg("greeting", "hi"), fmt::arg("name", "fmt"));
// Result: "hi, fmt!"
```

A *width* or *precision* inside a *format_spec* may itself be written as a
nested replacement field — `{}` or `{arg_id}` — in which case it takes its
value from an integer argument at runtime. Nested fields accept only an
*arg_id*; they cannot themselves contain a *format_spec*.

## Format Specification

The grammar below describes the *format_spec* shared by the built-in
types — integers, floating-point values, characters, strings, booleans, and
pointers — as well as by any user-defined type whose `formatter` reuses
fmt's parser.

<a id="format-spec"></a>
<pre><code class="language-json"
>format_spec ::= [[fill]align][sign]["#"]["0"][width]["." precision]["L"][type]
fill        ::= &lt;a character other than '{' or '}'>
align       ::= "<" | ">" | "^"
sign        ::= "+" | "-" | " "
width       ::= <a href="#replacement-field">integer</a> | "{" [<a
  href="#replacement-field">arg_id</a>] "}"
precision   ::= <a href="#replacement-field">integer</a> | "{" [<a
  href="#replacement-field">arg_id</a>] "}"
type        ::= "a" | "A" | "b" | "B" | "c" | "d" | "e" | "E" | "f" | "F" |
                "g" | "G" | "o" | "p" | "s" | "x" | "X" | "?"</code>
</pre>

Whether a particular option is meaningful depends on the value being
formatted; options that do not apply to a value's type are diagnosed at
compile time when possible and otherwise raise a `format_error`.

### Fill and alignment

The *align* field selects where padding is placed when *width* makes the
field wider than the value's natural rendering.

| Option | Effect                                                            |
|--------|-------------------------------------------------------------------|
| `<`    | Left-align; pad on the right. Default for non-numeric types.      |
| `>`    | Right-align; pad on the left. Default for numeric types.          |
| `^`    | Center the value; if the padding cannot be split evenly, the extra padding character goes on the right. |

The *fill* character is any single Unicode code point other than `{` or `}`,
encoded the same way as the format string. It supplies the padding character
in place of the default space. A fill character is recognized only when it is
immediately followed by an *align* character — otherwise it would be
indistinguishable from an option in another position — so to use a custom
fill you must also specify an alignment.

Alignment has no observable effect when the value's natural rendering is
already at least as wide as *width*; the value is never truncated to fit.

```c++
fmt::format("[{:<10}]", "42");   // Result: "[42        ]"
fmt::format("[{:>10}]", "42");   // Result: "[        42]"
fmt::format("[{:^10}]", "42");   // Result: "[    42    ]"
fmt::format("[{:*^10}]", "42");  // Result: "[****42****]"  - '*' as fill
```

### Sign

The *sign* field controls how the sign of a numeric value is emitted. It
applies to signed integer and floating-point types only.

| Option | Effect                                                            |
|--------|-------------------------------------------------------------------|
| `+`    | Always emit a sign (`+` for nonnegative values, `-` for negative).|
| `-`    | Emit `-` only for negative values. This is the default.           |
| space  | Emit a leading space for nonnegative values and `-` for negative ones; useful for aligning columns of signed numbers. |

The sign of `-0.0` is preserved in floating-point output.

```c++
fmt::format("{:+d} {:+d}", 7, -7);  // Result: "+7 -7"
fmt::format("{: d} {: d}", 7, -7);  // Result: " 7 -7"
```

### Alternate form (`#`)

The `#` flag selects an *alternate form* whose exact meaning depends on the
presentation type:

- For integers rendered in binary, octal, or hexadecimal, it prepends the
  appropriate base prefix (`0b`/`0B`, `0`, or `0x`/`0X`). The case of the
  prefix follows the case of the type specifier — `0x` for `x`, `0X` for
  `X`, and so on.
- For floating-point values, it forces the decimal point to appear in the
  output even if no fractional digits would otherwise be emitted, and
  prevents the `g`/`G` presentation types from removing trailing zeros from
  the significand.

The `#` flag is not accepted by non-numeric types.

### Zero padding (`0`)

A `0` placed immediately before *width* enables sign-aware zero padding for
numeric types. Zeros are inserted between the sign (or base prefix, if any)
and the most significant digit, so that a sign or `0x` prefix stays adjacent
to the digits rather than being separated by spaces. For example, `{:+08d}`
applied to `120` produces `+0000120`.

Zero padding:

- applies only to numeric types;
- has no effect on `inf` or `nan`;
- is ignored when an explicit *align* is also present.

### Width

*width* is a non-negative decimal integer giving the minimum number of
characters that the field should occupy. If the formatted value is shorter
than *width*, it is padded according to *align* and *fill*; if it is longer,
the value is written in full. *width* never causes the value to be
truncated.

To supply *width* at runtime, write the field as `{}` to consume the next
argument, or as `{arg_id}` to reference an integer argument by position or
by name.

When formatting strings, "width" is measured in display columns using a
Unicode-aware estimate (East Asian wide and fullwidth characters, plus
common emoji ranges, count as two columns; everything else counts as one).
This keeps fixed *width* values visually consistent in monospace renderings
that combine Latin and CJK text.

```c++
fmt::format("[{:6}]", 42);
// Result: "[    42]"  - right-aligned by default
fmt::format("[{:6}]", "hi");
// Result: "[hi    ]"  - left-aligned by default
fmt::format("[{:{}}]", 42, 6);
// Result: "[    42]"  - width from an argument
```

### Precision

*precision* is a non-negative decimal integer (introduced by `.`) whose
meaning depends on the value being formatted. As with *width*, it may be
supplied as a nested replacement field for runtime evaluation.

| Type                          | Meaning of `.precision`                       |
|-------------------------------|-----------------------------------------------|
| `e`, `E`, `f`, `F`            | Digits emitted after the decimal point.       |
| `g`, `G`                      | Total number of significant digits.           |
| `a`, `A`                      | Digits after the decimal point in the hexadecimal significand. If omitted, just enough digits are emitted to round-trip the value exactly. |
| Strings (`s`, `?`, or default) | Upper bound on the number of code points copied from the value. |

A *precision* is not accepted for integer, character, boolean, or pointer
types. When a *precision* limits the number of characters taken from a C
string, the string must still be null-terminated.

```c++
fmt::format("{:.2f}", 3.14159);        // Result: "3.14"
fmt::format("{:.3g}", 3.14159);        // Result: "3.14"
fmt::format("{:.4}", "hello, world");  // Result: "hell"
fmt::format("{:.{}f}", 3.14159, 4);
// Result: "3.1416"  - precision from an argument
```

### Locale (`L`)

The `L` flag selects locale-sensitive formatting for numeric types. The
formatter inspects the C++ locale supplied to the formatting function (or
the global locale, if none was passed) and inserts the locale's digit
grouping characters and — for floating-point values — its decimal point.
The flag has no effect on non-numeric types.

```c++
auto loc = std::locale("en_US.UTF-8");
fmt::format(loc, "{:L}", 1234567890);     // Result: "1,234,567,890"
fmt::format(loc, "{:.2Lf}", 1234567.89);  // Result: "1,234,567.89"
```

### Presentation type

The *type* field chooses the representation for the value. Specifiers are
grouped below by the value categories they apply to.

**Integers, booleans, and characters:**

| Type | Effect                                                              |
|------|---------------------------------------------------------------------|
| `b`  | Base 2. The `#` flag adds a `0b` prefix.                            |
| `B`  | Base 2. The `#` flag adds a `0B` prefix.                            |
| `c`  | Render the integer as the character with that code point. Not allowed for `bool`. |
| `d`  | Base 10. The default for integer types.                             |
| `o`  | Base 8.                                                             |
| `x`  | Base 16, lower-case digits. The `#` flag adds a `0x` prefix.        |
| `X`  | Base 16, upper-case digits. The `#` flag adds a `0X` prefix.        |
| none | Same as `d` for integers, `c` for characters, and the textual form (`true`/`false`) for `bool`. |

```c++
fmt::format("{:d} {:#x} {:#o} {:#b}", 42, 42, 42, 42);
// Result: "42 0x2a 052 0b101010"

fmt::format("{:#06x}", 0xfe);  // # adds the prefix, 06 zero-pads to width 6
// Result: "0x00fe"
```

**Floating-point values:**

| Type | Effect                                                              |
|------|---------------------------------------------------------------------|
| `a`  | Hexadecimal-significand form (e.g. `1.8p+1`). Lower-case digits and a lower-case `p` for the binary exponent. The `#` flag adds a `0x` prefix. |
| `A`  | Same as `a`, but upper-case throughout.                             |
| `e`  | Scientific notation with a lower-case `e` for the decimal exponent. |
| `E`  | Scientific notation with an upper-case `E`.                         |
| `f`  | Fixed-point notation.                                               |
| `F`  | Same as `f`, but renders `nan` as `NAN` and `inf` as `INF`.         |
| `g`  | General form: scientific notation when the exponent would be less than &minus;4 or not less than the precision, otherwise fixed-point; trailing zeros are removed from the fractional part unless `#` is set. A precision of `0` is interpreted as `1`. |
| `G`  | Same as `g`, but uses `E` for the exponent and upper-case `INF`/`NAN`. |
| none | Shortest round-trip representation: the formatted value, when parsed back into the same floating-point type, reproduces the input bit for bit. |

**Strings and characters:**

| Type | Effect                                                              |
|------|---------------------------------------------------------------------|
| `s`  | Plain string output. Default for string types and for `bool` (which is rendered as `true` or `false`). |
| `c`  | Character output. Default for character types. Not allowed for `bool`. |
| `?`  | Debug output: the value is wrapped in single quotes (characters) or double quotes (strings), and non-printable, non-ASCII, and special characters are escaped using C-style escape sequences such as `\n`, `\t`, `\"`, and `\u{...}`. |
| none | Same as `s` for strings and `bool`, and as `c` for characters.      |

```c++
fmt::format("{}",   "tab\there");  // Result contains a literal tab character.
fmt::format("{:?}", "tab\there");  // Result: "\"tab\\there\""
```

**Pointers:**

| Type | Effect                                                              |
|------|---------------------------------------------------------------------|
| `p`  | Hexadecimal address prefixed by `0x`. Default for pointer types.    |
| none | Same as `p`.                                                        |

A C string (`char*` or `const char*`) accepts both the string presentation
types and `p`, so the same value can be formatted as either text or an
address.

## Chrono Format Specifications

Format specifications for chrono duration and time point types as well as
`std::tm` have the following syntax:

<a id="chrono-format-spec"></a>
<pre><code class="language-json"
>chrono_format_spec ::= [[<a href="#format-spec">fill</a>]<a href="#format-spec"
  >align</a>][<a href="#format-spec">width</a>]["." <a href="#format-spec"
  >precision</a>][chrono_specs]
chrono_specs       ::= conversion_spec |
                       chrono_specs (conversion_spec | literal_char)
conversion_spec    ::= "%" [padding_modifier] [locale_modifier] chrono_type
literal_char       ::= &lt;a character other than '{', '}' or '%'>
padding_modifier   ::= "-" | "_"  | "0"
locale_modifier    ::= "E" | "O"
chrono_type        ::= "a" | "A" | "b" | "B" | "c" | "C" | "d" | "D" | "e" |
                       "F" | "g" | "G" | "h" | "H" | "I" | "j" | "m" | "M" |
                       "n" | "p" | "q" | "Q" | "r" | "R" | "S" | "t" | "T" |
                       "u" | "U" | "V" | "w" | "W" | "x" | "X" | "y" | "Y" |
                       "z" | "Z" | "%"</code>
</pre>

Literal chars are copied unchanged to the output. Precision is valid only
for `std::chrono::duration` types with a floating-point representation type.

The available presentation types (*chrono_type*) are:

<table>
<tr>
  <th>Type</th>
  <th>Meaning</th>
</tr>
<tr>
  <td><code>'a'</code></td>
  <td>
    The abbreviated weekday name, e.g. "Sat". If the value does not contain a
    valid weekday, an exception of type <code>format_error</code> is thrown.
  </td>
</tr>
<tr>
  <td><code>'A'</code></td>
  <td>
    The full weekday name, e.g. "Saturday". If the value does not contain a
    valid weekday, an exception of type <code>format_error</code> is thrown.
  </td>
</tr>
<tr>
  <td><code>'b'</code></td>
  <td>
    The abbreviated month name, e.g. "Nov". If the value does not contain a
    valid month, an exception of type <code>format_error</code> is thrown.
  </td>
</tr>
<tr>
  <td><code>'B'</code></td>
  <td>
    The full month name, e.g. "November". If the value does not contain a valid
    month, an exception of type <code>format_error</code> is thrown.
  </td>
</tr>
<tr>
  <td><code>'c'</code></td>
  <td>
    The date and time representation, e.g. "Sat Nov 12 22:04:00 1955". The
    modified command <code>%Ec</code> produces the locale's alternate date and
    time representation.
  </td>
</tr>
<tr>
  <td><code>'C'</code></td>
  <td>
    The year divided by 100 using floored division, e.g. "19". If the result
    is a single decimal digit, it is prefixed with 0. The modified command
    <code>%EC</code> produces the locale's alternative representation of the
    century.
  </td>
</tr>
<tr>
  <td><code>'d'</code></td>
  <td>
    The day of month as a decimal number. If the result is a single decimal
    digit, it is prefixed with 0. The modified command <code>%Od</code>
    produces the locale's alternative representation.
  </td>
</tr>
<tr>
  <td><code>'D'</code></td>
  <td>Equivalent to <code>%m/%d/%y</code>, e.g. "11/12/55".</td>
</tr>
<tr>
  <td><code>'e'</code></td>
  <td>
    The day of month as a decimal number. If the result is a single decimal
    digit, it is prefixed with a space. The modified command <code>%Oe</code>
    produces the locale's alternative representation.
  </td>
</tr>
<tr>
  <td><code>'F'</code></td>
  <td>Equivalent to <code>%Y-%m-%d</code>, e.g. "1955-11-12".</td>
</tr>
<tr>
  <td><code>'g'</code></td>
  <td>
    The last two decimal digits of the ISO week-based year. If the result is a
    single digit it is prefixed by 0.
  </td>
</tr>
<tr>
  <td><code>'G'</code></td>
  <td>
    The ISO week-based year as a decimal number. If the result is less than
    four digits it is left-padded with 0 to four digits.
  </td>
</tr>
<tr>
  <td><code>'h'</code></td>
  <td>Equivalent to <code>%b</code>, e.g. "Nov".</td>
</tr>
<tr>
  <td><code>'H'</code></td>
  <td>
    The hour (24-hour clock) as a decimal number. If the result is a single
    digit, it is prefixed with 0. The modified command <code>%OH</code>
    produces the locale's alternative representation.
  </td>
</tr>
<tr>
  <td><code>'I'</code></td>
  <td>
    The hour (12-hour clock) as a decimal number. If the result is a single
    digit, it is prefixed with 0. The modified command <code>%OI</code>
    produces the locale's alternative representation.
  </td>
</tr>
<tr>
  <td><code>'j'</code></td>
  <td>
    If the type being formatted is a specialization of duration, the decimal
    number of days without padding. Otherwise, the day of the year as a decimal
    number. Jan 1 is 001. If the result is less than three digits, it is
    left-padded with 0 to three digits.
  </td>
</tr>
<tr>
  <td><code>'m'</code></td>
  <td>
    The month as a decimal number. Jan is 01. If the result is a single digit,
    it is prefixed with 0. The modified command <code>%Om</code> produces the
    locale's alternative representation.
  </td>
</tr>
<tr>
  <td><code>'M'</code></td>
  <td>
    The minute as a decimal number. If the result is a single digit, it
    is prefixed with 0. The modified command <code>%OM</code> produces the
    locale's alternative representation.
  </td>
</tr>
<tr>
  <td><code>'n'</code></td>
  <td>A new-line character.</td>
</tr>
<tr>
  <td><code>'p'</code></td>
  <td>The AM/PM designations associated with a 12-hour clock.</td>
</tr>
<tr>
  <td><code>'q'</code></td>
  <td>The duration's unit suffix.</td>
</tr>
<tr>
  <td><code>'Q'</code></td>
  <td>
    The duration's numeric value (as if extracted via <code>.count()</code>).
  </td>
</tr>
<tr>
  <td><code>'r'</code></td>
  <td>The 12-hour clock time, e.g. "10:04:00 PM".</td>
</tr>
<tr>
  <td><code>'R'</code></td>
  <td>Equivalent to <code>%H:%M</code>, e.g. "22:04".</td>
</tr>
<tr>
  <td><code>'S'</code></td>
  <td>
    Seconds as a decimal number. If the number of seconds is less than 10, the
    result is prefixed with 0. If the precision of the input cannot be exactly
    represented with seconds, then the format is a decimal floating-point number
    with a fixed format and a precision matching that of the precision of the
    input (or to a microseconds precision if the conversion to floating-point
    decimal seconds cannot be made within 18 fractional digits). The modified
    command <code>%OS</code> produces the locale's alternative representation.
  </td>
</tr>
<tr>
  <td><code>'t'</code></td>
  <td>A horizontal-tab character.</td>
</tr>
<tr>
  <td><code>'T'</code></td>
  <td>Equivalent to <code>%H:%M:%S</code>.</td>
</tr>
<tr>
  <td><code>'u'</code></td>
  <td>
    The ISO weekday as a decimal number (1-7), where Monday is 1. The modified
    command <code>%Ou</code> produces the locale's alternative representation.
  </td>
</tr>
<tr>
  <td><code>'U'</code></td>
  <td>
    The week number of the year as a decimal number. The first Sunday of the
    year is the first day of week 01. Days of the same year prior to that are
    in week 00. If the result is a single digit, it is prefixed with 0.
    The modified command <code>%OU</code> produces the locale's alternative
    representation.
  </td>
</tr>
<tr>
  <td><code>'V'</code></td>
  <td>
    The ISO week-based week number as a decimal number. If the result is a
    single digit, it is prefixed with 0. The modified command <code>%OV</code>
    produces the locale's alternative representation.
  </td>
</tr>
<tr>
  <td><code>'w'</code></td>
  <td>
    The weekday as a decimal number (0-6), where Sunday is 0. The modified
    command <code>%Ow</code> produces the locale's alternative representation.
  </td>
</tr>
<tr>
  <td><code>'W'</code></td>
  <td>
    The week number of the year as a decimal number. The first Monday of the
    year is the first day of week 01. Days of the same year prior to that are
    in week 00. If the result is a single digit, it is prefixed with 0.
    The modified command <code>%OW</code> produces the locale's alternative
    representation.
  </td>
</tr>
<tr>
  <td><code>'x'</code></td>
  <td>
    The date representation, e.g. "11/12/55". The modified command
    <code>%Ex</code> produces the locale's alternate date representation.
  </td>
</tr>
<tr>
  <td><code>'X'</code></td>
  <td>
    The time representation, e.g. "10:04:00". The modified command
    <code>%EX</code> produces the locale's alternate time representation.
  </td>
</tr>
<tr>
  <td><code>'y'</code></td>
  <td>
    The last two decimal digits of the year. If the result is a single digit
    it is prefixed by 0. The modified command <code>%Oy</code> produces the
    locale's alternative representation. The modified command <code>%Ey</code>
    produces the locale's alternative representation of offset from
    <code>%EC</code> (year only).
  </td>
</tr>
<tr>
  <td><code>'Y'</code></td>
  <td>
    The year as a decimal number. If the result is less than four digits it is
    left-padded with 0 to four digits. The modified command <code>%EY</code>
    produces the locale's alternative full year representation.
  </td>
</tr>
<tr>
  <td><code>'z'</code></td>
  <td>
    The offset from UTC in the ISO 8601:2004 format. For example -0430 refers
    to 4 hours 30 minutes behind UTC. If the offset is zero, +0000 is used.
    The modified commands <code>%Ez</code> and <code>%Oz</code> insert a
    <code>:</code> between the hours and minutes: -04:30. If the offset
    information is not available, an exception of type
    <code>format_error</code> is thrown.
  </td>
</tr>
<tr>
  <td><code>'Z'</code></td>
  <td>
    The time zone abbreviation. If the time zone abbreviation is not available,
    an exception of type <code>format_error</code> is thrown.
  </td>
</tr>
<tr>
  <td><code>'%'</code></td>
  <td>A % character.</td>
</tr>
</table>

Specifiers that have a calendaric component such as `'d'` (the day of month)
are valid only for `std::tm` and time points but not durations.

The available padding modifiers (*padding_modifier*) are:

| Type  | Meaning                                 |
|-------|-----------------------------------------|
| `'_'` | Pad a numeric result with spaces.       |
| `'-'` | Do not pad a numeric result string.     |
| `'0'` | Pad a numeric result string with zeros. |

These modifiers are only supported for the `'H'`, `'I'`, `'M'`, `'S'`, `'U'`,
`'V'`, `'W'`, `'Y'`, `'d'`, `'j'` and `'m'` presentation types.

Example:

```c++
#include <fmt/chrono.h>

auto t = std::tm();
t.tm_year = 2010 - 1900;
t.tm_mon = 7;
t.tm_mday = 4;
t.tm_hour = 12;
t.tm_min = 15;
t.tm_sec = 58;
fmt::print("{:%Y-%m-%d %H:%M:%S}", t);
// Prints: 2010-08-04 12:15:58
```

## Range Format Specifications

Format specifications for range types have the following syntax:

<pre><code class="language-json"
>range_format_spec ::= ["n"][range_type][":" range_underlying_spec]</code>
</pre>

The `'n'` option formats the range without the opening and closing brackets.

The available presentation types for `range_type` are:

| Type   | Meaning                                                    |
|--------|------------------------------------------------------------|
| none   | Default format.                                            |
| `'s'`  | String format. The range is formatted as a string.         |
| `'?⁠s'` | Debug format. The range is formatted as an escaped string. |

If `range_type` is `'s'` or `'?s'`, the range element type must be a character
type. The `'n'` option and `range_underlying_spec` are mutually exclusive with
`'s'` and `'?s'`.

The `range_underlying_spec` is parsed based on the formatter of the range's
element type.

By default, a range of characters or strings is printed escaped and quoted.
But if any `range_underlying_spec` is provided (even if it is empty), then the
characters or strings are printed according to the provided specification.

Examples:

```c++
fmt::print("{}", std::vector{10, 20, 30});
// Output: [10, 20, 30]
fmt::print("{::#x}", std::vector{10, 20, 30});
// Output: [0xa, 0x14, 0x1e]
fmt::print("{}", std::vector{'h', 'e', 'l', 'l', 'o'});
// Output: ['h', 'e', 'l', 'l', 'o']
fmt::print("{:n}", std::vector{'h', 'e', 'l', 'l', 'o'});
// Output: 'h', 'e', 'l', 'l', 'o'
fmt::print("{:s}", std::vector{'h', 'e', 'l', 'l', 'o'});
// Output: "hello"
fmt::print("{:?s}", std::vector{'h', 'e', 'l', 'l', 'o', '\n'});
// Output: "hello\n"
fmt::print("{::}", std::vector{'h', 'e', 'l', 'l', 'o'});
// Output: [h, e, l, l, o]
fmt::print("{::d}", std::vector{'h', 'e', 'l', 'l', 'o'});
// Output: [104, 101, 108, 108, 111]
fmt::print("{:n:f}", std::array{std::numbers::pi, std::numbers::e});
// Output: 3.141593, 2.718282
```

## A Combined Example

The example below ties together several elements introduced above — nested
replacement fields, fill characters, and centering — to draw a fixed-width
box around a message:

```c++
fmt::print(
    "┌{0:─^{2}}┐\n"
    "│{1: ^{2}}│\n"
    "└{0:─^{2}}┘\n", "", "Hello, world!", 20);
```

Output:

```
┌────────────────────┐
│   Hello, world!    │
└────────────────────┘
```
