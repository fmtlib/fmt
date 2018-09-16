.. _string-formatting-api:

*************
API Reference
*************

The {fmt} library API consists of the following parts:

* :ref:`fmt/core.h <core-api>`: the core API providing argument handling
  facilities and a lightweight subset of formatting functions
* :ref:`fmt/format.h <format-api>`: the full format API providing compile-time
  format string checks, output iterator and user-defined type support
* :ref:`fmt/time.h <time-api>`: date and time formatting
* :ref:`fmt/ostream.h <ostream-api>`: ``std::ostream`` support
* :ref:`fmt/printf.h <printf-api>`: ``printf`` formatting

All functions and types provided by the library reside in namespace ``fmt`` and
macros have prefix ``FMT_`` or ``fmt``.

.. _core-api:

Core API
========

``fmt/core.h`` defines the core API which provides argument handling facilities
and a lightweight subset of formatting functions.

The following functions use :ref:`format string syntax <syntax>`
imilar to that of Python's `str.format
<http://docs.python.org/3/library/stdtypes.html#str.format>`_.
They take *format_str* and *args* as arguments.

*format_str* is a format string that contains literal text and replacement
fields surrounded by braces ``{}``. The fields are replaced with formatted
arguments in the resulting string.

*args* is an argument list representing objects to be formatted.

.. _format:

.. doxygenfunction:: format(const String&, const Args&...)
.. doxygenfunction:: vformat(const String&, basic_format_args<typename buffer_context<Char>::type>)

.. _print:

.. doxygenfunction:: print(string_view, const Args&...)
.. doxygenfunction:: vprint(string_view, format_args)

.. doxygenfunction:: print(std::FILE *, string_view, const Args&...)
.. doxygenfunction:: vprint(std::FILE *, string_view, format_args)

.. doxygenfunction:: print(std::FILE *, wstring_view, const Args&...)
.. doxygenfunction:: vprint(std::FILE *, wstring_view, wformat_args)

Named arguments
---------------

.. doxygenfunction:: fmt::arg(string_view, const T&)

Argument lists
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

Compile-time format string checks
---------------------------------

.. doxygendefine:: fmt

Formatting user-defined types
-----------------------------

To make a user-defined type formattable, specialize the ``formatter<T>`` struct
template and implement ``parse`` and ``format`` methods::

  #include <fmt/format.h>

  struct point { double x, y; };

  namespace fmt {
  template <>
  struct formatter<point> {
    template <typename ParseContext>
    constexpr auto parse(ParseContext &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const point &p, FormatContext &ctx) {
      return format_to(ctx.begin(), "({:.1f}, {:.1f})", p.x, p.y);
    }
  };
  }

Then you can pass objects of type ``point`` to any formatting function::

  point p = {1, 2};
  std::string s = fmt::format("{}", p);
  // s == "(1.0, 2.0)"

In the example above the ``formatter<point>::parse`` function ignores the
contents of the format string referred to by ``ctx.begin()`` so the object will
always be formatted in the same way. See ``formatter<tm>::parse`` in
:file:`fmt/time.h` for an advanced example of how to parse the format string and
customize the formatted output.

You can also reuse existing formatters, for example::

  enum class color {red, green, blue};

  template <>
  struct fmt::formatter<color>: formatter<string_view> {
    // parse is inherited from formatter<string_view>.
    template <typename FormatContext>
    auto format(color c, FormatContext &ctx) {
      string_view name = "unknown";
      switch (c) {
      case color::red:   name = "red"; break;
      case color::green: name = "green"; break;
      case color::blue:  name = "blue"; break;
      }
      return formatter<string_view>::format(name, ctx);
    }
  };

This section shows how to define a custom format function for a user-defined
type. The next section describes how to get ``fmt`` to use a conventional stream
output ``operator<<`` when one is defined for a user-defined type.

Output iterator support
-----------------------

