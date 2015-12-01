2.0.0 - 2015-12-01
------------------

General
~~~~~~~

* [Breaking] Named arguments
  (`#169 <https://github.com/cppformat/cppformat/pull/169>`_,
  `#173 <https://github.com/cppformat/cppformat/pull/173>`_,
  `#174 <https://github.com/cppformat/cppformat/pull/174>`_):

  .. code:: c++

    fmt::print("The answer is {answer}.", fmt::arg("answer", 42));

  Thanks to `@jamboree <https://github.com/jamboree>`_.

* [Experimental] User-defined literals for format and named arguments
  (`#204 <https://github.com/cppformat/cppformat/pull/204>`_,
  `#206 <https://github.com/cppformat/cppformat/pull/206>`_,
  `#207 <https://github.com/cppformat/cppformat/pull/207>`_):

  .. code:: c++

    using namespace fmt::literals;
    fmt::print("The answer is {answer}.", "answer"_a=42);

  Thanks to `@dean0x7d (Dean Moldovan) <https://github.com/dean0x7d>`_.

* [Breaking] Formatting of more than 16 arguments is now supported when using
  variadic templates
  (`#141 <https://github.com/cppformat/cppformat/issues/141>`_).
  Thanks to `@Shauren <https://github.com/Shauren>`_.

* Runtime width specification
  (`#168 <https://github.com/cppformat/cppformat/pull/168>`_):

  .. code:: c++

    fmt::format("{0:{1}}", 42, 5); // gives "   42"

  Thanks to `@jamboree <https://github.com/jamboree>`_.

* [Breaking] Enums are now formatted with an overloaded ``std::ostream`` insertion
  operator (``operator<<``) if available
  (`#232 <https://github.com/cppformat/cppformat/issues/232>`_).

* [Breaking] Changed default ``bool`` format to textual, "true" or "false"
  (`#170 <https://github.com/cppformat/cppformat/issues/170>`_):

  .. code:: c++
  
    fmt::print("{}", true); // prints "true"

  To print ``bool`` as a number use numeric format specifier such as ``d``:

  .. code:: c++

    fmt::print("{:d}", true); // prints "1"

* ``fmt::printf`` and ``fmt::sprintf`` now support formatting of ``bool`` with the
  ``%s`` specifier giving textual output, "true" or "false"
  (`#223 <https://github.com/cppformat/cppformat/pull/223>`_):

  .. code:: c++

    fmt::printf("%s", true); // prints "true"

  Thanks to `@LarsGullik <https://github.com/LarsGullik>`_.

* [Breaking] ``signed char`` and ``unsigned char`` are now formatted as integers by default
  (`#217 <https://github.com/cppformat/cppformat/pull/217>`_).

* [Breaking] Pointers to C strings can now be formatted with the ``p`` specifier
  (`#223 <https://github.com/cppformat/cppformat/pull/223>`_):

  .. code:: c++

    fmt::print("{:p}", "test"); // prints pointer value

  Thanks to `@LarsGullik <https://github.com/LarsGullik>`_.

* [Breaking] ``fmt::printf`` and ``fmt::sprintf`` now print null pointers as ``(nil)``
  and null strings as ``(null)`` for consistency with glibc
  (`#226 <https://github.com/cppformat/cppformat/pull/226>`_).
  Thanks to `@LarsGullik <https://github.com/LarsGullik>`_.

* [Breaking] ``fmt::(s)printf`` now supports formatting of objects of user-defined types
  that provide an overloaded ``std::ostream`` insertion operator (``operator<<``)
  (`#201 <https://github.com/cppformat/cppformat/issues/201>`_):

  .. code:: c++

    fmt::printf("The date is %s", Date(2012, 12, 9));

* [Breaking] The ``Buffer`` template is now part of the public API and can be used
  to implement custom memory buffers
  (`#140 <https://github.com/cppformat/cppformat/issues/140>`_).
  Thanks to `@polyvertex (Jean-Charles Lefebvre) <https://github.com/polyvertex>`_.

* [Breaking] Improved compatibility between ``BasicStringRef`` and
  `std::experimental::basic_string_view
  <http://en.cppreference.com/w/cpp/experimental/basic_string_view>`_
  (`#100 <https://github.com/cppformat/cppformat/issues/100>`_,
  `#159 <https://github.com/cppformat/cppformat/issues/159>`_,
  `#183 <https://github.com/cppformat/cppformat/issues/183>`_):

  - Comparison operators now compare string content, not pointers
  - ``BasicStringRef::c_str`` replaced by ``BasicStringRef::data``
  - ``BasicStringRef`` is no longer assumed to be null-terminated

  References to null-terminated strings are now represented by a new class,
  ``BasicCStringRef``.

