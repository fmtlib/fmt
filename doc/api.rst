.. _string-formatting-api:

*************
API Reference
*************

The {fmt} library API consists of the following parts:

* :ref:`fmt/core.h <core-api>`: the core API providing argument handling
  facilities and a lightweight subset of formatting functions
* :ref:`fmt/format.h <format-api>`: the full format API providing compile-time
  format string checks, output iterator and user-defined type support
* :ref:`fmt/ranges.h <ranges-api>`: additional formatting support for ranges
  and tuples
* :ref:`fmt/chrono.h <chrono-api>`: date and time formatting
* :ref:`fmt/ostream.h <ostream-api>`: ``std::ostream`` support
* :ref:`fmt/printf.h <printf-api>`: ``printf`` formatting

All functions and types provided by the library reside in namespace ``fmt`` and
macros have prefix ``FMT_``.

.. _core-api:

Core API
========

``fmt/core.h`` defines the core API which provides argument handling facilities
and a lightweight subset of formatting functions. In the header-only mode
include ``fmt/format.h`` instead of ``fmt/core.h``.

The following functions use :ref:`format string syntax <syntax>`
similar to that of Python's `str.format
<http://docs.python.org/3/library/stdtypes.html#str.format>`_.
They take *format_str* and *args* as arguments.

*format_str* is a format string that contains literal text and replacement
fields surrounded by braces ``{}``. The fields are replaced with formatted
arguments in the resulting string. A function taking *format_str* doesn't
participate in an overload resolution if the latter is not a string.

*args* is an argument list representing objects to be formatted.

.. _format:

.. doxygenfunction:: format(const S&, Args&&...)
.. doxygenfunction:: vformat(const S&, basic_format_args<buffer_context<Char>>)

.. _print:

.. doxygenfunction:: print(const S&, Args&&...)
.. doxygenfunction:: vprint(string_view, format_args)

.. doxygenfunction:: print(std::FILE *, const S&, Args&&...)
.. doxygenfunction:: vprint(std::FILE *, string_view, format_args)

Named Arguments
---------------

.. doxygenfunction:: fmt::arg(const S&, const T&)

Named arguments are not supported in compile-time checks at the moment.

Argument Lists
--------------

.. doxygenfunction:: fmt::make_format_args(const Args&...)

.. doxygenclass:: fmt::format_arg_store
   :members:

.. doxygenclass:: fmt::basic_format_args
   :members:

.. doxygenstruct:: fmt::format_args

.. doxygenclass:: fmt::basic_format_arg
   :members:

Compatibility
-------------

.. doxygenclass:: fmt::basic_string_view
   :members:

.. doxygentypedef:: fmt::string_view
.. doxygentypedef:: fmt::wstring_view

.. _format-api:

Format API
==========

``fmt/format.h`` defines the full format API providing compile-time format
string checks, output iterator and user-defined type support.

Compile-time Format String Checks
---------------------------------

Compile-time checks are supported for built-in and string types as well as
user-defined types with ``constexpr`` ``parse`` functions in their ``formatter``
specializations.

.. doxygendefine:: FMT_STRING

Formatting User-defined Types
-----------------------------

To make a user-defined type formattable, specialize the ``formatter<T>`` struct
template and implement ``parse`` and ``format`` methods::

  #include <fmt/format.h>

  struct point { double x, y; };

  template <>
  struct fmt::formatter<point> {
    // Presentation format: 'f' - fixed, 'e' - exponential.
    char presentation = 'f';

    // Parses format specifications of the form ['f' | 'e'].
    constexpr auto parse(format_parse_context& ctx) {
      // [ctx.begin(), ctx.end()) is a character range that contains a part of
      // the format string starting from the format specifications to be parsed,
      // e.g. in
      //
      //   fmt::format("{:f} - point of interest", point{1, 2});
      //
      // the range will contain "f} - point of interest". The formatter should
      // parse specifiers until '}' or the end of the range. In this example
      // the formatter should parse the 'f' specifier and return an iterator
      // pointing to '}'.

      // Parse the presentation format and store it in the formatter:
      auto it = ctx.begin(), end = ctx.end();
      if (it != end && (*it == 'f' || *it == 'e')) presentation = *it++;

      // Check if reached the end of the range:
      if (it != end && *it != '}')
        throw format_error("invalid format");

      // Return an iterator past the end of the parsed range:
      return it;
    }

    // Formats the point p using the parsed format specification (presentation)
    // stored in this formatter.
    template <typename FormatContext>
    auto format(const point& p, FormatContext& ctx) {
      // ctx.out() is an output iterator to write to.
      return format_to(
          ctx.out(),
          presentation == 'f' ? "({:.1f}, {:.1f})" : "({:.1e}, {:.1e})",
          p.x, p.y);
    }
  };

