Overview
========

C++ Format (cppformat) is an open-source formatting library for C++.
It can be used as a safe alternative to printf or as a fast
alternative to IOStreams.

.. raw:: html

   <div class="panel panel-default">
     <div class="panel-heading">What users say:</div>
     <div class="panel-body">
       Thanks for creating this library. It’s been a hole in C++ for a long time.
       I’ve used both boost::format and loki::SPrintf, and neither felt like the
       right answer. This does.
     </div>
   </div>

.. _format-api:

Format API
----------

The replacement-based Format API provides a safe alternative to ``printf``,
``sprintf`` and friends with comparable or `better performance
<http://zverovich.net/2013/09/07/integer-to-string-conversion-in-cplusplus.html>`_.
The `format string syntax <doc/latest/index.html#format-string-syntax>`_ is similar
to the one used by `str.format <http://docs.python.org/2/library/stdtypes.html#str.format>`_
in Python:

.. code:: c++

  fmt::format("The answer is {}", 42);
  
The ``fmt::format`` function returns a string "The answer is 42". You can use
``fmt::MemoryWriter`` to avoid constructing ``std::string``:

.. code:: c++

  fmt::MemoryWriter w;
  w.write("Look, a {} string", 'C');
  w.c_str(); // returns a C string (const char*)

The ``fmt::print`` function performs formatting and writes the result to a file:

.. code:: c++

  fmt::print(stderr, "System error code = {}\n", errno);

The file argument can be omitted in which case the function prints to
``stdout``:

.. code:: c++

  fmt::print("Don't {}\n", "panic");

If your compiler supports C++11, then the formatting functions are implemented
with variadic templates. Otherwise variadic functions are emulated by generating
a set of lightweight wrappers. This ensures compatibility with older compilers
while providing a natural API.

The Format API also supports positional arguments useful for localization:

.. code:: c++

  fmt::print("I'd rather be {1} than {0}.", "right", "happy");

.. _write-api:
  
Write API
---------

The concatenation-based Write API (experimental) provides a
`fast <http://zverovich.net/2013/09/07/integer-to-string-conversion-in-cplusplus.html>`_
stateless alternative to IOStreams:

.. code:: c++

  fmt::MemoryWriter out;
  out << "The answer in hexadecimal is " << hex(42);

.. _safety:

Safety
------

The library is fully type safe, automatic memory management prevents buffer overflow,
errors in format strings are reported using exceptions. For example, the code

.. code:: c++

  fmt::format("The answer is {:d}", "forty-two");

throws a ``FormatError`` exception with description
"unknown format code 'd' for string", because the argument
``"forty-two"`` is a string while the format code ``d``
only applies to integers.

Where possible, errors are caught at compile time. For example, the code

.. code:: c++

  fmt::format("Cyrillic letter {}", L'\x42e');
  
produces a compile-time error because wide character ``L'\x42e'`` cannot be
formatted into a narrow string. You can use a wide format string instead:

.. code:: c++

  fmt::format(L"Cyrillic letter {}", L'\x42e');

For comparison, writing a wide character to ``std::ostream`` results in
its numeric value being written to the stream (i.e. 1070 instead of letter 'ю' which
is represented by ``L'\x42e'`` if we use Unicode) which is rarely what is needed.

.. _portability:

Portability
-----------

C++ Format is highly portable. Here is an incomplete list of operating systems and
compilers where it has been tested and known to work:

* 64-bit (amd64) GNU/Linux with GCC 4.4.3, `4.6.3 <https://travis-ci.org/cppformat/cppformat>`_,
  4.7.2, 4.8.1 and Intel C++ Compiler (ICC) 14.0.2

* 32-bit (i386) GNU/Linux with GCC 4.4.3, 4.6.3

* Mac OS X with GCC 4.2.1 and Clang 4.2, 5.1.0

* 64-bit Windows with Visual C++ 2010 and
  `2013 <https://ci.appveyor.com/project/vitaut/cppformat>`_

* 32-bit Windows with Visual C++ 2010

Although the library uses C++11 features when available, it also works with older
compilers and standard library implementations.

The output of all formatting functions is consistent across platforms. In particular,
formatting a floating-point infinity always gives ``inf`` while the output
of ``printf`` is platform-dependent in this case. For example,

.. code::

  fmt::print("{}", std::numeric_limits<double>::infinity());

always prints ``inf``.

.. _ease-of-use:

Ease of Use
-----------

C++ Format has small self-contained code base consisting of a single header file
and a single source file and no external dependencies. A permissive BSD `license
<https://github.com/cppformat/cppformat#license>`_ allows using the library both
in open-source and commercial projects.

.. raw:: html

  <a class="btn btn-success" href="https://github.com/cppformat/cppformat">GitHub Repository</a>

  <div class="section footer">
    <iframe src="http://ghbtns.com/github-btn.html?user=cppformat&amp;repo=cppformat&amp;type=watch&amp;count=true"
            class="github-btn" width="100" height="20"></iframe>
  </div>
