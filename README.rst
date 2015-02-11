C++ Format
==========

.. image:: https://travis-ci.org/cppformat/cppformat.png?branch=master
  :target: https://travis-ci.org/cppformat/cppformat

.. image:: https://ci.appveyor.com/api/projects/status/qk0bhyhqp1ekpat8
  :target: https://ci.appveyor.com/project/vitaut/cppformat

.. image:: https://readthedocs.org/projects/cppformat/badge/?version=stable
   :target: http://cppformat.readthedocs.org/en/stable/
   :alt: Documentation Status
   
.. image:: https://webapi.biicode.com/v1/badges/vitaut/vitaut/cppformat/master
  :target: https://www.biicode.com/vitaut/cppformat

C++ Format is an open-source formatting library for C++.
It can be used as a safe alternative to printf or as a fast
alternative to IOStreams.

Features
--------

* Two APIs: faster concatenation-based write API and slower (but still
  very fast) replacement-based format API with positional arguments for
  localization.
* Write API similar to the one used by IOStreams but much faster and more
  consistent.
* Format API with `format string syntax
  <http://cppformat.readthedocs.org/en/latest/syntax.html>`_
  similar to the one used by `str.format
  <http://docs.python.org/2/library/stdtypes.html#str.format>`_ in Python.
* Support for user-defined types.
* High speed: performance of the format API is close to that of
  glibc's `printf <http://en.cppreference.com/w/cpp/io/c/fprintf>`_
  and better than performance of IOStreams. See `Speed tests`_ and
  `Fast integer to string conversion in C++
  <http://zverovich.net/2013/09/07/integer-to-string-conversion-in-cplusplus.html>`_.
* Small code size both in terms of source code (format consists of a single
  header file and a single source file) and compiled code.
  See `Compile time and code bloat`_.
* Reliability: the library has an extensive set of `unit tests
  <https://github.com/cppformat/cppformat/tree/master/test>`_.
* Safety: the library is fully type safe, errors in format strings are
  reported using exceptions, automatic memory management prevents buffer
  overflow errors.
* Ease of use: small self-contained code base, no external dependencies,
  permissive BSD `license`_.
* `Portability <http://cppformat.github.io#portability>`_ with consistent output
  across platforms and support for older compilers.
* Clean warning-free codebase even on high warning levels
  (-Wall -Wextra -pedantic).
* Support for wide strings.

See the `documentation <http://cppformat.readthedocs.org/en/stable/>`_ for more details.

Examples
--------

This prints ``Hello, world!`` to stdout:

.. code:: c++

    fmt::print("Hello, {}!", "world");  // uses Python-like format string syntax
    fmt::printf("Hello, %s!", "world"); // uses printf format string syntax

Arguments can be accessed by position and arguments' indices can be repeated:

.. code:: c++

    std::string s = fmt::format("{0}{1}{0}", "abra", "cad");
    // s == "abracadabra"

C++ Format can be used as a safe portable replacement for ``itoa``:

.. code:: c++

    fmt::MemoryWriter w;
    w << 42;           // replaces itoa(42, buffer, 10)
    w << fmt::hex(42); // replaces itoa(42, buffer, 16)
    // access the string using w.str() or w.c_str()

An object of any user-defined type for which there is an overloaded
:code:`std::ostream` insertion operator (``operator<<``) can be formatted:

.. code:: c++

    class Date {
      int year_, month_, day_;
     public:
      Date(int year, int month, int day) : year_(year), month_(month), day_(day) {}

      friend std::ostream &operator<<(std::ostream &os, const Date &d) {
        return os << d.year_ << '-' << d.month_ << '-' << d.day_;
      }
    };

    std::string s = fmt::format("The date is {}", Date(2012, 12, 9));
    // s == "The date is 2012-12-9"

You can use the `FMT_VARIADIC
<http://cppformat.readthedocs.org/en/latest/reference.html#utilities>`_
macro to create your own functions similar to `format
<http://cppformat.readthedocs.org/en/latest/reference.html#format>`_ and
`print <http://cppformat.readthedocs.org/en/latest/reference.html#print>`_
which take arbitrary arguments:

.. code:: c++

    // Prints formatted error message.
    void report_error(const char *format, fmt::ArgList args) {
      fmt::print("Error: ");
      fmt::print(format, args);
    }
    FMT_VARIADIC(void, report_error, const char *)

    report_error("file not found: {}", path);

Note that you only need to define one function that takes ``fmt::ArgList``
argument. ``FMT_VARIADIC`` automatically defines necessary wrappers that
accept variable number of arguments.

Projects using this library
---------------------------

* `0 A.D. <http://play0ad.com/>`_: A free, open-source, cross-platform real-time strategy game

* `AMPL/MP <https://github.com/ampl/mp>`_:
  An open-source library for mathematical programming

