format
======

Format is a C++ library that provides printf-like formatting functionality.

Features
--------

* Format string syntax similar to the one used by `str.format
  <http://docs.python.org/2/library/stdtypes.html#str.format>`__ in Python.
* Support for user-defined types.
* High speed: performance of the current proof-of-concept implementation
  is close to that of IOStreams (see `Speed tests`_).
* Small code size both in terms of source code (format consists of a single
  header file and a single source file) and compiled code
  (see `Compile time and code bloat`_).
* Easy deployment: small self-contained code base, no external dependencies,
  permissive license.

Examples
--------

This prints "Hello, world!" to stdout::

    fmt::Print("Hello, {0}!") << "world";

Arguments are accessed by position and arguments' indices can be repeated::

    std::string s = str(fmt::Format("{0}{1}{0}") << "abra" << "cad");
    // s == "abracadabra"

An object of any user-defined type for which there is an overloaded
``std::ostream`` insertion operator (``operator<<``) can be formatted::

    class Date {
      int year_, month_, day_;
     public:
      Date(int year, int month, int day) : year_(year), month_(month), day_(day) {}

      friend std::ostream &operator<<(std::ostream &os, const Date &d) {
        os << d.year_ << '-' << d.month_ << '-' << d.day_;
        return os;
      }
    };

    std::string s = str(fmt::Format("The date is {0}") << Date(2012, 12, 9));
    // s == "The date is 2012-12-9"

Format string syntax
--------------------

A format string can contain "replacement fields" delimited by curly
braces ``{}``.  Text outside of braces is copied unchanged to the output.
If you need to include a brace character in the literal text, it can be
escaped by doubling: ``{{`` and ``}}``.

The grammar for a replacement field is as follows::

   replacement_field: "{" arg_index [":" format_spec] "}"
   arg_index: integer
   format_spec: ["+"]["0"][width]["." precision][type]
   width: integer
   precision: integer
   type: "c" | "d" | "e" | "E" | "f" | "F" | "g" | "G" | "o" | "s" | "x" | "X"

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
being the part of the C standard library. The main drawback is that it
doesn't support user-defined types. Printf also has safety issues although
they are mostly solved with `__attribute__ ((format (printf, ...))
<http://gcc.gnu.org/onlinedocs/gcc/Function-Attributes.html>`__ in GCC.
There is a POSIX extension that adds positional arguments required for
`i18n <http://en.wikipedia.org/wiki/Internationalization_and_localization>`__
to printf but it is not a part of C99 and may not be available on some
platforms.

IOStreams
~~~~~~~~~

The main issue with IOStreams is best illustrated with an example::

    std::cout << std::setprecision(2) << std::fixed << 1.23456 << "\n";

which is a lot of typing compared to printf::

    printf("%.2f\n", 1.23456);

Matthew Wilson, the author of FastFormat referred to this situations with
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

TODO

Tinyformat
~~~~~~~~~~

This library supports printf-like format strings and is very small and
fast. Unfortunately it doesn't support positional arguments and wrapping
it in C++98 is somewhat difficult.  However if you only need a type-safe
printf replacement with support for user-defined types, I highly recommend
this library.

Benchmarks
----------

Compile time and code bloat
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Speed tests
~~~~~~~~~~~

The following speed tests results were generated by building
``tinyformat_test.cpp`` on Ubuntu GNU/Linux 12.10 with
``g++-4.7.2 -O3 -DSPEED_TEST -DHAVE_FORMAT``, and taking the best of three
runs.  In the test, the format string ``"%0.10f:%04d:%+g:%s:%p:%c:%%\n"`` or
equivalent is filled 2000000 times with output sent to ``/dev/null``; for
further details see the `source
<https://github.com/vitaut/tinyformat/blob/master/tinyformat_test.cpp>`__.

============== ========
test name      run time
============== ========
libc printf     1.26s
std::ostream    2.02s
format          2.20s
tinyformat      2.51s
boost::format  10.40s
============== ========

As you can see boost::format is much slower than the alternative methods; this
is confirmed by `other tests <http://accu.org/index.php/journals/1539>`__.
Tinyformat is quite good coming close to IOStreams.  Unfortunately tinyformat
cannot be faster than the IOStreams because it uses them internally.
Performance of format is close to that of std::ostream but there is a room for
improvement since format is not based on IOStreams.

The script ``bloat_test.sh`` from the `tinyformat
<https://github.com/c42f/tinyformat>`__ repository tests compile time and
code bloat for nontrivial projects.  It generates 100 translation units
and uses ``printf()`` or its alternative five times in each to simulate
a medium sized project.  The resulting executable size and compile time
(g++-4.7.2, Ubuntu GNU/Linux 12.10, best of three) is shown in the following
tables.

**Non-optimized build**

====================== ================== ==========================
test name              total compile time executable size (stripped)
====================== ================== ==========================
libc printf            2.8s               44K  (32K)
std::ostream           12.9s              84K  (60K)
format                 16.0s              152K (128K)
tinyformat             20.6s              240K (200K)
boost::format          76.0s              888K (780K)
====================== ================== ==========================

**Optimized build (-O3)**

====================== ================== ==========================
test name              total compile time executable size (stripped)
====================== ================== ==========================
libc printf            3.5s               40K  (28K)
std::ostream           14.1s              88K  (64K)
format                 25.1s              552K (536K)
tinyformat             56.3s              200K (164K)
boost::format          169.4s             1.7M (1.6M)
====================== ================== ==========================

Printf and std::ostream win here which is not surprising considering
that they are included in the standard library. Tinyformat has somewhat
slower compilation times compared to format. Interestingly optimized
executable size is smaller with tinyformat then with format and for
non-optimized build its the other way around. Boost::format has by far
the largest overheads.

Running the tests
~~~~~~~~~~~~~~~~~

To run the tests you first need to get the format repository with submodules::

    $ git clone --recursive git://github.com/vitaut/format.git

Then go to the format directory and generate Makefiles with
`CMake <http://www.cmake.org/>`__::

    $ cd format
    $ cmake .

Next use the following commands to run the speed test::

    $ make speed_test

or the bloat test::

    $ make bloat_test

Acknowledgments
---------------

The benchmark section of this readme file and the performance tests are taken
from the excellent `tinyformat <https://github.com/c42f/tinyformat>`__ library
written by Chris Foster.  Boost Format library is acknowledged transitively
since it had some influence on tinyformat.
Some ideas used in the implementation are borrowed from `Loki
<http://loki-lib.sourceforge.net/>`__ SafeFormat and `Diagnostic API
<http://clang.llvm.org/doxygen/classclang_1_1Diagnostic.html>`__ in
`Clang <http://clang.llvm.org/>`__.
