.. _string-formatting-api:

*************
API Reference
*************

All functions and classes provided by the {fmt} library reside in namespace
``fmt`` and macros have prefix ``FMT_``.

Format API
==========

The following functions defined in ``fmt/core.h`` use :ref:`format string
syntax <syntax>` similar to that of Python's `str.format
<http://docs.python.org/3/library/stdtypes.html#str.format>`_.
They take *format_str* and *args* as arguments.

*format_str* is a format string that contains literal text and replacement
fields surrounded by braces ``{}``. The fields are replaced with formatted
arguments in the resulting string.

*args* is an argument list representing objects to be formatted.

The `performance of the formating functions
<https://github.com/fmtlib/fmt/blob/master/README.rst#speed-tests>`_ is close
to that of glibc's ``printf`` and better than the performance of IOStreams.

.. _format:

.. doxygenfunction:: format(string_view, const Args&...)

.. _print:

.. doxygenfunction:: print(string_view, const Args&...)

.. doxygenfunction:: print(std::FILE *, string_view, const Args&...)

Date and time formatting
------------------------

The library supports `strftime
<http://en.cppreference.com/w/cpp/chrono/c/strftime>`_-like date and time
formatting::

  #include "fmt/time.h"

  std::time_t t = std::time(nullptr);
  // Prints "The date is 2016-04-29." (with the current date)
  fmt::print("The date is {:%Y-%m-%d}.", *std::localtime(&t));

The format string syntax is described in the documentation of
`strftime <http://en.cppreference.com/w/cpp/chrono/c/strftime>`_.

Formatting user-defined types
-----------------------------

To make a user-defined type formattable, specialize the ``formatter<T>`` struct
template and implement ``parse`` and ``format`` methods::

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

This section shows how to define a custom format function for a user-defined
type. The next section describes how to get ``fmt`` to use a conventional stream
output ``operator<<`` when one is defined for a user-defined type.

``std::ostream`` support
------------------------

The header ``fmt/ostream.h`` provides ``std::ostream`` support including
formatting of user-defined types that have overloaded ``operator<<``::

  #include "fmt/ostream.h"

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

Literal-based API
-----------------

The following user-defined literals are defined in ``fmt/format.h``.

.. doxygenfunction:: operator""_format(const char *, std::size_t)

.. doxygenfunction:: operator""_a(const char *, std::size_t)

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
    custom_arg_formatter(fmt::context &ctx, fmt::format_specs &spec)
      : arg_formatter(ctx, spec) {}

    using arg_formatter::operator();

    void operator()(int value) {
      if (spec().type() == 'x')
        (*this)(static_cast<unsigned>(value)); // convert to unsigned and format
      else
        arg_formatter::operator()(value);
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
    return custom_vformat(format_str, fmt::make_args(args...));
  }

  std::string s = custom_format("{:x}", -42); // s == "ffffffd6"

.. doxygenclass:: fmt::ArgVisitor
   :members:

.. doxygenclass:: fmt::arg_formatter_base
   :members:

.. doxygenclass:: fmt::arg_formatter
   :members:

Printf formatting
-----------------

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

Write API
=========

The write API provides classes for writing formatted data into character
streams. It is usually faster than the `format API`_ but, as IOStreams,
may result in larger compiled code size. The main writer class is
`~fmt::basic_memory_writer` which stores its output in a memory buffer and
provides direct access to it. It is possible to create custom writers that
store output elsewhere by subclassing `~fmt::BasicWriter`.

.. doxygenclass:: fmt::BasicWriter
   :members:

.. doxygenclass:: fmt::basic_memory_writer
   :members:

.. doxygenclass:: fmt::BasicArrayWriter
   :members:

.. doxygenclass:: fmt::BasicStringWriter
   :members:

.. doxygenfunction:: bin(int)

.. doxygenfunction:: oct(int)

.. doxygenfunction:: hex(int)

.. doxygenfunction:: hexu(int)

.. doxygenfunction:: pad(int, unsigned, Char)

Utilities
=========

.. doxygenfunction:: fmt::arg(string_view, const T&)

.. doxygenclass:: fmt::basic_format_args
   :members:

.. doxygenfunction:: fmt::to_string(const T&)

.. doxygenclass:: fmt::basic_string_view
   :members:

.. doxygenclass:: fmt::basic_memory_buffer
   :protected-members:
   :members:

System errors
=============

.. doxygenclass:: fmt::system_error
   :members:

.. doxygenfunction:: fmt::format_system_error

.. doxygenclass:: fmt::windows_error
   :members:

.. _formatstrings:

Custom allocators
=================

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
      return vformat(alloc, format_str, fmt::make_args(args...));
    }