* `HarpyWar/pvpgn <https://github.com/HarpyWar/pvpgn>`_:
  Player vs Player Gaming Network with tweaks

* `KBEngine <http://www.kbengine.org/>`_: An open-source MMOG server engine

* `Lifeline <https://github.com/peter-clark/lifeline>`_: A 2D game

* `PenUltima Online (POL) <http://www.polserver.com/>`_:
  An MMO server, compatible with most Ultima Online clients

* `readpe <https://bitbucket.org/sys_dev/readpe>`_: Read Portable Executable

* `Saddy <https://code.google.com/p/saddy/>`_:
  Small crossplatform 2D graphic engine

* `Salesforce Analytics Cloud <http://www.salesforce.com/analytics-cloud/overview/>`_:
  Business intelligence software

* `spdlog <https://github.com/gabime/spdlog>`_: Super fast C++ logging library

`More... <https://github.com/search?q=cppformat&type=Code>`_

If you are aware of other projects using this library, please let me know
by `email <mailto:victor.zverovich@gmail.com>`_ or by submitting an
`issue <https://github.com/cppformat/cppformat/issues>`_.

Motivation
----------

So why yet another formatting library?

There are plenty of methods for doing this task, from standard ones like
the printf family of function and IOStreams to Boost Format library and
FastFormat. The reason for creating a new library is that every existing
solution that I found either had serious issues or didn't provide
all the features I needed.

Printf
~~~~~~

The good thing about printf is that it is very fast and readily available
being a part of the C standard library. The main drawback is that it
doesn't support user-defined types. Printf also has safety issues although
they are mostly solved with `_attribute__ ((format (printf, ...))
<http://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html>`_ in GCC.
There is a POSIX extension that adds positional arguments required for
`i18n <http://en.wikipedia.org/wiki/Internationalization_and_localization>`_
to printf but it is not a part of C99 and may not be available on some
platforms.

IOStreams
~~~~~~~~~

The main issue with IOStreams is best illustrated with an example:

.. code:: c++

    std::cout << std::setprecision(2) << std::fixed << 1.23456 << "\n";

which is a lot of typing compared to printf:

.. code:: c++

    printf("%.2f\n", 1.23456);

Matthew Wilson, the author of FastFormat, referred to this situation with
IOStreams as "chevron hell". IOStreams doesn't support positional arguments
by design.

The good part is that IOStreams supports user-defined types and is safe
although error reporting is awkward.

Boost Format library
~~~~~~~~~~~~~~~~~~~~

This is a very powerful library which supports both printf-like format
strings and positional arguments. The main its drawback is performance.
According to various benchmarks it is much slower than other methods
considered here. Boost Format also has excessive build times and severe
code bloat issues (see `Benchmarks`_).

FastFormat
~~~~~~~~~~

This is an interesting library which is fast, safe and has positional
arguments. However it has significant limitations, citing its author:

    Three features that have no hope of being accommodated within the
    current design are:

    * Leading zeros (or any other non-space padding)
    * Octal/hexadecimal encoding
    * Runtime width/alignment specification

It is also quite big and has a heavy dependency, STLSoft, which might be
too restrictive for using it in some projects.

Loki SafeFormat
~~~~~~~~~~~~~~~

SafeFormat is a formatting library which uses printf-like format strings
and is type safe. It doesn't support user-defined types or positional
arguments. It makes unconventional use of ``operator()`` for passing
format arguments.

Tinyformat
~~~~~~~~~~

This library supports printf-like format strings and is very small and
fast. Unfortunately it doesn't support positional arguments and wrapping
it in C++98 is somewhat difficult.  However if you only need a type-safe
printf replacement with support for user-defined types, I highly recommend
this library.

Boost Spirit.Karma
~~~~~~~~~~~~~~~~~~

This is not really a formatting library but I decided to include it here
for completeness. As IOStreams it suffers from the problem of mixing
verbatim text with arguments. The library is pretty fast, but slower
on integer formatting than ``fmt::Writer`` on Karma's own benchmark,
see `Fast integer to string conversion in C++
<http://zverovich.net/2013/09/07/integer-to-string-conversion-in-cplusplus.html>`_.

Benchmarks
----------

Speed tests
~~~~~~~~~~~

The following speed tests results were generated by building
``tinyformat_test.cpp`` on Ubuntu GNU/Linux 14.04.1 with
``g++-4.8.2 -O3 -DSPEED_TEST -DHAVE_FORMAT``, and taking the best of three
runs.  In the test, the format string ``"%0.10f:%04d:%+g:%s:%p:%c:%%\n"`` or
equivalent is filled 2000000 times with output sent to ``/dev/null``; for
further details see the `source
<https://github.com/cppformat/format-benchmark/blob/master/tinyformat_test.cpp>`_.

