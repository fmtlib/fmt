{fmt}
=====

.. image:: https://travis-ci.org/fmtlib/fmt.png?branch=master
   :target: https://travis-ci.org/fmtlib/fmt

.. image:: https://ci.appveyor.com/api/projects/status/ehjkiefde6gucy1v
   :target: https://ci.appveyor.com/project/vitaut/fmt

.. image:: https://oss-fuzz-build-logs.storage.googleapis.com/badges/libfmt.svg
   :alt: fmt is continuously fuzzed att oss-fuzz
   :target: https://bugs.chromium.org/p/oss-fuzz/issues/list?colspec=ID%20Type%20Component%20Status%20Proj%20Reported%20Owner%20Summary&q=proj%3Dlibfmt&can=1

.. image:: https://img.shields.io/badge/stackoverflow-fmt-blue.svg
   :alt: Ask questions at StackOverflow with the tag fmt
   :target: https://stackoverflow.com/questions/tagged/fmt

**{fmt}** is an open-source formatting library for C++.
It can be used as a safe and fast alternative to (s)printf and iostreams.

`Documentation <https://fmt.dev/latest/>`__

Q&A: ask questions on `StackOverflow with the tag fmt <https://stackoverflow.com/questions/tagged/fmt>`_.

Features
--------

* Replacement-based `format API <https://fmt.dev/dev/api.html>`_ with
  positional arguments for localization.
* `Format string syntax <https://fmt.dev/dev/syntax.html>`_ similar to the one
  of `str.format <https://docs.python.org/3/library/stdtypes.html#str.format>`_
  in Python.
* Safe `printf implementation
  <https://fmt.dev/latest/api.html#printf-formatting>`_ including
  the POSIX extension for positional arguments.
* Implementation of `C++20 std::format <https://fmt.dev/Text%20Formatting.html>`__.
* Support for user-defined types.
* High performance: faster than common standard library implementations of
  `printf <https://en.cppreference.com/w/cpp/io/c/fprintf>`_ and
  iostreams. See `Speed tests`_ and `Fast integer to string conversion in C++
  <http://zverovich.net/2013/09/07/integer-to-string-conversion-in-cplusplus.html>`_.
* Small code size both in terms of source code (the minimum configuration
  consists of just three header files, ``core.h``, ``format.h`` and
  ``format-inl.h``) and compiled code. See `Compile time and code bloat`_.
* Reliability: the library has an extensive set of `unit tests
  <https://github.com/fmtlib/fmt/tree/master/test>`_ and is continuously fuzzed.
* Safety: the library is fully type safe, errors in format strings can be
  reported at compile time, automatic memory management prevents buffer overflow
  errors.
* Ease of use: small self-contained code base, no external dependencies,
  permissive MIT `license
  <https://github.com/fmtlib/fmt/blob/master/LICENSE.rst>`_
* `Portability <https://fmt.dev/latest/index.html#portability>`_ with
  consistent output across platforms and support for older compilers.
* Clean warning-free codebase even on high warning levels
  (``-Wall -Wextra -pedantic``).
* Support for wide strings.
* Optional header-only configuration enabled with the ``FMT_HEADER_ONLY`` macro.

See the `documentation <https://fmt.dev/latest/>`_ for more details.

Examples
--------

Print ``Hello, world!`` to ``stdout``:

.. code:: c++

    fmt::print("Hello, {}!", "world");  // Python-like format string syntax
    fmt::printf("Hello, %s!", "world"); // printf format string syntax

Format a string and use positional arguments:

.. code:: c++

    std::string s = fmt::format("I'd rather be {1} than {0}.", "right", "happy");
    // s == "I'd rather be happy than right."

Check a format string at compile time:

.. code:: c++

    // test.cc
    #include <fmt/format.h>
    std::string s = format(FMT_STRING("{2}"), 42);

.. code::

    $ c++ -Iinclude -std=c++14 test.cc
    ...
    test.cc:4:17: note: in instantiation of function template specialization 'fmt::v5::format<S, int>' requested here
    std::string s = format(FMT_STRING("{2}"), 42);
                    ^
    include/fmt/core.h:778:19: note: non-constexpr function 'on_error' cannot be used in a constant expression
        ErrorHandler::on_error(message);
                      ^
    include/fmt/format.h:2226:16: note: in call to '&checker.context_->on_error(&"argument index out of range"[0])'
          context_.on_error("argument index out of range");
                   ^

Use {fmt} as a safe portable replacement for ``itoa``
(`godbolt <https://godbolt.org/g/NXmpU4>`_):

