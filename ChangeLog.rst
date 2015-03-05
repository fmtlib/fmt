0.11.0 - 2014-08-21
-------------------

* Safe printf implementation with a POSIX extension for positional arguments:

  .. code:: c++

    fmt::printf("Elapsed time: %.2f seconds", 1.23);
    fmt::printf("%1$s, %3$d %2$s", weekday, month, day);

* Arguments of ``char`` type can now be formatted as integers
  (Issue `#55 <https://github.com/cppformat/cppformat/issues/55>`_):

  .. code:: c++

    fmt::format("0x{0:02X}", 'a');

* Deprecated parts of the API removed.

* The library is now built and tested on MinGW with Appveyor in addition to
  existing test platforms Linux/GCC, OS X/Clang, Windows/MSVC.

0.10.0 - 2014-07-01
-------------------

**Improved API**

* All formatting methods are now implemented as variadic functions instead
  of using ``operator<<`` for feeding arbitrary arguments into a temporary
  formatter object. This works both with C++11 where variadic templates are
  used and with older standards where variadic functions are emulated by
  providing lightweight wrapper functions defined with the ``FMT_VARIADIC``
  macro. You can use this macro for defining your own portable variadic
  functions:

  .. code:: c++

    void report_error(const char *format, const fmt::ArgList &args) {
      fmt::print("Error: {}");
      fmt::print(format, args);
    }
    FMT_VARIADIC(void, report_error, const char *)

    report_error("file not found: {}", path);

  Apart from a more natural syntax, this also improves performance as there
  is no need to construct temporary formatter objects and control arguments'
  lifetimes. Because the wrapper functions are very ligthweight, this doesn't
  cause code bloat even in pre-C++11 mode.

* Simplified common case of formatting an ``std::string``. Now it requires a
  single function call:

  .. code:: c++

    std::string s = format("The answer is {}.", 42);

  Previously it required 2 function calls:

  .. code:: c++

    std::string s = str(Format("The answer is {}.") << 42);

  Instead of unsafe ``c_str`` function, ``fmt::Writer`` should be used directly
  to bypass creation of ``std::string``:

  .. code:: c++

    fmt::Writer w;
    w.write("The answer is {}.", 42);
    w.c_str();  // returns a C string

  This doesn't do dynamic memory allocation for small strings and is less error
  prone as the lifetime of the string is the same as for ``std::string::c_str``
  which is well understood (hopefully).

* Improved consistency in naming functions that are a part of the public API.
  Now all public functions are lowercase following the standard library
  conventions. Previously it was a combination of lowercase and
  CapitalizedWords.
  Issue `#50 <https://github.com/cppformat/cppformat/issues/50>`_.

* Old functions are marked as deprecated and will be removed in the next
  release.

**Other Changes**

* Experimental support for printf format specifications (work in progress):

  .. code:: c++

    fmt::printf("The answer is %d.", 42);
    std::string s = fmt::sprintf("Look, a %s!", "string");

* Support for hexadecimal floating point format specifiers ``a`` and ``A``:

  .. code:: c++

    print("{:a}", -42.0); // Prints -0x1.5p+5
    print("{:A}", -42.0); // Prints -0X1.5P+5

* CMake option ``FMT_SHARED`` that specifies whether to build format as a
  shared library (off by default).

0.9.0 - 2014-05-13
------------------

* More efficient implementation of variadic formatting functions.

* ``Writer::Format`` now has a variadic overload:

  .. code:: c++

    Writer out;
    out.Format("Look, I'm {}!", "variadic");

* For efficiency and consistency with other overloads, variadic overload of
  the ``Format`` function now returns ``Writer`` instead of ``std::string``.
  Use the ``str`` function to convert it to ``std::string``:

  .. code:: c++

    std::string s = str(Format("Look, I'm {}!", "variadic"));

* Replaced formatter actions with output sinks: ``NoAction`` -> ``NullSink``,
  ``Write`` -> ``FileSink``, ``ColorWriter`` -> ``ANSITerminalSink``.
  This improves naming consistency and shouldn't affect client code unless
  these classes are used directly which should be rarely needed.

* Added ``ThrowSystemError`` function that formats a message and throws
  ``SystemError`` containing the formatted message and system-specific error
  description. For example, the following code

  .. code:: c++

    FILE *f = fopen(filename, "r");
    if (!f)
      ThrowSystemError(errno, "Failed to open file '{}'") << filename;

  will throw ``SystemError`` exception with description
  "Failed to open file '<filename>': No such file or directory" if file
  doesn't exist.

* Support for AppVeyor continuous integration platform.

* ``Format`` now throws ``SystemError`` in case of I/O errors.

* Improve test infrastructure. Print functions are now tested by redirecting
  the output to a pipe.

0.8.0 - 2014-04-14
------------------

* Initial release
