.. _string-formatting-api:

*************
API Reference
*************

All functions and classes provided by the fmt library reside
in namespace ``fmt`` and macros have prefix ``FMT_``. For brevity the
namespace is usually omitted in examples.

Format API
==========

The following functions use :ref:`format string syntax <syntax>` similar
to the one used by Python's `str.format
<http://docs.python.org/3/library/stdtypes.html#str.format>`_ function.
They take *format_str* and *args* as arguments.

*format_str* is a format string that contains literal text and replacement
fields surrounded by braces ``{}``. The fields are replaced with formatted
arguments in the resulting string.

*args* is an argument list representing arbitrary arguments.

The `performance of the format API
<https://github.com/fmtlib/fmt/blob/master/README.rst#speed-tests>`_ is close
to that of glibc's ``printf`` and better than the performance of IOStreams.
For even better speed use the `write API`_.

.. _format:

.. doxygenfunction:: format(CStringRef, ArgList)

.. doxygenfunction:: operator""_format(const char *, std::size_t)

.. _print:

.. doxygenfunction:: print(CStringRef, ArgList)

.. doxygenfunction:: print(std::FILE *, CStringRef, ArgList)

.. doxygenclass:: fmt::BasicFormatter
   :members:

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

A custom ``format_arg`` function may be implemented and used to format any
user-defined type. That is how date and time formatting described in the
previous section is implemented in :file:`fmt/time.h`. The following example
shows how to implement custom formatting for a user-defined structure.

::

  struct MyStruct { double a, b; };

  void format_arg(fmt::BasicFormatter<char> &f,
    const char *&format_str, const MyStruct &s) {
    f.writer().write("[MyStruct: a={:.1f}, b={:.2f}]", s.a, s.b);
  }

  MyStruct m = { 1, 2 };
  std::string s = fmt::format("m={}", n);
  // s == "m=[MyStruct: a=1.0, b=2.00]"

Note in the example above the ``format_arg`` function ignores the contents of
``format_str`` so the type will always be formatted as specified. See
``format_arg`` in :file:`fmt/time.h` for an advanced example of how to use
the ``format_str`` argument to customize the formatted output.

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

.. doxygenfunction:: print(std::ostream&, CStringRef, ArgList)

Argument formatters
-------------------

It is possible to change the way arguments are formatted by providing a
custom argument formatter class::

  // A custom argument formatter that formats negative integers as unsigned
  // with the ``x`` format specifier.
  class CustomArgFormatter :
    public fmt::BasicArgFormatter<CustomArgFormatter, char>  {
    public:
    CustomArgFormatter(fmt::BasicFormatter<char, CustomArgFormatter> &f,
                       fmt::FormatSpec &s, const char *fmt)
      : fmt::BasicArgFormatter<CustomArgFormatter, char>(f, s, fmt) {}

    void visit_int(int value) {
      if (spec().type() == 'x')
        visit_uint(value); // convert to unsigned and format
      else
        fmt::BasicArgFormatter<CustomArgFormatter, char>::visit_int(value);
    }
  };

  std::string custom_format(const char *format_str, fmt::ArgList args) {
    fmt::MemoryWriter writer;
    // Pass custom argument formatter as a template arg to BasicFormatter.
    fmt::BasicFormatter<char, CustomArgFormatter> formatter(args, writer);
    formatter.format(format_str);
    return writer.str();
  }
  FMT_VARIADIC(std::string, custom_format, const char *)

  std::string s = custom_format("{:x}", -42); // s == "ffffffd6"

.. doxygenclass:: fmt::ArgVisitor
   :members:

.. doxygenclass:: fmt::BasicArgFormatter
   :members:

.. doxygenclass:: fmt::ArgFormatter
   :members:

Printf formatting
-----------------

The header ``fmt/printf.h`` provides ``printf``-like formatting functionality.
The following functions use `printf format string syntax
<http://pubs.opengroup.org/onlinepubs/009695399/functions/fprintf.html>`_ with
the POSIX extension for positional arguments. Unlike their standard
counterparts, the ``fmt`` functions are type-safe and throw an exception if an
argument type doesn't match its format specification.

.. doxygenfunction:: printf(CStringRef, ArgList)

.. doxygenfunction:: fprintf(std::FILE *, CStringRef, ArgList)

.. doxygenfunction:: fprintf(std::ostream&, CStringRef, ArgList)

.. doxygenfunction:: sprintf(CStringRef, ArgList)

.. doxygenclass:: fmt::PrintfFormatter
   :members:

.. doxygenclass:: fmt::BasicPrintfArgFormatter
   :members:

.. doxygenclass:: fmt::PrintfArgFormatter
   :members:

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

.. doxygenfunction:: fmt::arg(StringRef, const T&)

.. doxygenfunction:: operator""_a(const char *, std::size_t)

.. doxygendefine:: FMT_CAPTURE

.. doxygendefine:: FMT_VARIADIC

.. doxygenclass:: fmt::ArgList
   :members:

.. doxygenfunction:: fmt::to_string(const T&)

.. doxygenclass:: fmt::BasicStringRef
   :members:

.. doxygenclass:: fmt::BasicCStringRef
   :members:

.. doxygenclass:: fmt::Buffer
   :protected-members:
   :members:

System errors
=============

.. doxygenclass:: fmt::SystemError
   :members:

.. doxygenfunction:: fmt::format_system_error

.. doxygenclass:: fmt::WindowsError
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