* Dependency on pthreads introduced by Google Test is now optional
  (`#185 <https://github.com/cppformat/cppformat/issues/185>`_).

* New CMake options ``FMT_DOC``, ``FMT_INSTALL`` and ``FMT_TEST`` to control
  generation of ``doc``, ``install`` and ``test`` targets respectively, on by default
  (`#197 <https://github.com/cppformat/cppformat/issues/197>`_,
  `#198 <https://github.com/cppformat/cppformat/issues/198>`_,
  `#200 <https://github.com/cppformat/cppformat/issues/200>`_).
  Thanks to `@maddinat0r (Alex Martin) <https://github.com/maddinat0r>`_.

* ``noexcept`` is now used when compiling with MSVC2015
  (`#215 <https://github.com/cppformat/cppformat/pull/215>`_).
  Thanks to `@dmkrepo (Dmitriy) <https://github.com/dmkrepo>`_.

* Added an option to disable use of ``windows.h`` when ``FMT_USE_WINDOWS_H``
  is defined as 0 before including ``format.h``
  (`#171 <https://github.com/cppformat/cppformat/issues/171>`_).
  Thanks to `@alfps (Alf P. Steinbach) <https://github.com/alfps>`_.

* [Breaking] ``windows.h`` is now included with ``NOMINMAX`` unless
  ``FMT_WIN_MINMAX`` is defined. This is done to prevent breaking code using
  ``std::min`` and ``std::max`` and only affects the header-only configuration
  (`#152 <https://github.com/cppformat/cppformat/issues/152>`_,
  `#153 <https://github.com/cppformat/cppformat/pull/153>`_,
  `#154 <https://github.com/cppformat/cppformat/pull/154>`_).
  Thanks to `@DevO2012 <https://github.com/DevO2012>`_.

* Improved support for custom character types
  (`#171 <https://github.com/cppformat/cppformat/issues/171>`_).
  Thanks to `@alfps (Alf P. Steinbach) <https://github.com/alfps>`_.

* Added an option to disable use of IOStreams when ``FMT_USE_IOSTREAMS``
  is defined as 0 before including ``format.h``
  (`#205 <https://github.com/cppformat/cppformat/issues/205>`_,
  `#208 <https://github.com/cppformat/cppformat/pull/208>`_).
  Thanks to `@JodiTheTigger <https://github.com/JodiTheTigger>`_.

* Improved detection of ``isnan``, ``isinf`` and ``signbit``.

Optimization
~~~~~~~~~~~~

* Made formatting of user-defined types more efficient with a custom stream buffer
  (`#92 <https://github.com/cppformat/cppformat/issues/92>`_,
  `#230 <https://github.com/cppformat/cppformat/pull/230>`_).
  Thanks to `@NotImplemented <https://github.com/NotImplemented>`_.

* Further improved performance of ``fmt::Writer`` on integer formatting
  and fixed a minor regression. Now it is ~7% faster than ``karma::generate``
  on Karma's benchmark
  (`#186 <https://github.com/cppformat/cppformat/issues/186>`_).

* [Breaking] Reduced `compiled code size
  <https://github.com/cppformat/cppformat#compile-time-and-code-bloat>`_
  (`#143 <https://github.com/cppformat/cppformat/issues/143>`_,
  `#149 <https://github.com/cppformat/cppformat/pull/149>`_).

Distribution
~~~~~~~~~~~~

* [Breaking] Headers are now installed in
  ``${CMAKE_INSTALL_PREFIX}/include/cppformat``
  (`#178 <https://github.com/cppformat/cppformat/issues/178>`_).
  Thanks to `@jackyf (Eugene V. Lyubimkin) <https://github.com/jackyf>`_.

* [Breaking] Changed the library name from ``format`` to ``cppformat``
  for consistency with the project name and to avoid potential conflicts
  (`#178 <https://github.com/cppformat/cppformat/issues/178>`_).
  Thanks to `@jackyf (Eugene V. Lyubimkin) <https://github.com/jackyf>`_.

* C++ Format is now available in `Debian <https://www.debian.org/>`_ GNU/Linux
  (`stretch <https://packages.debian.org/source/stretch/cppformat>`_,
  `sid <https://packages.debian.org/source/sid/cppformat>`_) and 
  derived distributions such as
  `Ubuntu <https://launchpad.net/ubuntu/+source/cppformat>`_ 15.10 and later
  (`#155 <https://github.com/cppformat/cppformat/issues/155>`_)::

    $ sudo apt-get install libcppformat1-dev

  Thanks to `@jackyf (Eugene V. Lyubimkin) <https://github.com/jackyf>`_.