.. doxygenfunction:: fmt::format_to(OutputIt, string_view, const Args&...)
.. doxygenfunction:: fmt::format_to_n(OutputIt, std::size_t, string_view, const Args&...)
.. doxygenstruct:: fmt::format_to_n_result
   :members:

Literal-based API
-----------------

The following user-defined literals are defined in ``fmt/format.h``.

.. doxygenfunction:: operator""_format(const char *, std::size_t)

.. doxygenfunction:: operator""_a(const char *, std::size_t)

Utilities
---------

.. doxygenfunction:: fmt::formatted_size(string_view, const Args&...)

.. doxygenfunction:: fmt::to_string(const T&)

.. doxygenfunction:: fmt::to_wstring(const T&)

.. doxygenclass:: fmt::basic_memory_buffer
   :protected-members:
   :members:

System errors
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

Custom allocators
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
                                const Args & ... args) {
      return vformat(alloc, format_str, fmt::make_format_args(args...));
    }

The allocator will be used for the output container only. If you are using named
arguments, the container that stores pointers to them will be allocated using
the default allocator. Also floating-point formatting falls back on ``sprintf``
which may do allocations.

Custom formatting of built-in types
-----------------------------------

It is possible to change the way arguments are formatted by providing a
custom argument formatter class::

  using arg_formatter =
    fmt::arg_formatter<fmt::back_insert_range<fmt::internal::buffer>>;

  // A custom argument formatter that formats negative integers as unsigned
  // with the ``x`` format specifier.
  class custom_arg_formatter : public arg_formatter {
   public:
    custom_arg_formatter(fmt::format_context &ctx, fmt::format_specs &spec)
      : arg_formatter(ctx, spec) {}

    using arg_formatter::operator();

    auto operator()(int value) {
      if (spec().type() == 'x')
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
      fmt::string_view format_str, const Args &... args) {
    return custom_vformat(format_str, fmt::make_format_args(args...));
  }

  std::string s = custom_format("{:x}", -42); // s == "ffffffd6"

.. doxygenclass:: fmt::arg_formatter
   :members:

.. _time-api:

Date and time formatting
========================

The library supports `strftime
<http://en.cppreference.com/w/cpp/chrono/c/strftime>`_-like date and time
formatting::

  #include <fmt/time.h>

  std::time_t t = std::time(nullptr);
  // Prints "The date is 2016-04-29." (with the current date)
  fmt::print("The date is {:%Y-%m-%d}.", *std::localtime(&t));

The format string syntax is described in the documentation of
`strftime <http://en.cppreference.com/w/cpp/chrono/c/strftime>`_.

.. _ostream-api:

``std::ostream`` support
========================

``fmt/ostream.h`` provides ``std::ostream`` support including formatting of
user-defined types that have overloaded ``operator<<``::

  #include <fmt/ostream.h>

  class date {
    int year_, month_, day_;
  public:
    date(int year, int month, int day): year_(year), month_(month), day_(day) {}

    friend std::ostream &operator<<(std::ostream &os, const date &d) {
      return os << d.year_ << '-' << d.month_ << '-' << d.day_;
    }
  };

  std::string s = fmt::format("The date is {}", date(2012, 12, 9));
  // s == "The date is 2012-12-9"

.. doxygenfunction:: print(std::ostream&, string_view, const Args&...)

.. _printf-api:

``printf`` formatting
=====================

The header ``fmt/printf.h`` provides ``printf``-like formatting functionality.
The following functions use `printf format string syntax
<http://pubs.opengroup.org/onlinepubs/009695399/functions/fprintf.html>`_ with
the POSIX extension for positional arguments. Unlike their standard
counterparts, the ``fmt`` functions are type-safe and throw an exception if an
argument type doesn't match its format specification.

.. doxygenfunction:: printf(string_view, const Args&...)

.. doxygenfunction:: fprintf(std::FILE *, string_view, const Args&...)

.. doxygenfunction:: fprintf(std::ostream&, string_view, const Args&...)

.. doxygenfunction:: sprintf(string_view, const Args&...)
