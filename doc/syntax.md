# Format String Syntax {#syntax}

[Formatting functions](api) such as `fmt::format` and `fmt::print` use the same
format string syntax described in this section.

Format strings contain "replacement fields" surrounded by curly braces `{}`.
Anything that is not contained in braces is considered literal text, which is
copied unchanged to the output. If you need to include a brace character in the
literal text, it can be escaped by doubling: `{{` and `}}`.

The grammar for a replacement field is as follows:

<a id="replacement-field"></a>
<code class="grammar">
replacement_field ::= "{" [arg_id] [":" ([format_spec](#format-spec) | [chrono_format_spec](#chrono-format-spec))] "}"
arg_id            ::= integer | identifier
integer           ::= digit+
digit             ::= "0"..."9"
identifier        ::= id_start id_continue*
id_start          ::= "a"..."z" | "A"..."Z" | "_"
id_continue       ::= id_start | digit
</code>

In less formal terms, the replacement field can start with an *arg_id* that
specifies the argument whose value is to be formatted and inserted into the
output instead of the replacement field. The *arg_id* is optionally followed
by a *format_spec*, which is preceded by a colon `':'`. These specify a
non-default format for the replacement value.

See also the [Format Specification Mini-Language](#formatspec) section.

If the numerical arg_ids in a format string are 0, 1, 2, \... in
sequence, they can all be omitted (not just some) and the numbers 0, 1,
2, \... will be automatically inserted in that order.

Named arguments can be referred to by their names or indices.

Some simple format string examples:

    "First, thou shalt count to {0}" // References the first argument
    "Bring me a {}"                  // Implicitly references the first argument
    "From {} to {}"                  // Same as "From {0} to {1}"

The *format_spec* field contains a specification of how the value should
be presented, including such details as field width, alignment, padding,
decimal precision and so on. Each value type can define its own
"formatting mini-language" or interpretation of the *format_spec*.

Most built-in types support a common formatting mini-language, which is
described in the next section.

A *format_spec* field can also include nested replacement fields in
certain positions within it. These nested replacement fields can contain
only an argument id; format specifications are not allowed. This allows
the formatting of a value to be dynamically specified.

See the [Format Examples](#formatexamples) section for some examples.

## Format Specification Mini-Language {#formatspec}

"Format specifications" are used within replacement fields contained within a
format string to define how individual values are presented. Each formattable
type may define how the format specification is to be interpreted.

Most built-in types implement the following options for format
specifications, although some of the formatting options are only
supported by the numeric types.

The general form of a *standard format specifier* is:

<a id="format-spec"></a>
<code class="grammar">
format_spec ::= [[fill]align][sign]["#"]["0"][width]["." precision]["L"][type]
fill        ::= <a character other than '{' or '}'\>
align       ::= "<" | ">" | "^"
sign        ::= "+" | "-" | " "
width       ::= [integer](#replacement-field) | "{" [[arg_id](#replacement-field)] "}"
precision   ::= [integer](#replacement-field) | "{" [[arg_id](#replacement-field)] "}"
type        ::= "a" | "A" | "b" | "B" | "c" | "d" | "e" | "E" | "f" | "F" |
                "g" | "G" | "o" | "p" | "s" | "x" | "X" | "?"
</code>

The *fill* character can be any Unicode code point other than `'{'` or
`'}'`. The presence of a fill character is signaled by the character
following it, which must be one of the alignment options. If the second
character of *format_spec* is not a valid alignment option, then it is
assumed that both the fill character and the alignment option are
absent.

The meaning of the various alignment options is as follows:

| Option | Meaning                                                                                                |
|--------|--------------------------------------------------------------------------------------------------------|
| `'<'`  | Forces the field to be left-aligned within the available space (this is the default for most objects). |
| `'>'`  | Forces the field to be right-aligned within the available space (this is the default for numbers).     |
| `'^'`  | Forces the field to be centered within the available space.                                            |

Note that unless a minimum field width is defined, the field width will
always be the same size as the data to fill it, so that the alignment
option has no meaning in this case.

The *sign* option is only valid for floating point and signed integer
types, and can be one of the following:

| Option | Meaning                                                                                                     |
|--------|-------------------------------------------------------------------------------------------------------------|
| `'+'`  | indicates that a sign should be used for both nonnegative as well as negative numbers.                      |
| `'-'`  | indicates that a sign should be used only for negative numbers (this is the default behavior).              |
| space  | indicates that a leading space should be used on nonnegative numbers, and a minus sign on negative numbers. |

The `'#'` option causes the "alternate form" to be used for the
conversion. The alternate form is defined differently for different
types. This option is only valid for integer and floating-point types.
For integers, when binary, octal, or hexadecimal output is used, this
option adds the prefix respective `"0b"` (`"0B"`), `"0"`, or `"0x"`
(`"0X"`) to the output value. Whether the prefix is lower-case or
upper-case is determined by the case of the type specifier, for example,
the prefix `"0x"` is used for the type `'x'` and `"0X"` is used for
`'X'`. For floating-point numbers the alternate form causes the result
of the conversion to always contain a decimal-point character, even if
no digits follow it. Normally, a decimal-point character appears in the
result of these conversions only if a digit follows it. In addition, for
`'g'` and `'G'` conversions, trailing zeros are not removed from the
result.

*width* is a decimal integer defining the minimum field width. If not
specified, then the field width will be determined by the content.

Preceding the *width* field by a zero (`'0'`) character enables
sign-aware zero-padding for numeric types. It forces the padding to be
placed after the sign or base (if any) but before the digits. This is
used for printing fields in the form "+000000120". This option is only
valid for numeric types and it has no effect on formatting of infinity
and NaN. This option is ignored when any alignment specifier is present.

The *precision* is a decimal number indicating how many digits should be
displayed after the decimal point for a floating-point value formatted
with `'f'` and `'F'`, or before and after the decimal point for a
floating-point value formatted with `'g'` or `'G'`. For non-number types
the field indicates the maximum field size - in other words, how many
characters will be used from the field content. The *precision* is not
allowed for integer, character, Boolean, and pointer values. Note that a
C string must be null-terminated even if precision is specified.

The `'L'` option uses the current locale setting to insert the
appropriate number separator characters. This option is only valid for
numeric types.

Finally, the *type* determines how the data should be presented.

The available string presentation types are:

| Type  | Meaning                                                                 |
|-------|-------------------------------------------------------------------------|
| `'s'` | String format. This is the default type for strings and may be omitted. |
| `'?'` | Debug format. The string is quoted and special characters escaped.      |
| none  | The same as `'s'`.                                                      |

The available character presentation types are:

| Type  | Meaning                                                                       |
|-------|-------------------------------------------------------------------------------|
| `'c'` | Character format. This is the default type for characters and may be omitted. |
| `'?'` | Debug format. The character is quoted and special characters escaped.         |
| none  | The same as `'c'`.                                                            |

The available integer presentation types are:

| Type  | Meaning                                                                                                                                                                       |
|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `'b'` | Binary format. Outputs the number in base 2. Using the `'#'` option with this type adds the prefix `"0b"` to the output value.                                                |
| `'B'` | Binary format. Outputs the number in base 2. Using the `'#'` option with this type adds the prefix `"0B"` to the output value.                                                |
| `'c'` | Character format. Outputs the number as a character.                                                                                                                          |
| `'d'` | Decimal integer. Outputs the number in base 10.                                                                                                                               |
| `'o'` | Octal format. Outputs the number in base 8.                                                                                                                                   |
| `'x'` | Hex format. Outputs the number in base 16, using lower-case letters for the digits above 9. Using the `'#'` option with this type adds the prefix `"0x"` to the output value. |
| `'X'` | Hex format. Outputs the number in base 16, using upper-case letters for the digits above 9. Using the `'#'` option with this type adds the prefix `"0X"` to the output value. |
| none  | The same as `'d'`.                                                                                                                                                            |

Integer presentation types can also be used with character and Boolean values
with the only exception that `'c'` cannot be used with `bool`. Boolean values
are formatted using textual representation, either `true` or `false`, if the
presentation type is not specified.

The available presentation types for floating-point values are:

<table style="width:96%;">
<colgroup>
<col style="width: 13%" />
<col style="width: 81%" />
</colgroup>
<thead>
<tr class="header">
<th>Type</th>
<th>Meaning</th>
</tr>
</thead>
<tbody>
<tr class="odd">
<td><code>'a'</code></td>
<td>Hexadecimal floating point format. Prints the number in base 16 with
prefix <code>"0x"</code> and lower-case letters for digits above 9. Uses
<code>'p'</code> to indicate the exponent.</td>
</tr>
<tr class="even">
<td><code>'A'</code></td>
<td>Same as <code>'a'</code> except it uses upper-case letters for the
prefix, digits above 9 and to indicate the exponent.</td>
</tr>
<tr class="odd">
<td><code>'e'</code></td>
<td>Exponent notation. Prints the number in scientific notation using
the letter 'e' to indicate the exponent.</td>
</tr>
<tr class="even">
<td><code>'E'</code></td>
<td>Exponent notation. Same as <code>'e'</code> except it uses an
upper-case <code>'E'</code> as the separator character.</td>
</tr>
<tr class="odd">
<td><code>'f'</code></td>
<td>Fixed point. Displays the number as a fixed-point number.</td>
</tr>
<tr class="even">
<td><code>'F'</code></td>
<td>Fixed point. Same as <code>'f'</code>, but converts <code>nan</code>
to <code>NAN</code> and <code>inf</code> to <code>INF</code>.</td>
</tr>
<tr class="odd">
<td><code>'g'</code></td>
<td><p>General format. For a given precision <code>p &gt;= 1</code>,
this rounds the number to <code>p</code> significant digits and then
formats the result in either fixed-point format or in scientific
notation, depending on its magnitude.</p>
<p>A precision of <code>0</code> is treated as equivalent to a precision
of <code>1</code>.</p></td>
</tr>
<tr class="even">
<td><code>'G'</code></td>
<td>General format. Same as <code>'g'</code> except switches to
<code>'E'</code> if the number gets too large. The representations of
infinity and NaN are uppercased, too.</td>
</tr>
<tr class="odd">
<td>none</td>
<td>Similar to <code>'g'</code>, except that the default precision is as
high as needed to represent the particular value.</td>
</tr>
</tbody>
</table>

The available presentation types for pointers are:

| Type  | Meaning                                                                   |
|-------|---------------------------------------------------------------------------|
| `'p'` | Pointer format. This is the default type for pointers and may be omitted. |
| none  | The same as `'p'`.                                                        |

## Chrono Format Specifications {#chrono-specs}

Format specifications for chrono duration and time point types as well
as `std::tm` have the following syntax:

<a id="chrono-format-spec"></a>
<code class="grammar">
chrono_format_spec ::= \[\[[fill](#replacement-field)][align](#replacement-field)][[width](#replacement-field)]["." [precision](#replacement-field)][chrono_specs]
chrono_specs       ::= [chrono_specs] conversion_spec | chrono_specs literal_char
conversion_spec    ::= "%" [modifier] chrono_type
literal_char       ::= <a character other than '{', '}' or '%'\>
modifier           ::= "E" | "O"
chrono_type        ::= "a" | "A" | "b" | "B" | "c" | "C" | "d" | "D" | "e" | "F" |
                       "g" | "G" | "h" | "H" | "I" | "j" | "m" | "M" | "n" | "p" |
                       "q" | "Q" | "r" | "R" | "S" | "t" | "T" | "u" | "U" | "V" |
                       "w" | "W" | "x" | "X" | "y" | "Y" | "z" | "Z" | "%"
</code>

Literal chars are copied unchanged to the output. Precision is valid
only for `std::chrono::duration` types with a floating-point
representation type.

The available presentation types (*chrono_type*) are:

| Type  | Meaning                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
|-------|-------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `'a'` | The abbreviated weekday name, e.g. "Sat". If the value does not contain a valid weekday, an exception of type `format_error` is thrown.                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| `'A'` | The full weekday name, e.g. "Saturday". If the value does not contain a valid weekday, an exception of type `format_error` is thrown.                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| `'b'` | The abbreviated month name, e.g. "Nov". If the value does not contain a valid month, an exception of type `format_error` is thrown.                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| `'B'` | The full month name, e.g. "November". If the value does not contain a valid month, an exception of type `format_error` is thrown.                                                                                                                                                                                                                                                                                                                                                                                                                                                                         |
| `'c'` | The date and time representation, e.g. "Sat Nov 12 22:04:00 1955". The modified command `%Ec` produces the locale's alternate date and time representation.                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| `'C'` | The year divided by 100 using floored division, e.g. "19". If the result is a single decimal digit, it is prefixed with 0. The modified command `%EC` produces the locale's alternative representation of the century.                                                                                                                                                                                                                                                                                                                                                                                   |
| `'d'` | The day of month as a decimal number. If the result is a single decimal digit, it is prefixed with 0. The modified command `%Od` produces the locale's alternative representation.                                                                                                                                                                                                                                                                                                                                                                                                                         |
| `'D'` | Equivalent to `%m/%d/%y`, e.g. "11/12/55".                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| `'e'` | The day of month as a decimal number. If the result is a single decimal digit, it is prefixed with a space. The modified command `%Oe` produces the locale's alternative representation.                                                                                                                                                                                                                                                                                                                                                                                                                   |
| `'F'` | Equivalent to `%Y-%m-%d`, e.g. "1955-11-12".                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |
| `'g'` | The last two decimal digits of the ISO week-based year. If the result is a single digit it is prefixed by 0.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| `'G'` | The ISO week-based year as a decimal number. If the result is less than four digits it is left-padded with 0 to four digits.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| `'h'` | Equivalent to `%b`, e.g. "Nov".                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                           |
| `'H'` | The hour (24-hour clock) as a decimal number. If the result is a single digit, it is prefixed with 0. The modified command `%OH` produces the locale's alternative representation.                                                                                                                                                                                                                                                                                                                                                                                                                         |
| `'I'` | The hour (12-hour clock) as a decimal number. If the result is a single digit, it is prefixed with 0. The modified command `%OI` produces the locale's alternative representation.                                                                                                                                                                                                                                                                                                                                                                                                                         |
| `'j'` | If the type being formatted is a specialization of duration, the decimal number of days without padding. Otherwise, the day of the year as a decimal number. Jan 1 is 001. If the result is less than three digits, it is left-padded with 0 to three digits.                                                                                                                                                                                                                                                                                                                                               |
| `'m'` | The month as a decimal number. Jan is 01. If the result is a single digit, it is prefixed with 0. The modified command `%Om` produces the locale's alternative representation.                                                                                                                                                                                                                                                                                                                                                                                                                             |
| `'M'` | The minute as a decimal number. If the result is a single digit, it is prefixed with 0. The modified command `%OM` produces the locale's alternative representation.                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| `'n'` | A new-line character.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       |
| `'p'` | The AM/PM designations associated with a 12-hour clock.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| `'q'` | The duration's unit suffix.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| `'Q'` | The duration's numeric value (as if extracted via `.count()`).                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                             |
| `'r'` | The 12-hour clock time, e.g. "10:04:00 PM".                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                               |
| `'R'` | Equivalent to `%H:%M`, e.g. "22:04".                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      |
| `'S'` | Seconds as a decimal number. If the number of seconds is less than 10, the result is prefixed with 0. If the precision of the input cannot be exactly represented with seconds, then the format is a decimal floating-point number with a fixed format and a precision matching that of the precision of the input (or to a microseconds precision if the conversion to floating-point decimal seconds cannot be made within 18 fractional digits). The character for the decimal point is localized according to the locale. The modified command `%OS` produces the locale's alternative representation. |
| `'t'` | A horizontal-tab character.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| `'T'` | Equivalent to `%H:%M:%S`.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                   |
| `'u'` | The ISO weekday as a decimal number (1-7), where Monday is 1. The modified command `%Ou` produces the locale's alternative representation.                                                                                                                                                                                                                                                                                                                                                                                                                                                                 |
| `'U'` | The week number of the year as a decimal number. The first Sunday of the year is the first day of week 01. Days of the same year prior to that are in week 00. If the result is a single digit, it is prefixed with 0. The modified command `%OU` produces the locale's alternative representation.                                                                                                                                                                                                                                                                                                        |
| `'V'` | The ISO week-based week number as a decimal number. If the result is a single digit, it is prefixed with 0. The modified command `%OV` produces the locale's alternative representation.                                                                                                                                                                                                                                                                                                                                                                                                                   |
| `'w'` | The weekday as a decimal number (0-6), where Sunday is 0. The modified command `%Ow` produces the locale's alternative representation.                                                                                                                                                                                                                                                                                                                                                                                                                                                                     |
| `'W'` | The week number of the year as a decimal number. The first Monday of the year is the first day of week 01. Days of the same year prior to that are in week 00. If the result is a single digit, it is prefixed with 0. The modified command `%OW` produces the locale's alternative representation.                                                                                                                                                                                                                                                                                                        |
| `'x'` | The date representation, e.g. "11/12/55". The modified command `%Ex` produces the locale's alternate date representation.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| `'X'` | The time representation, e.g. "10:04:00". The modified command `%EX` produces the locale's alternate time representation.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                |
| `'y'` | The last two decimal digits of the year. If the result is a single digit it is prefixed by 0. The modified command `%Oy` produces the locale's alternative representation. The modified command `%Ey` produces the locale's alternative representation of offset from `%EC` (year only).                                                                                                                                                                                                                                                                                                                  |
| `'Y'` | The year as a decimal number. If the result is less than four digits it is left-padded with 0 to four digits. The modified command `%EY` produces the locale's alternative full year representation.                                                                                                                                                                                                                                                                                                                                                                                                       |
| `'z'` | The offset from UTC in the ISO 8601:2004 format. For example -0430 refers to 4 hours 30 minutes behind UTC. If the offset is zero, +0000 is used. The modified commands `%Ez` and `%Oz` insert a `:` between the hours and minutes: -04:30. If the offset information is not available, an exception of type `format_error` is thrown.                                                                                                                                                                                                                                                                      |
| `'Z'` | The time zone abbreviation. If the time zone abbreviation is not available, an exception of type `format_error` is thrown.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  |
| `'%'` | A % character.                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              |

Specifiers that have a calendaric component such as `'d'` (the day of
month) are valid only for `std::tm` and time points but not durations.

## Range Format Specifications

Format specifications for range types have the following syntax:

<code class="grammar">
range_format_spec ::= ["n"][range_type][range_underlying_spec]
</code>

The `'n'` option formats the range without the opening and closing
brackets.

The available presentation types for `range_type` are:

| Type   | Meaning                                                    |
|--------|------------------------------------------------------------|
| none   | Default format.                                            |
| `'s'`  | String format. The range is formatted as a string.         |
| `'?s'` | Debug format. The range is formatted as an escaped string. |

If `range_type` is `'s'` or `'?s'`, the range element type must be a character
type. The `'n'` option and `range_underlying_spec` are mutually exclusive with
`'s'` and `'?s'`.

The `range_underlying_spec` is parsed based on the formatter of the range's
element type.

By default, a range of characters or strings is printed escaped and quoted.
But if any `range_underlying_spec` is provided (even if it is empty), then the
characters or strings are printed according to the provided specification.

Examples:

    fmt::format("{}", std::vector{10, 20, 30});
    // Result: [10, 20, 30]
    fmt::format("{::#x}", std::vector{10, 20, 30});
    // Result: [0xa, 0x14, 0x1e]
    fmt::format("{}", vector{'h', 'e', 'l', 'l', 'o'});
    // Result: ['h', 'e', 'l', 'l', 'o']
    fmt::format("{:n}", vector{'h', 'e', 'l', 'l', 'o'});
    // Result: 'h', 'e', 'l', 'l', 'o'
    fmt::format("{:s}", vector{'h', 'e', 'l', 'l', 'o'});
    // Result: "hello"
    fmt::format("{:?s}", vector{'h', 'e', 'l', 'l', 'o', '\n'});
    // Result: "hello\n"
    fmt::format("{::}", vector{'h', 'e', 'l', 'l', 'o'});
    // Result: [h, e, l, l, o]
    fmt::format("{::d}", vector{'h', 'e', 'l', 'l', 'o'});
    // Result: [104, 101, 108, 108, 111]

## Format Examples {#formatexamples}

This section contains examples of the format syntax and comparison with
the printf formatting.

In most of the cases the syntax is similar to the printf formatting,
with the addition of the `{}` and with `:` used instead of `%`. For
example, `"%03.2f"` can be translated to `"{:03.2f}"`.

The new format syntax also supports new and different options, shown in
the following examples.

Accessing arguments by position:

    fmt::format("{0}, {1}, {2}", 'a', 'b', 'c');
    // Result: "a, b, c"
    fmt::format("{}, {}, {}", 'a', 'b', 'c');
    // Result: "a, b, c"
    fmt::format("{2}, {1}, {0}", 'a', 'b', 'c');
    // Result: "c, b, a"
    fmt::format("{0}{1}{0}", "abra", "cad");  // arguments' indices can be repeated
    // Result: "abracadabra"

Aligning the text and specifying a width:

    fmt::format("{:<30}", "left aligned");
    // Result: "left aligned                  "
    fmt::format("{:>30}", "right aligned");
    // Result: "                 right aligned"
    fmt::format("{:^30}", "centered");
    // Result: "           centered           "
    fmt::format("{:*^30}", "centered");  // use '*' as a fill char
    // Result: "***********centered***********"

Dynamic width:

    fmt::format("{:<{}}", "left aligned", 30);
    // Result: "left aligned                  "

Dynamic precision:

    fmt::format("{:.{}f}", 3.14, 1);
    // Result: "3.1"

Replacing `%+f`, `%-f`, and `% f` and specifying a sign:

    fmt::format("{:+f}; {:+f}", 3.14, -3.14);  // show it always
    // Result: "+3.140000; -3.140000"
    fmt::format("{: f}; {: f}", 3.14, -3.14);  // show a space for positive numbers
    // Result: " 3.140000; -3.140000"
    fmt::format("{:-f}; {:-f}", 3.14, -3.14);  // show only the minus -- same as '{:f}; {:f}'
    // Result: "3.140000; -3.140000"

Replacing `%x` and `%o` and converting the value to different bases:

    fmt::format("int: {0:d};  hex: {0:x};  oct: {0:o}; bin: {0:b}", 42);
    // Result: "int: 42;  hex: 2a;  oct: 52; bin: 101010"
    // with 0x or 0 or 0b as prefix:
    fmt::format("int: {0:d};  hex: {0:#x};  oct: {0:#o};  bin: {0:#b}", 42);
    // Result: "int: 42;  hex: 0x2a;  oct: 052;  bin: 0b101010"

Padded hex byte with prefix and always prints both hex characters:

    fmt::format("{:#04x}", 0);
    // Result: "0x00"

Box drawing using Unicode fill:

    fmt::print(
      "┌{0:─^{2}}┐\n"
      "│{1: ^{2}}│\n"
      "└{0:─^{2}}┘\n", "", "Hello, world!", 20);

prints:

    ┌────────────────────┐
    │   Hello, world!    │
    └────────────────────┘

Using type-specific formatting:

    #include <fmt/chrono.h>

    auto t = tm();
    t.tm_year = 2010 - 1900;
    t.tm_mon = 7;
    t.tm_mday = 4;
    t.tm_hour = 12;
    t.tm_min = 15;
    t.tm_sec = 58;
    fmt::print("{:%Y-%m-%d %H:%M:%S}", t);
    // Prints: 2010-08-04 12:15:58

Using the comma as a thousands separator:

    #include <fmt/format.h>

    auto s = fmt::format(std::locale("en_US.UTF-8"), "{:L}", 1234567890);
    // s == "1,234,567,890"