Then you can pass objects of type ``point`` to any formatting function::

  point p = {1, 2};
  std::string s = fmt::format("{:f}", p);
  // s == "(1.0, 2.0)"

You can also reuse existing formatters via inheritance or composition, for
example::

  enum class color {red, green, blue};

  template <>
  struct fmt::formatter<color>: formatter<string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(color c, FormatContext& ctx) {
      string_view name = "unknown";
      switch (c) {
      case color::red:   name = "red"; break;
      case color::green: name = "green"; break;
      case color::blue:  name = "blue"; break;
      }
      return formatter<string_view>::format(name, ctx);
    }
  };

You can also write a formatter for a hierarchy of classes::

  #include <type_traits>
  #include <fmt/format.h>

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
    template <typename FormatCtx>
    auto format(const A& a, FormatCtx& ctx) {
      return fmt::formatter<std::string>::format(a.name(), ctx);
    }
  };

  int main() {
    B b;
    A& a = b;
    fmt::print("{}", a); // prints "B"
  }

.. doxygenclass:: fmt::basic_format_parse_context
   :members:

Output Iterator Support
-----------------------

.. doxygenfunction:: fmt::format_to(OutputIt, const S&, Args&&...)
.. doxygenfunction:: fmt::format_to_n(OutputIt, std::size_t, string_view, Args&&...)
.. doxygenstruct:: fmt::format_to_n_result
   :members:

Literal-based API
-----------------

The following user-defined literals are defined in ``fmt/format.h``.

.. doxygenfunction:: operator""_format(const char *, std::size_t)

.. doxygenfunction:: operator""_a(const char *, std::size_t)

Utilities
---------

.. doxygenstruct:: fmt::is_char

.. doxygentypedef:: fmt::char_t

.. doxygenfunction:: fmt::formatted_size(string_view, const Args&...)

.. doxygenfunction:: fmt::to_string(const T&)

.. doxygenfunction:: fmt::to_wstring(const T&)

.. doxygenfunction:: fmt::to_string_view(const Char *)

.. doxygenfunction:: fmt::join(const Range&, string_view)

.. doxygenfunction:: fmt::join(It, It, string_view)

.. doxygenclass:: fmt::basic_memory_buffer
   :protected-members:
   :members:

System Errors
-------------

fmt does not use ``errno`` to communicate errors to the user, but it may call
system functions which set ``errno``. Users should not make any assumptions about
the value of ``errno`` being preserved by library functions.

.. doxygenclass:: fmt::system_error
   :members:

.. doxygenfunction:: fmt::format_system_error

.. doxygenclass:: fmt::windows_error
   :members:

.. _formatstrings:

Custom Allocators
-----------------

The {fmt} library supports custom dynamic memory allocators.
A custom allocator class can be specified as a template argument to
:class:`fmt::basic_memory_buffer`::

    using custom_memory_buffer = 
      fmt::basic_memory_buffer<char, fmt::inline_buffer_size, custom_allocator>;

It is also possible to write a formatting function that uses a custom
allocator::

    using custom_string =
      std::basic_string<char, std::char_traits<char>, custom_allocator>;

    custom_string vformat(custom_allocator alloc, fmt::string_view format_str,
                          fmt::format_args args) {
      custom_memory_buffer buf(alloc);
      fmt::vformat_to(buf, format_str, args);
      return custom_string(buf.data(), buf.size(), alloc);
    }

    template <typename ...Args>
    inline custom_string format(custom_allocator alloc,
                                fmt::string_view format_str,
                                const Args& ... args) {
      return vformat(alloc, format_str, fmt::make_format_args(args...));
    }

The allocator will be used for the output container only. If you are using named
arguments, the container that stores pointers to them will be allocated using
the default allocator. Also floating-point formatting falls back on ``sprintf``
which may do allocations.