* `Packages for Fedora and RHEL <https://admin.fedoraproject.org/pkgdb/package/cppformat/>`_
  are now available. Thanks to Dave Johansen.
  
* C++ Format can now be installed via `Homebrew <http://brew.sh/>`_ on OS X
  (`#157 <https://github.com/cppformat/cppformat/issues/157>`_)::

    $ brew install cppformat

  Thanks to `@ortho <https://github.com/ortho>`_, Anatoliy Bulukin.

Documentation
~~~~~~~~~~~~~

* Migrated from ReadTheDocs to GitHub Pages for better responsiveness
  and reliability
  (`#128 <https://github.com/cppformat/cppformat/issues/128>`_).
  New documentation address is http://cppformat.github.io/.


* Added `Building the documentation
  <http://cppformat.github.io/dev/usage.html#building-the-documentation>`_
  section to the documentation.

* Documentation build script is now compatible with Python 3 and newer pip versions.
  (`#189 <https://github.com/cppformat/cppformat/pull/189>`_,
  `#209 <https://github.com/cppformat/cppformat/issues/209>`_).
  Thanks to `@JodiTheTigger <https://github.com/JodiTheTigger>`_ and
  `@xentec <https://github.com/xentec>`_.
  
* Documentation fixes and improvements
  (`#36 <https://github.com/cppformat/cppformat/issues/36>`_,
  `#75 <https://github.com/cppformat/cppformat/issues/75>`_,
  `#125 <https://github.com/cppformat/cppformat/issues/125>`_,
  `#160 <https://github.com/cppformat/cppformat/pull/160>`_,
  `#161 <https://github.com/cppformat/cppformat/pull/161>`_,
  `#162 <https://github.com/cppformat/cppformat/issues/162>`_,
  `#165 <https://github.com/cppformat/cppformat/issues/165>`_,
  `#210 <https://github.com/cppformat/cppformat/issues/210>`_).
  Thanks to `@syohex (Syohei YOSHIDA) <https://github.com/syohex>`_ and
  bug reporters.

* Fixed out-of-tree documentation build
  (`#177 <https://github.com/cppformat/cppformat/issues/177>`_).
  Thanks to `@jackyf (Eugene V. Lyubimkin) <https://github.com/jackyf>`_.

Fixes
~~~~~

* Fixed ``initializer_list`` detection
  (`#136 <https://github.com/cppformat/cppformat/issues/136>`_).
  Thanks to `@Gachapen (Magnus Bjerke Vik) <https://github.com/Gachapen>`_.

* [Breaking] Fixed formatting of enums with numeric format specifiers in
  ``fmt::(s)printf`` 
  (`#131 <https://github.com/cppformat/cppformat/issues/131>`_,
  `#139 <https://github.com/cppformat/cppformat/issues/139>`_):

  .. code:: c++

    enum { ANSWER = 42 };
    fmt::printf("%d", ANSWER);

  Thanks to `@Naios <https://github.com/Naios>`_.

* Improved compatibility with old versions of MinGW
  (`#129 <https://github.com/cppformat/cppformat/issues/129>`_,
  `#130 <https://github.com/cppformat/cppformat/pull/130>`_,
  `#132 <https://github.com/cppformat/cppformat/issues/132>`_).
  Thanks to `@cstamford (Christopher Stamford) <https://github.com/cstamford>`_.

* Fixed a compile error on MSVC with disabled exceptions
  (`#144 <https://github.com/cppformat/cppformat/issues/144>`_).

* Added a workaround for broken implementation of variadic templates in MSVC2012
  (`#148 <https://github.com/cppformat/cppformat/issues/148>`_).

* Placed the anonymous namespace within ``fmt`` namespace for the header-only
  configuration
  (`#171 <https://github.com/cppformat/cppformat/issues/171>`_).
  Thanks to `@alfps (Alf P. Steinbach) <https://github.com/alfps>`_.

* Fixed issues reported by Coverity Scan
  (`#187 <https://github.com/cppformat/cppformat/issues/187>`_,
  `#192 <https://github.com/cppformat/cppformat/issues/192>`_).

* Implemented a workaround for a name lookup bug in MSVC2010
  (`#188 <https://github.com/cppformat/cppformat/issues/188>`_).