.. code:: c++

    fmt::memory_buffer buf;
    format_to(buf, "{}", 42);    // replaces itoa(42, buffer, 10)
    format_to(buf, "{:x}", 42);  // replaces itoa(42, buffer, 16)
    // access the string with to_string(buf) or buf.data()

Format objects of user-defined types via a simple `extension API
<https://fmt.dev/latest/api.html#formatting-user-defined-types>`_:

.. code:: c++

    #include "fmt/format.h"

    struct date {
      int year, month, day;
    };

    template <>
    struct fmt::formatter<date> {
      constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }

      template <typename FormatContext>
      auto format(const date& d, FormatContext& ctx) {
        return format_to(ctx.out(), "{}-{}-{}", d.year, d.month, d.day);
      }
    };

    std::string s = fmt::format("The date is {}", date{2012, 12, 9});
    // s == "The date is 2012-12-9"

Create your own functions similar to `format
<https://fmt.dev/latest/api.html#format>`_ and
`print <https://fmt.dev/latest/api.html#print>`_
which take arbitrary arguments (`godbolt <https://godbolt.org/g/MHjHVf>`_):

.. code:: c++

    // Prints formatted error message.
    void vreport_error(const char* format, fmt::format_args args) {
      fmt::print("Error: ");
      fmt::vprint(format, args);
    }
    template <typename... Args>
    void report_error(const char* format, const Args & ... args) {
      vreport_error(format, fmt::make_format_args(args...));
    }

    report_error("file not found: {}", path);

Note that ``vreport_error`` is not parameterized on argument types which can
improve compile times and reduce code size compared to a fully parameterized
version.

Benchmarks
----------

Speed tests
~~~~~~~~~~~

================= ============= ===========
Library           Method        Run Time, s
================= ============= ===========
libc              printf          1.04
libc++            std::ostream    3.05
{fmt} 6.1.1       fmt::print      0.75
Boost Format 1.67 boost::format   7.24
Folly Format      folly::format   2.23
================= ============= ===========

{fmt} is the fastest of the benchmarked methods, ~35% faster than ``printf``.

The above results were generated by building ``tinyformat_test.cpp`` on macOS
10.14.6 with ``clang++ -O3 -DSPEED_TEST -DHAVE_FORMAT``, and taking the best of
three runs. In the test, the format string ``"%0.10f:%04d:%+g:%s:%p:%c:%%\n"``
or equivalent is filled 2,000,000 times with output sent to ``/dev/null``; for
further details refer to the `source
<https://github.com/fmtlib/format-benchmark/blob/master/tinyformat_test.cpp>`_.

{fmt} is 10x faster than ``std::ostringstream`` and ``sprintf`` on floating-point
formatting (`dtoa-benchmark <https://github.com/fmtlib/dtoa-benchmark>`_)
and as fast as `double-conversion <https://github.com/google/double-conversion>`_:

.. image:: https://user-images.githubusercontent.com/576385/69767160-cdaca400-112f-11ea-9fc5-347c9f83caad.png
   :target: https://fmt.dev/unknown_mac64_clang10.0.html

Compile time and code bloat
~~~~~~~~~~~~~~~~~~~~~~~~~~~

The script `bloat-test.py
<https://github.com/fmtlib/format-benchmark/blob/master/bloat-test.py>`_
from `format-benchmark <https://github.com/fmtlib/format-benchmark>`_
tests compile time and code bloat for nontrivial projects.
It generates 100 translation units and uses ``printf()`` or its alternative
five times in each to simulate a medium sized project.  The resulting
executable size and compile time (Apple LLVM version 8.1.0 (clang-802.0.42),
macOS Sierra, best of three) is shown in the following tables.

**Optimized build (-O3)**

============= =============== ==================== ==================
Method        Compile Time, s Executable size, KiB Stripped size, KiB
============= =============== ==================== ==================
printf                    2.6                   29                 26
printf+string            16.4                   29                 26
iostreams                31.1                   59                 55
{fmt}                    19.0                   37                 34
Boost Format             91.9                  226                203
Folly Format            115.7                  101                 88
============= =============== ==================== ==================

As you can see, {fmt} has 60% less overhead in terms of resulting binary code
size compared to iostreams and comes pretty close to ``printf``. Boost Format
and Folly Format have the largest overheads.

``printf+string`` is the same as ``printf`` but with extra ``<string>``
include to measure the overhead of the latter.

**Non-optimized build**

============= =============== ==================== ==================
Method        Compile Time, s Executable size, KiB Stripped size, KiB
============= =============== ==================== ==================
printf                    2.2                   33                 30
printf+string            16.0                   33                 30
iostreams                28.3                   56                 52
{fmt}                    18.2                   59                 50
Boost Format             54.1                  365                303
Folly Format             79.9                  445                430
============= =============== ==================== ==================