Custom Formatting of Built-in Types
-----------------------------------

It is possible to change the way arguments are formatted by providing a
custom argument formatter class::

  using arg_formatter = fmt::arg_formatter<fmt::buffer_range<char>>;

  // A custom argument formatter that formats negative integers as unsigned
  // with the ``x`` format specifier.
  class custom_arg_formatter : public arg_formatter {
   public:
    custom_arg_formatter(fmt::format_context& ctx,
                         fmt::format_parse_context* parse_ctx = nullptr,
                         fmt::format_specs* spec = nullptr)
      : arg_formatter(ctx, parse_ctx, spec) {}

    using arg_formatter::operator();

    auto operator()(int value) {
      if (specs() && specs()->type == 'x')
        return (*this)(static_cast<unsigned>(value)); // convert to unsigned and format
      return arg_formatter::operator()(value);
    }
  };

  std::string custom_vformat(fmt::string_view format_str, fmt::format_args args) {
    fmt::memory_buffer buffer;
    // Pass custom argument formatter as a template arg to vformat_to.
    fmt::vformat_to<custom_arg_formatter>(buffer, format_str, args);
    return fmt::to_string(buffer);
  }

  template <typename ...Args>
  inline std::string custom_format(
      fmt::string_view format_str, const Args&... args) {
    return custom_vformat(format_str, fmt::make_format_args(args...));
  }

  std::string s = custom_format("{:x}", -42); // s == "ffffffd6"

.. doxygenclass:: fmt::arg_formatter
   :members:

.. _ranges-api:

Ranges and Tuple Formatting
===========================

The library also supports convenient formatting of ranges and tuples::

  #include <fmt/ranges.h>

  std::tuple<char, int, float> t{'a', 1, 2.0f};
  // Prints "('a', 1, 2.0)"
  fmt::print("{}", t);


NOTE: currently, the overload of ``fmt::join`` for iterables exists in the main
``format.h`` header, but expect this to change in the future.

Using ``fmt::join``, you can separate tuple elements with a custom separator::

  #include <fmt/ranges.h>

  std::tuple<int, char> t = {1, 'a'};
  // Prints "1, a"
  fmt::print("{}", fmt::join(t, ", "));

.. _chrono-api:

Date and Time Formatting
========================

The library supports `strftime
<http://en.cppreference.com/w/cpp/chrono/c/strftime>`_-like date and time
formatting::

  #include <fmt/chrono.h>

  std::time_t t = std::time(nullptr);
  // Prints "The date is 2016-04-29." (with the current date)
  fmt::print("The date is {:%Y-%m-%d}.", *std::localtime(&t));

The format string syntax is described in the documentation of
`strftime <http://en.cppreference.com/w/cpp/chrono/c/strftime>`_.

.. _ostream-api:

``std::ostream`` Support
========================

``fmt/ostream.h`` provides ``std::ostream`` support including formatting of
user-defined types that have overloaded ``operator<<``::

  #include <fmt/ostream.h>

  class date {
    int year_, month_, day_;
  public:
    date(int year, int month, int day): year_(year), month_(month), day_(day) {}

    friend std::ostream& operator<<(std::ostream& os, const date& d) {
      return os << d.year_ << '-' << d.month_ << '-' << d.day_;
    }
  };

  std::string s = fmt::format("The date is {}", date(2012, 12, 9));
  // s == "The date is 2012-12-9"

.. doxygenfunction:: print(std::basic_ostream<Char>&, const S&, Args&&...)

.. _printf-api:

``printf`` Formatting
=====================

The header ``fmt/printf.h`` provides ``printf``-like formatting functionality.
The following functions use `printf format string syntax
<http://pubs.opengroup.org/onlinepubs/009695399/functions/fprintf.html>`_ with
the POSIX extension for positional arguments. Unlike their standard
counterparts, the ``fmt`` functions are type-safe and throw an exception if an
argument type doesn't match its format specification.

.. doxygenfunction:: printf(const S&, const Args&...)

.. doxygenfunction:: fprintf(std::FILE *, const S&, const Args&...)

.. doxygenfunction:: fprintf(std::basic_ostream<Char>&, const S&, const Args&...)

.. doxygenfunction:: sprintf(const S&, const Args&...)
