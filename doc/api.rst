.. _string-formatting-api:

*************
API Reference
*************

All functions and classes provided by the fmt library reside in namespace
``fmt`` and macros have prefix ``FMT_``. For brevity the namespace is usually
omitted in examples.

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

.. doxygenfunction:: operator""_format(const char *, std::size_t)

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

  struct MyStruct { double x, y; };

  namespace fmt {
  template <>
  struct formatter<MyStruct> {
    template <typename ParseContext>
    auto parse(ParseContext &ctx) { return ctx.begin(); }

    template <typename FormatContext>
    auto format(const MyStruct &s, FormatContext &ctx) {
      return format_to(ctx.begin(), "[MyStruct: x={:.1f}, y={:.2f}]", s.x, s.y);
    }
  };
  }

Then you can pass objects of type ``MyStruct`` to any formatting function::

  MyStruct m = {1, 2};
  std::string s = fmt::format("m={}", m);
  // s == "m=[MyStruct: x=1.0, y=2.00]"

In the example above the ``formatter<MyStruct>::parse`` function ignores the
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

  class Date {
    int year_, month_, day_;
  public:
    Date(int year, int month, int day): year_(year), month_(month), day_(day) {}

    friend std::ostream &operator<<(std::ostream &os, const Date &d) {
      return os << d.year_ << '-' << d.month_ << '-' << d.day_;
    }
  };

  std::string s = fmt::format("The date is {}", Date(2012, 12, 9));
  // s == "The date is 2012-12-9"

.. doxygenfunction:: print(std::ostream&, string_view, const Args&...)

Argument formatters
-------------------

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
`~fmt::BasicMemoryWriter` which stores its output in a memory buffer and
provides direct access to it. It is possible to create custom writers that
store output elsewhere by subclassing `~fmt::BasicWriter`.

.. doxygenclass:: fmt::BasicWriter
   :members:

.. doxygenclass:: fmt::BasicMemoryWriter
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

.. doxygenfunction:: operator""_a(const char *, std::size_t)

.. doxygendefine:: FMT_CAPTURE

.. doxygenclass:: fmt::basic_format_args
   :members:

.. doxygenfunction:: fmt::to_string(const T&)

.. doxygenclass:: fmt::basic_string_view
   :members:

.. doxygenclass:: fmt::Buffer
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

The fmt library supports custom dynamic memory allocators.
A custom allocator class can be specified as a template argument to
:class:`fmt::BasicMemoryWriter`::

    typedef fmt::BasicMemoryWriter<char, CustomAllocator> CustomMemoryWriter;

It is also possible to write a formatting function that uses a custom
allocator::

    typedef std::basic_string<char, std::char_traits<char>, CustomAllocator>
            CustomString;

    CustomString format(CustomAllocator alloc, fmt::CStringRef format_str,
                        fmt::ArgList args) {
      CustomMemoryWriter writer(alloc);
      writer.write(format_str, args);
      return CustomString(writer.data(), writer.size(), alloc);
    }
    FMT_VARIADIC(CustomString, format, CustomAllocator, fmt::CStringRef)
