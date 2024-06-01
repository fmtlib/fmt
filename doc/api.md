# API Reference

The {fmt} library API consists of the following parts:

- [`fmt/base.h`](#base-api): the base API providing main formatting functions
  for `char`/UTF-8 with C++20 compile-time checks and minimal dependencies
- [`fmt/format.h`](#format-api): the full format API providing additional
  formatting functions and locale support
- [`fmt/ranges.h`](#ranges-api): formatting of ranges and tuples
- [`fmt/chrono.h`](#chrono-api): date and time formatting
- [`fmt/std.h`](#std-api): formatters for standard library types
- [`fmt/compile.h`](#compile-api): format string compilation
- [`fmt/color.h`](#color-api): terminal color and text style
- [`fmt/os.h`](#os-api): system APIs
- [`fmt/ostream.h`](#ostream-api): `std::ostream` support
- [`fmt/args.h`](#args-api): dynamic argument lists
- [`fmt/printf.h`](#printf-api): `printf` formatting
- [`fmt/xchar.h`](#xchar-api): optional `wchar_t` support

All functions and types provided by the library reside in namespace `fmt`
and macros have prefix `FMT_`.

## Base API

`fmt/base.h` defines the base API which provides main formatting functions
for `char`/UTF-8 with C++20 compile-time checks. It has minimal include
dependencies for better compile times. This header is only beneficial when
using {fmt} as a library (the default) and not in the header-only mode.
It also provides `formatter` specializations for the following types:

- `int`, `unsigned`, `long long`, `unsigned long long`
- `float`, `double`, `long double`
- `bool`
- `char`
- `const char*`, `fmt::string_view`
- `const void*`

The following functions use [format string syntax](syntax.md) similar to that
of [str.format](https://docs.python.org/3/library/stdtypes.html#str.format)
in Python. They take *fmt* and *args* as arguments.

*fmt* is a format string that contains literal text and replacement fields
surrounded by braces `{}`. The fields are replaced with formatted arguments
in the resulting string. [\~fmt::format_string]{.title-ref} is a format string
which can be implicitly constructed from a string literal or a `constexpr`
string and is checked at compile time in C++20. To pass a runtime format
string wrap it in [fmt::runtime]{.title-ref}.

*args* is an argument list representing objects to be formatted.

I/O errors are reported as [`std::system_error`](
https://en.cppreference.com/w/cpp/error/system_error) exceptions unless
specified otherwise.

<a id="print"></a>
::: print(format_string<T...>, T&&...)

::: print(FILE*, format_string<T...>, T&&...)

::: println(format_string<T...>, T&&...)

::: println(FILE*, format_string<T...>, T&&...)

::: format_to(OutputIt&&, format_string<T...>, T&&...)

::: format_to_n(OutputIt, size_t, format_string<T...>, T&&...)

::: formatted_size(format_string<T...>, T&&...)

::: format_to_n_result

### Compile-Time Format String Checks

Compile-time format string checks are enabled by default on compilers
that support C++20 `consteval`. On older compilers you can use the
`FMT_STRING <legacy-checks>`{.interpreted-text role="ref"}: macro
defined in `fmt/format.h` instead.

Unused arguments are allowed as in Python's `str.format` and ordinary functions.

::: basic_format_string

:: {.doxygentypedef}
fmt::format_string
::

::: runtime(string_view)

<a id="udt"></a>
### Formatting User-Defined Types

The {fmt} library provides formatters for many standard C++ types.
See [`fmt/ranges.h`](#ranges-api) for ranges and tuples including standard
containers such as `std::vector`, [`fmt/chrono.h`](#chrono-api) for date and
time formatting and [`fmt/std.h`](#std-api) for other standard library types.

There are two ways to make a user-defined type formattable: providing a
`format_as` function or specializing the `formatter` struct template.

Use `format_as` if you want to make your type formattable as some other
type with the same format specifiers. The `format_as` function should
take an object of your type and return an object of a formattable type.
It should be defined in the same namespace as your type.

Example ([run](https://godbolt.org/z/nvME4arz8)):

    #include <fmt/format.h>

    namespace kevin_namespacy {
    enum class film {
      house_of_cards, american_beauty, se7en = 7
    };
    auto format_as(film f) { return fmt::underlying(f); }
    }

    int main() {
      fmt::print("{}\n", kevin_namespacy::film::se7en); // prints "7"
    }

Using specialization is more complex but gives you full control over
parsing and formatting. To use this method specialize the `formatter`
struct template for your type and implement `parse` and `format`
methods.

The recommended way of defining a formatter is by reusing an existing
one via inheritance or composition. This way you can support standard
format specifiers without implementing them yourself. For example:

    // color.h:
    #include <fmt/base.h>

    enum class color {red, green, blue};

    template <> struct fmt::formatter<color>: formatter<string_view> {
      // parse is inherited from formatter<string_view>.

      auto format(color c, format_context& ctx) const
        -> format_context::iterator;
    };

    // color.cc:
    #include "color.h"
    #include <fmt/format.h>

    auto fmt::formatter<color>::format(color c, format_context& ctx) const
        -> format_context::iterator {
      string_view name = "unknown";
      switch (c) {
      case color::red:   name = "red"; break;
      case color::green: name = "green"; break;
      case color::blue:  name = "blue"; break;
      }
      return formatter<string_view>::format(name, ctx);
    }

Note that `formatter<string_view>::format` is defined in `fmt/format.h`
so it has to be included in the source file. Since `parse` is inherited
from `formatter<string_view>` it will recognize all string format
specifications, for example

``` c++
fmt::format("{:>10}", color::blue)
```

will return `"      blue"`.

The experimental `nested_formatter` provides an easy way of applying a
formatter to one or more subobjects.

For example:

    #include <fmt/format.h>

    struct point {
      double x, y;
    };

    template <>
    struct fmt::formatter<point> : nested_formatter<double> {
      auto format(point p, format_context& ctx) const {
        return write_padded(ctx, [=](auto out) {
          return format_to(out, "({}, {})", this->nested(p.x),
                           this->nested(p.y));
        });
      }
    };

    int main() {
      fmt::print("[{:>20.2f}]", point{1, 2});
    }

prints:

    [          (1.00, 2.00)]

Notice that fill, align and width are applied to the whole object which
is the recommended behavior while the remaining specifiers apply to
elements.

In general the formatter has the following form:

    template <> struct fmt::formatter<T> {
      // Parses format specifiers and stores them in the formatter.
      //
      // [ctx.begin(), ctx.end()) is a, possibly empty, character range that
      // contains a part of the format string starting from the format
      // specifications to be parsed, e.g. in
      //
      //   fmt::format("{:f} continued", ...);
      //
      // the range will contain "f} continued". The formatter should parse
      // specifiers until '}' or the end of the range. In this example the
      // formatter should parse the 'f' specifier and return an iterator
      // pointing to '}'.
      constexpr auto parse(format_parse_context& ctx)
        -> format_parse_context::iterator;

      // Formats value using the parsed format specification stored in this
      // formatter and writes the output to ctx.out().
      auto format(const T& value, format_context& ctx) const
        -> format_context::iterator;
    };

It is recommended to at least support fill, align and width that apply
to the whole object and have the same semantics as in standard
formatters.

You can also write a formatter for a hierarchy of classes:

    // demo.h:
    #include <type_traits>
    #include <fmt/core.h>

    struct A {
      virtual ~A() {}
      virtual std::string name() const { return "A"; }
    };

    struct B : A {
      virtual std::string name() const { return "B"; }
    };

    template <typename T>
    struct fmt::formatter<T, std::enable_if_t<std::is_base_of<A, T>::value, char>> :
        fmt::formatter<std::string> {
      auto format(const A& a, format_context& ctx) const {
        return fmt::formatter<std::string>::format(a.name(), ctx);
      }
    };

    // demo.cc:
    #include "demo.h"
    #include <fmt/format.h>

    int main() {
      B b;
      A& a = b;
      fmt::print("{}", a); // prints "B"
    }

Providing both a `formatter` specialization and a `format_as` overload
is disallowed.

### Named Arguments

::: arg(const Char*, const T&)

Named arguments are not supported in compile-time checks at the moment.

### Argument Lists

You can create your own formatting function with compile-time checks and
small binary footprint, for example (<https://godbolt.org/z/vajfWEG4b>):

``` c++
#include <fmt/base.h>

void vlog(const char* file, int line, fmt::string_view format,
          fmt::format_args args) {
  fmt::print("{}: {}: ", file, line);
  fmt::vprint(format, args);
}

template <typename... T>
void log(const char* file, int line, fmt::format_string<T...> format, T&&... args) {
  vlog(file, line, format, fmt::make_format_args(args...));
}

#define MY_LOG(format, ...) log(__FILE__, __LINE__, format, __VA_ARGS__)

MY_LOG("invalid squishiness: {}", 42);
```

Note that `vlog` is not parameterized on argument types which improves
compile times and reduces binary code size compared to a fully
parameterized version.

::: make_format_args(T&...)

::: basic_format_args

:: {.doxygentypedef}
fmt::format_args
::

:: {.doxygenclass members=""}
fmt::basic_format_arg
::

:: {.doxygenclass members=""}
fmt::basic_format_parse_context
::

:: {.doxygenclass members=""}
fmt::context
::

:: {.doxygentypedef}
fmt::format_context
::

### Dynamic Argument Lists {#args-api}

The header `fmt/args.h` provides `dynamic_format_arg_store`, a
builder-like API that can be used to construct format argument lists
dynamically.

:: {.doxygenclass members=""}
fmt::dynamic_format_arg_store
::

### Compatibility

:: {.doxygenclass members=""}
fmt::basic_string_view
::

:: {.doxygentypedef}
fmt::string_view
::

## Format API

`fmt/format.h` defines the full format API providing additional
formatting functions and locale support.

<a id="format"></a>
::: format(format_string<T...>, T&&...)

::: vformat(string_view, format_args)

### Literal-Based API

The following user-defined literals are defined in `fmt/format.h`.

::: operator""_a()

### Utilities

::: ptr(T)

::: underlying(Enum)

::: to_string(const T&)

::: group_digits(T)

:: {.doxygenclass members=""}
fmt::detail::buffer
::

::: basic_memory_buffer

### System Errors

{fmt} does not use `errno` to communicate errors to the user, but it may
call system functions which set `errno`. Users should not make any
assumptions about the value of `errno` being preserved by library
functions.

::: system_error

::: format_system_error

### Custom Allocators

The {fmt} library supports custom dynamic memory allocators. A custom
allocator class can be specified as a template argument to
`fmt::basic_memory_buffer`{.interpreted-text role="class"}:

    using custom_memory_buffer = 
      fmt::basic_memory_buffer<char, fmt::inline_buffer_size, custom_allocator>;

It is also possible to write a formatting function that uses a custom
allocator:

    using custom_string =
      std::basic_string<char, std::char_traits<char>, custom_allocator>;

    custom_string vformat(custom_allocator alloc, fmt::string_view format_str,
                          fmt::format_args args) {
      auto buf = custom_memory_buffer(alloc);
      fmt::vformat_to(std::back_inserter(buf), format_str, args);
      return custom_string(buf.data(), buf.size(), alloc);
    }

    template <typename ...Args>
    inline custom_string format(custom_allocator alloc,
                                fmt::string_view format_str,
                                const Args& ... args) {
      return vformat(alloc, format_str, fmt::make_format_args(args...));
    }

The allocator will be used for the output container only. Formatting
functions normally don\'t do any allocations for built-in and string
types except for non-default floating-point formatting that occasionally
falls back on `sprintf`.

### Locale

All formatting is locale-independent by default. Use the `'L'` format
specifier to insert the appropriate number separator characters from the
locale:

    #include <fmt/core.h>
    #include <locale>

    std::locale::global(std::locale("en_US.UTF-8"));
    auto s = fmt::format("{:L}", 1000000);  // s == "1,000,000"

`fmt/format.h` provides the following overloads of formatting functions
that take `std::locale` as a parameter. The locale type is a template
parameter to avoid the expensive `<locale>` include.

::: format(const Locale&, format_string<T...>, T&&...)

::: format_to(OutputIt, const Locale&, format_string<T...>, T&&...)

::: formatted_size(const Locale&, format_string<T...>, T&&...)

### Legacy Compile-Time Format String Checks {#legacy-checks}

`FMT_STRING` enables compile-time checks on older compilers. It requires
C++14 or later and is a no-op in C++11.

:: {.doxygendefine}
FMT_STRING
::

To force the use of legacy compile-time checks, define the preprocessor
variable `FMT_ENFORCE_COMPILE_STRING`. When set, functions accepting
`FMT_STRING` will fail to compile with regular strings.

## Range and Tuple Formatting {#ranges-api}

The library also supports convenient formatting of ranges and tuples:

    #include <fmt/ranges.h>

    std::tuple<char, int, float> t{'a', 1, 2.0f};
    // Prints "('a', 1, 2.0)"
    fmt::print("{}", t);

Using `fmt::join`, you can separate tuple elements with a custom
separator:

    #include <fmt/ranges.h>

    std::tuple<int, char> t = {1, 'a'};
    // Prints "1, a"
    fmt::print("{}", fmt::join(t, ", "));

::: join(Range&&, string_view)

::: join(It, Sentinel, string_view)

## Date and Time Formatting {#chrono-api}

`fmt/chrono.h` provides formatters for

- [std::chrono::duration](https://en.cppreference.com/w/cpp/chrono/duration)
- [std::chrono::time_point](https://en.cppreference.com/w/cpp/chrono/time_point)
- [std::tm](https://en.cppreference.com/w/cpp/chrono/c/tm)

The format syntax is described in [Chrono Format Specifications](syntax.md#
chrono-format-specifications).

**Example**:

    #include <fmt/chrono.h>

    int main() {
      std::time_t t = std::time(nullptr);

      // Prints "The date is 2020-11-07." (with the current date):
      fmt::print("The date is {:%Y-%m-%d}.", fmt::localtime(t));

      using namespace std::literals::chrono_literals;

      // Prints "Default format: 42s 100ms":
      fmt::print("Default format: {} {}\n", 42s, 100ms);

      // Prints "strftime-like format: 03:15:30":
      fmt::print("strftime-like format: {:%H:%M:%S}\n", 3h + 15min + 30s);
    }

::: localtime(std::time_t)

::: gmtime(std::time_t)

## Standard Library Types Formatting {#std-api}

`fmt/std.h` provides formatters for:

- [`std::atomic`](https://en.cppreference.com/w/cpp/atomic/atomic)
- [`std::atomic_flag`](https://en.cppreference.com/w/cpp/atomic/atomic_flag)
- [`std::bitset`](https://en.cppreference.com/w/cpp/utility/bitset)
- [`std::error_code`](https://en.cppreference.com/w/cpp/error/error_code)
- [`std::filesystem::path`](https://en.cppreference.com/w/cpp/filesystem/path)
- [`std::monostate`](https://en.cppreference.com/w/cpp/utility/variant/monostate)
- [`std::optional`](https://en.cppreference.com/w/cpp/utility/optional)
- [`std::source_location`](https://en.cppreference.com/w/cpp/utility/source_location)
- [`std::thread::id`](https://en.cppreference.com/w/cpp/thread/thread/id)
- [`std::variant`](https://en.cppreference.com/w/cpp/utility/variant/variant)

::: ptr(const std::unique_ptr<T, Deleter>&)

::: ptr(const std::shared_ptr<T>&)

### Formatting Variants

A `std::variant` is only formattable if every variant alternative is
formattable, and requires the `__cpp_lib_variant` [library
feature](https://en.cppreference.com/w/cpp/feature_test).

**Example**:

    #include <fmt/std.h>

    std::variant<char, float> v0{'x'};
    // Prints "variant('x')"
    fmt::print("{}", v0);

    std::variant<std::monostate, char> v1;
    // Prints "variant(monostate)"

## Format String Compilation {#compile-api}

`fmt/compile.h` provides format string compilation enabled via the
`FMT_COMPILE` macro or the `_cf` user-defined literal. Format strings
marked with `FMT_COMPILE` or `_cf` are parsed, checked and converted
into efficient formatting code at compile-time. This supports arguments
of built-in and string types as well as user-defined types with `format`
functions taking the format context type as a template parameter in
their `formatter` specializations. For example:

    template <> struct fmt::formatter<point> {
      constexpr auto parse(format_parse_context& ctx);

      template <typename FormatContext>
      auto format(const point& p, FormatContext& ctx) const;
    };

Format string compilation can generate more binary code compared to the
default API and is only recommended in places where formatting is a
performance bottleneck.

:: {.doxygendefine}
FMT_COMPILE
::

::: operator""_cf

## Terminal Color and Text Style {#color-api}

`fmt/color.h` provides support for terminal color and text style output.

::: print(const text_style&, format_string<T...>, T&&...)

::: fg(detail::color_type)

:: {.doxygenfunction}
bg(detail::color_type)
::

:: {.doxygenfunction}
styled(const T& value, text_style ts)
::

## System APIs {#os-api}

:: {.doxygenclass members=""}
fmt::ostream
::

:: {.doxygenfunction}
fmt::windows_error
::

## `std::ostream` Support {#ostream-api}

`fmt/ostream.h` provides `std::ostream` support including formatting of
user-defined types that have an overloaded insertion operator
(`operator<<`). In order to make a type formattable via `std::ostream`
you should provide a `formatter` specialization inherited from
`ostream_formatter`:

    #include <fmt/ostream.h>

    struct date {
      int year, month, day;

      friend std::ostream& operator<<(std::ostream& os, const date& d) {
        return os << d.year << '-' << d.month << '-' << d.day;
      }
    };

    template <> struct fmt::formatter<date> : ostream_formatter {};

    std::string s = fmt::format("The date is {}", date{2012, 12, 9});
    // s == "The date is 2012-12-9"

:: {.doxygenfunction}
streamed(const T &)
::

:: {.doxygenfunction}
print(std::ostream &os, format_string\<T\...\> fmt, T&&\... args)
::

## `printf` Formatting {#printf-api}

The header `fmt/printf.h` provides `printf`-like formatting
functionality. The following functions use [printf format string
syntax](https://pubs.opengroup.org/onlinepubs/009695399/functions/fprintf.html)
with the POSIX extension for positional arguments. Unlike their standard
counterparts, the `fmt` functions are type-safe and throw an exception
if an argument type doesn\'t match its format specification.

:: {.doxygenfunction}
printf(string_view fmt, const T&\... args) -\> int
::

:: {.doxygenfunction}
fprintf(std::FILE \*f, const S &fmt, const T&\... args) -\> int
::

:: {.doxygenfunction}
sprintf(const S&, const T&\...)
::

## `wchar_t` Support {#xchar-api}

The optional header `fmt/xchar.h` provides support for `wchar_t` and
exotic character types.

:: {.doxygenstruct}
fmt::is_char
::

:: {.doxygentypedef}
fmt::wstring_view
::

:: {.doxygentypedef}
fmt::wformat_context
::

:: {.doxygenfunction}
fmt::to_wstring(const T &value)
::

## Compatibility with C++20 `std::format`

{fmt} implements nearly all of the [C++20 formatting
library](https://en.cppreference.com/w/cpp/utility/format) with the
following differences:

- Names are defined in the `fmt` namespace instead of `std` to avoid
  collisions with standard library implementations.
- Width calculation doesn\'t use grapheme clusterization. The latter has
  been implemented in a separate branch but hasn\'t been integrated yet.
