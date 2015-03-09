1.1.0 - 2015-03-06
------------------

* Added ``BasicArrayWriter``, a class template that provides operations for
  formatting and writing data into a fixed-size array
  (`#105 <https://github.com/cppformat/cppformat/issues/105>`_ and
  `#122 <https://github.com/cppformat/cppformat/issues/122>`_):

  .. code:: c++
  
    char buffer[100];
    fmt::ArrayWriter w(buffer);
    w.write("The answer is {}", 42);

* Added `0 A.D. <http://play0ad.com/>`_ and `PenUltima Online (POL)
  <http://www.polserver.com/>`_ to the list of notable projects using C++ Format.

* C++ Format now uses MSVC intrinsics for better formatting performance
  (`#115 <https://github.com/cppformat/cppformat/pull/115>`_,
  `#116 <https://github.com/cppformat/cppformat/pull/116>`_,
  `#118 <https://github.com/cppformat/cppformat/pull/118>`_ and
  `#121 <https://github.com/cppformat/cppformat/pull/121>`_).
  Previously these optimizations where only used on GCC and Clang.
  Thanks to `@CarterLi <https://github.com/CarterLi>`_ and
  `@objectx <https://github.com/objectx>`_.

* CMake install target (`#119 <https://github.com/cppformat/cppformat/pull/119>`_).
  Thanks to `@TrentHouliston <https://github.com/TrentHouliston>`_.

  You can now install C++ Format with ``make install`` command.

* Improved `Biicode <http://www.biicode.com/>`_ support
  (`#98 <https://github.com/cppformat/cppformat/pull/98>`_ and
  `#104 <https://github.com/cppformat/cppformat/pull/104>`_). Thanks to
  `@MariadeAnton <https://github.com/MariadeAnton>`_ and
  `@franramirez688 <https://github.com/franramirez688>`_.

* Improved support for bulding with `Android NDK
  <https://developer.android.com/tools/sdk/ndk/index.html>`_
  (`#107 <https://github.com/cppformat/cppformat/pull/107>`_).
  Thanks to `@newnon <https://github.com/newnon>`_.
  
  The `android-ndk-example <https://github.com/cppformat/android-ndk-example>`_
  repository provides and example of using C++ Format with Android NDK:

  .. image:: https://raw.githubusercontent.com/cppformat/android-ndk-example/
            master/screenshot.png

* Improved documentation of ``SystemError`` and ``WindowsError``
  (`#54 <https://github.com/cppformat/cppformat/issues/54>`_).

* Various code improvements
  (`#110 <https://github.com/cppformat/cppformat/pull/110>`_,
  `#111 <https://github.com/cppformat/cppformat/pull/111>`_
  `#112 <https://github.com/cppformat/cppformat/pull/112>`_).
  Thanks to `@CarterLi <https://github.com/CarterLi>`_.

* Improved compile-time errors when formatting wide into narrow strings
  (`#117 <https://github.com/cppformat/cppformat/issues/117>`_).

* Fixed ``BasicWriter::write`` without formatting arguments when C++11 support
  is disabled (`#109 <https://github.com/cppformat/cppformat/issues/109>`_).

* Fixed header-only build on OS X with GCC 4.9
  (`#124 <https://github.com/cppformat/cppformat/issues/124>`_).

* Fixed packaging issues (`#94 <https://github.com/cppformat/cppformat/issues/94>`_).

* Fixed warnings in GCC, MSVC and Xcode/Clang
  (`#95 <https://github.com/cppformat/cppformat/issues/95>`_,
  `#96 <https://github.com/cppformat/cppformat/issues/96>`_ and
  `#114 <https://github.com/cppformat/cppformat/pull/114>`_).

* Added `changelog <https://github.com/cppformat/cppformat/blob/master/ChangeLog.rst>`_
  (`#103 <https://github.com/cppformat/cppformat/issues/103>`_).

1.0.0 - 2015-02-05
------------------

* Add support for a header-only configuration when ``FMT_HEADER_ONLY`` is
  defined before including ``format.h``:

  .. code:: c++

    #define FMT_HEADER_ONLY
    #include "format.h"

* Compute string length in the constructor of ``BasicStringRef``
  instead of the ``size`` method
  (`#79 <https://github.com/cppformat/cppformat/issues/79>`_).
  This eliminates size computation for string literals on reasonable optimizing
  compilers.

* Fix formatting of types with overloaded ``operator <<`` for ``std::wostream``
  (`#86 <https://github.com/cppformat/cppformat/issues/86>`_):

  .. code:: c++

    fmt::format(L"The date is {0}", Date(2012, 12, 9));

* Fix linkage of tests on Arch Linux
  (`#89 <https://github.com/cppformat/cppformat/issues/89>`_).

* Allow precision specifier for non-float arguments
  (`#90 <https://github.com/cppformat/cppformat/issues/90>`_):

  .. code:: c++

    fmt::print("{:.3}\n", "Carpet"); // prints "Car"

* Fix build on Android NDK
  (`#93 <https://github.com/cppformat/cppformat/issues/93>`_)

* Improvements to documentation build procedure.

* Remove ``FMT_SHARED`` CMake variable in favor of standard `BUILD_SHARED_LIBS
  <http://www.cmake.org/cmake/help/v3.0/variable/BUILD_SHARED_LIBS.html>`_.

* Fix error handling in ``fmt::fprintf``.

* Fix a number of warnings.

0.12.0 - 2014-10-25
-------------------

* [Breaking] Improved separation between formatting and buffer management.
  ``Writer`` is now a base class that cannot be instantiated directly.
  The new ``MemoryWriter`` class implements the default buffer management
  with small allocations done on stack. So ``fmt::Writer`` should be replaced
  with ``fmt::MemoryWriter`` in variable declarations.

  Old code:

  .. code:: c++

    fmt::Writer w;

  New code: 

  .. code:: c++

    fmt::MemoryWriter w;

  If you pass ``fmt::Writer`` by reference, you can continue to do so:

  .. code:: c++

      void f(fmt::Writer &w);

  This doesn't affect the formatting API.

* Support for custom memory allocators
  (`#69 <https://github.com/cppformat/cppformat/issues/69>`_)

* Formatting functions now accept `signed char` and `unsigned char` strings as
  arguments (`#73 <https://github.com/cppformat/cppformat/issues/73>`_):

  .. code:: c++

    auto s = format("GLSL version: {}", glGetString(GL_VERSION));

* Reduced code bloat. According to the new `benchmark results
  <https://github.com/cppformat/cppformat#compile-time-and-code-bloat>`_,
  cppformat is close to ``printf`` and by the order of magnitude better than
  Boost Format in terms of compiled code size.

* Improved appearance of the documentation on mobile by using the `Sphinx
  Bootstrap theme <http://ryan-roemer.github.io/sphinx-bootstrap-theme/>`_:

  .. |old| image:: https://cloud.githubusercontent.com/assets/576385/4792130/
                   cd256436-5de3-11e4-9a62-c077d0c2b003.png

  .. |new| image:: https://cloud.githubusercontent.com/assets/576385/4792131/
                   cd29896c-5de3-11e4-8f59-cac952942bf0.png
  
  +-------+-------+
  |  Old  |  New  |
  +-------+-------+
  | |old| | |new| |
  +-------+-------+

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