============== ========
test name      run time
============== ========
libc printf     1.30s
std::ostream    1.85s
cppformat       1.42s
tinyformat      2.25s
boost::format   9.94s
============== ========

As you can see boost::format is much slower than the alternative methods; this
is confirmed by `other tests <http://accu.org/index.php/journals/1539>`_.
Tinyformat is quite good coming close to IOStreams.  Unfortunately tinyformat
cannot be faster than the IOStreams because it uses them internally.
Performance of cppformat is close to that of printf, being `faster than printf on integer
formatting <http://zverovich.net/2013/09/07/integer-to-string-conversion-in-cplusplus.html>`_,
but slower on floating-point formatting which dominates this benchmark.

Compile time and code bloat
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The script `bloat-test.py
<https://github.com/cppformat/format-benchmark/blob/master/bloat-test.py>`_
from `format-benchmark <https://github.com/cppformat/format-benchmark>`_
tests compile time and code bloat for nontrivial projects.
It generates 100 translation units and uses ``printf()`` or its alternative
five times in each to simulate a medium sized project.  The resulting
executable size and compile time (g++-4.8.1, Ubuntu GNU/Linux 13.10,
best of three) is shown in the following tables.

**Optimized build (-O3)**

============ =============== ==================== ==================
Method       Compile Time, s Executable size, KiB Stripped size, KiB
============ =============== ==================== ==================
printf                   2.6                   41                 30
IOStreams               19.4                   92                 70
C++ Format              46.8                   46                 34
tinyformat              64.6                  418                386
Boost Format           222.8                  990                923
============ =============== ==================== ==================

As you can see, C++ Format has 80% less overhead in terms of resulting
code size compared to IOStreams and comes pretty close to ``printf``.
Boost Format has by far the largest overheads.

**Non-optimized build**

============ =============== ==================== ==================
Method       Compile Time, s Executable size, KiB Stripped size, KiB
============ =============== ==================== ==================
printf                   2.1                   41                 30
IOStreams               19.7                   86                 62
C++ Format              47.9                  108                 86
tinyformat              27.7                  234                190
Boost Format           122.6                  884                763
============ =============== ==================== ==================

``libc``, ``libstdc++`` and ``libformat`` are all linked as shared
libraries to compare formatting function overhead only. Boost Format
and tinyformat are header-only libraries so they don't provide any
linkage options.

Running the tests
~~~~~~~~~~~~~~~~~

Please refer to `Building the library`__ for the instructions on how to build
the library and run the unit tests.

__ http://cppformat.readthedocs.org/en/latest/usage.html#building-the-library

Benchmarks reside in a separate repository,
`format-benchmarks <https://github.com/cppformat/format-benchmark>`_,
so to run the benchmarks you first need to clone this repository and
generate Makefiles with CMake::

    $ git clone --recursive https://github.com/cppformat/format-benchmark.git
    $ cd format-benchmark
    $ cmake .

Then you can run the speed test::

    $ make speed-test

or the bloat test::

    $ make bloat-test

License
-------

Copyright (c) 2012, Victor Zverovich

All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Documentation License
---------------------

The `Format String Syntax
<http://cppformat.readthedocs.org/en/latest/#format-string-syntax>`_
section in the documentation is based on the one from Python `string module
documentation <http://docs.python.org/3/library/string.html#module-string>`_
adapted for the current library. For this reason the documentation is
distributed under the Python Software Foundation license available in
`doc/LICENSE.python
<https://raw.github.com/cppformat/cppformat/master/doc/LICENSE.python>`_.

Acknowledgments
---------------

The benchmark section of this readme file and the performance tests are taken
from the excellent `tinyformat <https://github.com/c42f/tinyformat>`_ library
written by Chris Foster.  Boost Format library is acknowledged transitively
since it had some influence on tinyformat.
Some ideas used in the implementation are borrowed from `Loki
<http://loki-lib.sourceforge.net/>`_ SafeFormat and `Diagnostic API
<http://clang.llvm.org/doxygen/classclang_1_1Diagnostic.html>`_ in
`Clang <http://clang.llvm.org/>`_.
Format string syntax and the documentation are based on Python's `str.format
<http://docs.python.org/2/library/stdtypes.html#str.format>`_.
Thanks `Doug Turnbull <https://github.com/softwaredoug>`_ for his valuable
comments and contribution to the design of the type-safe API and
`Gregory Czajkowski <https://github.com/gcflymoto>`_ for implementing binary
formatting. Thanks `Ruslan Baratov <https://github.com/ruslo>`_ for comprehensive
`comparison of integer formatting algorithms <https://github.com/ruslo/int-dec-format-tests>`_
and useful comments regarding performance, `Boris Kaul <https://github.com/localvoid>`_ for
`C++ counting digits benchmark <https://github.com/localvoid/cxx-benchmark-count-digits>`_.