* Fixed compiler warnings
  (`#95 <https://github.com/cppformat/cppformat/issues/95>`_,
  `#96 <https://github.com/cppformat/cppformat/issues/96>`_,
  `#114 <https://github.com/cppformat/cppformat/pull/114>`_,
  `#135 <https://github.com/cppformat/cppformat/issues/135>`_,
  `#142 <https://github.com/cppformat/cppformat/issues/142>`_,
  `#145 <https://github.com/cppformat/cppformat/issues/145>`_,
  `#146 <https://github.com/cppformat/cppformat/issues/146>`_,
  `#158 <https://github.com/cppformat/cppformat/issues/158>`_,
  `#163 <https://github.com/cppformat/cppformat/issues/163>`_,
  `#175 <https://github.com/cppformat/cppformat/issues/175>`_,
  `#190 <https://github.com/cppformat/cppformat/issues/190>`_,
  `#191 <https://github.com/cppformat/cppformat/pull/191>`_,
  `#194 <https://github.com/cppformat/cppformat/issues/194>`_,
  `#196 <https://github.com/cppformat/cppformat/pull/196>`_,
  `#216 <https://github.com/cppformat/cppformat/issues/216>`_,
  `#218 <https://github.com/cppformat/cppformat/pull/218>`_,
  `#220 <https://github.com/cppformat/cppformat/pull/220>`_,
  `#229 <https://github.com/cppformat/cppformat/pull/229>`_,
  `#233 <https://github.com/cppformat/cppformat/issues/233>`_,
  `#234 <https://github.com/cppformat/cppformat/issues/234>`_,
  `#236 <https://github.com/cppformat/cppformat/pull/236>`_).
  Thanks to `@seanmiddleditch (Sean Middleditch) <https://github.com/seanmiddleditch>`_,
  `@dixlorenz (Dix Lorenz) <https://github.com/dixlorenz>`_,
  `@CarterLi (李通洲) <https://github.com/CarterLi>`_,
  `@Naios <https://github.com/Naios>`_,
  `@fmatthew5876 (Matthew Fioravante) <https://github.com/fmatthew5876>`_,
  `@LevskiWeng (Levski Weng) <https://github.com/LevskiWeng>`_,
  `@rpopescu <https://github.com/rpopescu>`_,
  `@gabime (Gabi Melman) <https://github.com/gabime>`_,
  `@cubicool (Jeremy Moles) <https://github.com/cubicool>`_,
  `@jkflying (Julian Kent) <https://github.com/jkflying>`_,
  `@LogicalKnight (Sean L) <https://github.com/LogicalKnight>`_,
  `@inguin (Ingo van Lil) <https://github.com/inguin>`_ and
  `@Jopie64 (Johan) <https://github.com/Jopie64>`_.

* Fixed portability issues (mostly causing test failures) on ARM, ppc64, ppc64le,
  s390x and SunOS 5.11 i386 (
  `#138 <https://github.com/cppformat/cppformat/issues/138>`_,
  `#179 <https://github.com/cppformat/cppformat/issues/179>`_,
  `#180 <https://github.com/cppformat/cppformat/issues/180>`_,
  `#202 <https://github.com/cppformat/cppformat/issues/202>`_,
  `#225 <https://github.com/cppformat/cppformat/issues/225>`_,
  `Red Hat Bugzilla Bug 1260297 <https://bugzilla.redhat.com/show_bug.cgi?id=1260297>`_).
  Thanks to `@Naios <https://github.com/Naios>`_,
  `@jackyf (Eugene V. Lyubimkin) <https://github.com/jackyf>`_ and Dave Johansen.

* Fixed a name conflict with macro ``free`` defined in
  ``crtdbg.h`` when ``_CRTDBG_MAP_ALLOC`` is set
  (`#211 <https://github.com/cppformat/cppformat/issues/211>`_).

* Fixed shared library build on OS X
  (`#212 <https://github.com/cppformat/cppformat/pull/212>`_).
  Thanks to `@dean0x7d (Dean Moldovan) <https://github.com/dean0x7d>`_.

* Fixed an overload conflict on MSVC when ``/Zc:wchar_t-`` option is specified
  (`#214 <https://github.com/cppformat/cppformat/pull/214>`_).
  Thanks to `@slavanap (Vyacheslav Napadovsky) <https://github.com/slavanap>`_.

* Improved compatibility with MSVC 2008
  (`#236 <https://github.com/cppformat/cppformat/pull/236>`_).
  Thanks to `@Jopie64 (Johan) <https://github.com/Jopie64>`_.

* Improved compatibility with bcc32
  (`#227 <https://github.com/cppformat/cppformat/issues/227>`_).

* Fixed ``static_assert`` detection on Clang
  (`#228 <https://github.com/cppformat/cppformat/pull/228>`_).
  Thanks to `@dean0x7d (Dean Moldovan) <https://github.com/dean0x7d>`_.

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