``libc``, ``lib(std)c++`` and ``libfmt`` are all linked as shared libraries to
compare formatting function overhead only. Boost Format is a
header-only library so it doesn't provide any linkage options.

Running the tests
~~~~~~~~~~~~~~~~~

Please refer to `Building the library`__ for the instructions on how to build
the library and run the unit tests.

__ https://fmt.dev/latest/usage.html#building-the-library

Benchmarks reside in a separate repository,
`format-benchmarks <https://github.com/fmtlib/format-benchmark>`_,
so to run the benchmarks you first need to clone this repository and
generate Makefiles with CMake::

    $ git clone --recursive https://github.com/fmtlib/format-benchmark.git
    $ cd format-benchmark
    $ cmake .

Then you can run the speed test::

    $ make speed-test

or the bloat test::

    $ make bloat-test

Projects using this library
---------------------------

* `0 A.D. <https://play0ad.com/>`_: A free, open-source, cross-platform real-time
  strategy game

* `AMPL/MP <https://github.com/ampl/mp>`_:
  An open-source library for mathematical programming
  
* `AvioBook <https://www.aviobook.aero/en>`_: A comprehensive aircraft
  operations suite
  
* `Celestia <https://celestia.space/>`_: Real-time 3D visualization of space

* `Ceph <https://ceph.com/>`_: A scalable distributed storage system

* `ccache <https://ccache.dev/>`_: A compiler cache

* `CUAUV <http://cuauv.org/>`_: Cornell University's autonomous underwater
  vehicle

* `HarpyWar/pvpgn <https://github.com/pvpgn/pvpgn-server>`_:
  Player vs Player Gaming Network with tweaks

* `KBEngine <https://kbengine.org/>`_: An open-source MMOG server engine

* `Keypirinha <https://keypirinha.com/>`_: A semantic launcher for Windows

* `Kodi <https://kodi.tv/>`_ (formerly xbmc): Home theater software

* `Lifeline <https://github.com/peter-clark/lifeline>`_: A 2D game

* `Drake <https://drake.mit.edu/>`_: A planning, control, and analysis toolbox
  for nonlinear dynamical systems (MIT)

* `Envoy <https://lyft.github.io/envoy/>`_: C++ L7 proxy and communication bus
  (Lyft)

* `FiveM <https://fivem.net/>`_: a modification framework for GTA V

* `MongoDB <https://mongodb.com/>`_: Distributed document database

* `MongoDB Smasher <https://github.com/duckie/mongo_smasher>`_: A small tool to
  generate randomized datasets

* `OpenSpace <https://openspaceproject.com/>`_: An open-source astrovisualization
  framework

* `PenUltima Online (POL) <https://www.polserver.com/>`_:
  An MMO server, compatible with most Ultima Online clients

* `quasardb <https://www.quasardb.net/>`_: A distributed, high-performance,
  associative database

* `readpe <https://bitbucket.org/sys_dev/readpe>`_: Read Portable Executable

* `redis-cerberus <https://github.com/HunanTV/redis-cerberus>`_: A Redis cluster
  proxy

* `rpclib <http://rpclib.net/>`_: A modern C++ msgpack-RPC server and client
  library

* `Saddy <https://github.com/mamontov-cpp/saddy-graphics-engine-2d>`_:
  Small crossplatform 2D graphic engine

* `Salesforce Analytics Cloud <https://www.salesforce.com/analytics-cloud/overview/>`_:
  Business intelligence software

* `Scylla <https://www.scylladb.com/>`_: A Cassandra-compatible NoSQL data store
  that can handle 1 million transactions per second on a single server

* `Seastar <http://www.seastar-project.org/>`_: An advanced, open-source C++
  framework for high-performance server applications on modern hardware

* `spdlog <https://github.com/gabime/spdlog>`_: Super fast C++ logging library

* `Stellar <https://www.stellar.org/>`_: Financial platform

* `Touch Surgery <https://www.touchsurgery.com/>`_: Surgery simulator

* `TrinityCore <https://github.com/TrinityCore/TrinityCore>`_: Open-source
  MMORPG framework

`More... <https://github.com/search?q=fmtlib&type=Code>`_

If you are aware of other projects using this library, please let me know
by `email <mailto:victor.zverovich@gmail.com>`_ or by submitting an
`issue <https://github.com/fmtlib/fmt/issues>`_.

Motivation
----------

So why yet another formatting library?

There are plenty of methods for doing this task, from standard ones like
the printf family of function and iostreams to Boost Format and FastFormat
libraries. The reason for creating a new library is that every existing
solution that I found either had serious issues or didn't provide
all the features I needed.

printf
~~~~~~

The good thing about ``printf`` is that it is pretty fast and readily available
being a part of the C standard library. The main drawback is that it
doesn't support user-defined types. ``printf`` also has safety issues although
they are somewhat mitigated with `__attribute__ ((format (printf, ...))
<https://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html>`_ in GCC.
There is a POSIX extension that adds positional arguments required for
`i18n <https://en.wikipedia.org/wiki/Internationalization_and_localization>`_
to ``printf`` but it is not a part of C99 and may not be available on some
platforms.

iostreams
~~~~~~~~~

The main issue with iostreams is best illustrated with an example:

.. code:: c++

    std::cout << std::setprecision(2) << std::fixed << 1.23456 << "\n";

which is a lot of typing compared to printf:

.. code:: c++

    printf("%.2f\n", 1.23456);

Matthew Wilson, the author of FastFormat, called this "chevron hell". iostreams
don't support positional arguments by design.

The good part is that iostreams support user-defined types and are safe although
error handling is awkward.

Boost Format
~~~~~~~~~~~~

This is a very powerful library which supports both ``printf``-like format
strings and positional arguments. Its main drawback is performance. According to
various benchmarks it is much slower than other methods considered here. Boost
Format also has excessive build times and severe code bloat issues (see
`Benchmarks`_).

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

Boost Spirit.Karma
~~~~~~~~~~~~~~~~~~

This is not really a formatting library but I decided to include it here for
completeness. As iostreams, it suffers from the problem of mixing verbatim text
with arguments. The library is pretty fast, but slower on integer formatting
than ``fmt::format_int`` on Karma's own benchmark,
see `Fast integer to string conversion in C++
<http://zverovich.net/2013/09/07/integer-to-string-conversion-in-cplusplus.html>`_.

FAQ
---

Q: how can I capture formatting arguments and format them later?

A: use ``std::tuple``:

.. code:: c++

   template <typename... Args>
   auto capture(const Args&... args) {
     return std::make_tuple(args...);
   }

   auto print_message = [](const auto&... args) {
     fmt::print(args...);
   };

   // Capture and store arguments:
   auto args = capture("{} {}", 42, "foo");
   // Do formatting:
   std::apply(print_message, args);

License
-------

{fmt} is distributed under the MIT `license
<https://github.com/fmtlib/fmt/blob/master/LICENSE.rst>`_.

The `Format String Syntax
<https://fmt.dev/latest/syntax.html>`_
section in the documentation is based on the one from Python `string module
documentation <https://docs.python.org/3/library/string.html#module-string>`_
adapted for the current library. For this reason the documentation is
distributed under the Python Software Foundation license available in
`doc/python-license.txt
<https://raw.github.com/fmtlib/fmt/master/doc/python-license.txt>`_.
It only applies if you distribute the documentation of fmt.

Acknowledgments
---------------

The {fmt} library is maintained by Victor Zverovich (`vitaut
<https://github.com/vitaut>`_) and Jonathan Müller (`foonathan
<https://github.com/foonathan>`_) with contributions from many other people.
See `Contributors <https://github.com/fmtlib/fmt/graphs/contributors>`_ and
`Releases <https://github.com/fmtlib/fmt/releases>`_ for some of the names.
Let us know if your contribution is not listed or mentioned incorrectly and
we'll make it right.

The benchmark section of this readme file and the performance tests are taken
from the excellent `tinyformat <https://github.com/c42f/tinyformat>`_ library
written by Chris Foster.  Boost Format library is acknowledged transitively
since it had some influence on tinyformat.
Some ideas used in the implementation are borrowed from `Loki
<http://loki-lib.sourceforge.net/>`_ SafeFormat and `Diagnostic API
<https://clang.llvm.org/doxygen/classclang_1_1Diagnostic.html>`_ in
`Clang <https://clang.llvm.org/>`_.
Format string syntax and the documentation are based on Python's `str.format
<https://docs.python.org/3/library/stdtypes.html#str.format>`_.
Thanks `Doug Turnbull <https://github.com/softwaredoug>`_ for his valuable
comments and contribution to the design of the type-safe API and
`Gregory Czajkowski <https://github.com/gcflymoto>`_ for implementing binary
formatting. Thanks `Ruslan Baratov <https://github.com/ruslo>`_ for comprehensive
`comparison of integer formatting algorithms <https://github.com/ruslo/int-dec-format-tests>`_
and useful comments regarding performance, `Boris Kaul <https://github.com/localvoid>`_ for
`C++ counting digits benchmark <https://github.com/localvoid/cxx-benchmark-count-digits>`_.
Thanks to `CarterLi <https://github.com/CarterLi>`_ for contributing various
improvements to the code.
