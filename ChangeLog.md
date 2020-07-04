7.0.0 - TBD
-----------

* Reduced the library size. For example, on macOS the stripped binary
  statically linked with {fmt} `shrank from ~368k to less than 100k
  <http://www.zverovich.net/2020/05/21/reducing-library-size.html>`_.

* Added a simpler and more efficient `format string compilation API
  <https://fmt.dev/dev/api.html#compile-api>`_:

  .. code:: c++

     #include <fmt/compile.h>

     // Converts 42 into std::string using the most efficient method and no
     // runtime format string processing.
     std::string s = fmt::format(FMT_COMPILE("{}"), 42);

  The old ``fmt::compile`` API is now deprecated.

* Optimized integer formatting: ``format_to`` with format string compilation
  and a stack-allocated buffer is now `faster than to_chars on both
  libc++ and libstdc++
  <http://www.zverovich.net/2020/06/13/fast-int-to-string-revisited.html>`_.

* Applied extern templates to improve compile times when using the core API
  and ``fmt/format.h`` (`#1452`_). For example, on macOS with clang the compile
  time of a test TU dropped from 2.3s to 0.3s with ``-O2`` and from 0.6s to 0.3s
  with the default settings (``-O0``).

  Before (``-O2``)::

    % time c++ -c test.cc -I include -std=c++17 -O2
    c++ -c test.cc -I include -std=c++17 -O2  2.22s user 0.08s system 99% cpu 2.311 total

  After (``-O2``)::

    % time c++ -c test.cc -I include -std=c++17 -O2
    c++ -c test.cc -I include -std=c++17 -O2  0.26s user 0.04s system 98% cpu 0.303 total

  Before (default)::

    % time c++ -c test.cc -I include -std=c++17
    c++ -c test.cc -I include -std=c++17  0.53s user 0.06s system 98% cpu 0.601 total

  After (default)::

    % time c++ -c test.cc -I include -std=c++17
    c++ -c test.cc -I include -std=c++17  0.24s user 0.06s system 98% cpu 0.301 total

  It is still recommended to use ``fmt/core.h`` instead of ``fmt/format.h`` but
  the compile time difference is now smaller.

  Thanks `@alex3d <https://github.com/alex3d>`_ for the suggestion.

* Named arguments are now stored on stack (no dynamic memory allocations) and
  the generated binary code is more compact and efficient. For example

  .. code:: c++

     #include <fmt/core.h>

     int main() {
       fmt::print("The answer is {answer}\n", fmt::arg("answer", 42));
     }

  compiles to just (`godbolt <https://godbolt.org/z/NcfEp_>`__)

  .. code:: asm

      .LC0:
              .string "answer"
      .LC1:
              .string "The answer is {answer}\n"
      main:
              sub     rsp, 56
              mov     edi, OFFSET FLAT:.LC1
              mov     esi, 23
              movabs  rdx, 4611686018427387905
              lea     rax, [rsp+32]
              lea     rcx, [rsp+16]
              mov     QWORD PTR [rsp+8], 1
              mov     QWORD PTR [rsp], rax
              mov     DWORD PTR [rsp+16], 42
              mov     QWORD PTR [rsp+32], OFFSET FLAT:.LC0
              mov     DWORD PTR [rsp+40], 0
              call    fmt::v6::vprint(fmt::v6::basic_string_view<char>,
                                      fmt::v6::format_args)
              xor     eax, eax
              add     rsp, 56
              ret

          .L.str.1:
                  .asciz  "answer"

* Implemented compile-time checks for dynamic width and precision (`#1614`_):

  .. code:: c++

     #include <fmt/format.h>

     int main() {
       fmt::print(FMT_STRING("{0:{1}}"), 42);
     }

  now gives a compilation error because argument 1 doesn't exist::

    In file included from test.cc:1:
    include/fmt/format.h:2726:27: error: constexpr variable 'invalid_format' must be
    initialized by a constant expression
      FMT_CONSTEXPR_DECL bool invalid_format =
                              ^
    ...
    include/fmt/core.h:569:26: note: in call to
    '&checker(s, {}).context_->on_error(&"argument not found"[0])'
        if (id >= num_args_) on_error("argument not found");
                            ^

* Added sentinel support to ``fmt::join`` (`#1689`_):

  .. code:: c++

    struct zstring_sentinel {};
    bool operator==(const char* p, zstring_sentinel) { return *p == '\0'; }
    bool operator!=(const char* p, zstring_sentinel) { return *p != '\0'; }

    struct zstring {
      const char* p;
      const char* begin() const { return p; }
      zstring_sentinel end() const { return {}; }
    };

    auto s = fmt::format("{}", fmt::join(zstring{"hello"}, "_"));
    // s == "h_e_l_l_o"

  Thanks `@BRevzin (Barry Revzin) <https://github.com/BRevzin>`_.

* Added support for named args, ``clear`` and ``reserve`` to
  ``dynamic_format_arg_store`` (`#1655`_, `#1663`_, `#1674`_, `#1677`_). Thanks
  `@vsolontsov-ll (Vladimir Solontsov) <https://github.com/vsolontsov-ll>`_.

* Added support for the ``'c'`` format specifier to integral types for
  compatibility with ``std::format`` (`#1652`_).

* Implemented the ``'L'`` format specifier for locale-specific formatting of
  floating-point numbers for compatibility with ``std::format`` (`#1624`_).
  The ``'n'`` specifier is now disabled by default but can be enabled via the
  ``FMT_DEPRECATED_N_SPECIFIER`` macro.

* The ``'='`` format specifier is now disabled by default for compatibility with
  ``std::format``. It can be enabled via the ``FMT_DEPRECATED_NUMERIC_ALIGN``
  macro.

* Optimized handling of small format strings. For example,

  .. code:: c++

      fmt::format("Result: {}: ({},{},{},{})", str1, str2, str3, str4, str5)

  is now ~40% faster (`#1685`_).

* Improved compatibility between ``fmt::printf`` with the standard specs
  (`#1595`_, `#1682`_, `#1683`_, `#1687`_, `#1699`_, `#1717`_).
  Thanks `@rimathia <https://github.com/rimathia>`_.

* Fixed handling of ``operator<<` overloads that use ``copyfmt`` (`#1666`_).

* Removed the following deprecated APIs:

  * ``FMT_STRING_ALIAS`` and ``fmt`` macros - replaced by ``FMT_STRING``
  * ``fmt::basic_string_view::char_type`` - replaced by
    ``fmt::basic_string_view::value_type``
  * ``convert_to_int``
  * ``format_arg_store::types``
  * ``*parse_context`` - replaced by ``*format_parse_context``
  * ``FMT_DEPRECATED_INCLUDE_OS``
  * ``FMT_DEPRECATED_PERCENT`` - incompatible with ``std::format``
  * ``*writer`` - replaced by compiled format API

* Deprecated ``arg_formatter``.

* Renamed the ``internal`` namespace to ``detail`` (`#1538`_). The former is
  still provided as an alias if the ``FMT_USE_INTERNAL`` macro is defined.

* Added the ``FMT_OS`` CMake option to control inclusion of OS-specific APIs
  in the fmt target. This can be useful for embedded platforms
  (`#1654`_, `#1656`_).
  Thanks `@kwesolowski (Krzysztof Wesolowski)
  <https://github.com/kwesolowski>`_.

* Replaced ``FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION`` with the ``FMT_FUZZ``
  macro to prevent interferring with fuzzing of projects using {fmt} (`#1650`_).
  Thanks `@asraa (Asra Ali) <https://github.com/asraa>`_.

* Fixed compatibility with emscripten (`#1736`_, `#1737`_).
  Thanks `@ArthurSonzogni (Arthur Sonzogni)
  <https://github.com/ArthurSonzogni>`_.

* Improved documentation (`#704`_, `#1643`_, `#1660`_, `#1681`_, `#1691`_,
  `#1706`_, `#1714`_, `#1721`_, `#1739`_, `#1740`_, `#1741`_).
  Thanks `@senior7515 (Alexander Gallego) <https://github.com/senior7515>`_,
  `@lsr0 (Lindsay Roberts) <https://github.com/lsr0>`_,
  `@puetzk (Kevin Puetz) <https://github.com/puetzk>`_,
  `@fpelliccioni (Fernando Pelliccioni) <https://github.com/fpelliccioni>`_,
  Alexey Kuzmenko, `@jelly (jelle van der Waa) <https://github.com/jelly>`_,
  `@claremacrae (Clare Macrae) <https://github.com/claremacrae>`_,
  `@jiapengwen (文佳鹏) <https://github.com/jiapengwen>`_,
  `@gsjaardema (Greg Sjaardema) <https://github.com/gsjaardema>`_.

* Implemented various build configuration fixes and improvements
  (`#1603`_, `#1657`_, `#1702`_, `#1728`_).
  Thanks `@scramsby (Scott Ramsby) <https://github.com/scramsby>`_,
  `@jtojnar (Jan Tojnar) <https://github.com/jtojnar>`_,
  `@orivej (Orivej Desh) <https://github.com/orivej>`_,
  `@flagarde <https://github.com/flagarde>`_.

* Fixed various warnings and compilation issues (`#1616`_, `#1620`_,
  `#1622`_, `#1625`_, `#1627`_, `#1628`_, `#1629`_, `#1631`_, `#1633`_,
  `#1649`_, `#1658`_, `#1661`_, `#1667`_, `#1668`_, `#1669`_, `#1692`_,
  `#1696`_, `#1697`_, `#1707`_, `#1712`_, `#1716`_, `#1722`_, `#1724`_,
  `#1729`_, `#1738`_, `#1742`_, `#1743`_, `#1744`_, `#1747`_, `#1750`_).
  Thanks `@gsjaardema (Greg Sjaardema) <https://github.com/gsjaardema>`_,
  `@gabime (Gabi Melman) <https://github.com/gabime>`_,
  `@johnor (Johan) <https://github.com/johnor>`_,
  `@gabime (Dmitry Kurkin) <https://github.com/Kurkin>`_,
  `@invexed (James Beach) <https://github.com/invexed>`_,
  `@peterbell10 <https://github.com/peterbell10>`_,
  `@daixtrose (Markus Werle) <https://github.com/daixtrose>`_,
  `@petrutlucian94 (Lucian Petrut) <https://github.com/petrutlucian94>`_,
  `@Neargye (Daniil Goncharov) <https://github.com/Neargye>`_,
  `@ambitslix (Attila M. Szilagyi) <https://github.com/ambitslix>`_,
  `@gabime (Gabi Melman) <https://github.com/gabime>`_,
  `@erthink (Leonid Yuriev) <https://github.com/erthink>`_,
  `@tohammer (Tobias Hammer ) <https://github.com/tohammer>`_,
  `@0x8000-0000 (Florin Iucha) <https://github.com/0x8000-0000>`_.

6.2.1 - 2020-05-09
------------------

* Fixed ostream support in ``sprintf`` (`#1631`_).

* Fixed type detection when using implicit conversion to ``string_view`` and
  ostream ``operator<<`` inconsistently (`#1662`_).

6.2.0 - 2020-04-05
------------------

* Improved error reporting when trying to format an object of a non-formattable
  type:

  .. code:: c++

     fmt::format("{}", S());

  now gives::

    include/fmt/core.h:1015:5: error: static_assert failed due to requirement
    'formattable' "Cannot format argument. To make type T formattable provide a
    formatter<T> specialization:
    https://fmt.dev/latest/api.html#formatting-user-defined-types"
        static_assert(
        ^
    ...
    note: in instantiation of function template specialization
    'fmt::v6::format<char [3], S, char>' requested here
      fmt::format("{}", S());
           ^

  if ``S`` is not formattable.

* Reduced the library size by ~10%.

* Always print decimal point if ``#`` is specified (`#1476`_, `#1498`_):

  .. code:: c++

     fmt::print("{:#.0f}", 42.0);

  now prints ``42.``

* Implemented the ``'L'`` specifier for locale-specific numeric formatting to
  improve compatibility with ``std::format``. The ``'n'`` specifier is now
  deprecated and will be removed in the next major release.

* Moved OS-specific APIs such as ``windows_error`` from ``fmt/format.h`` to
  ``fmt/os.h``. You can define ``FMT_DEPRECATED_INCLUDE_OS`` to automatically
  include ``fmt/os.h`` from ``fmt/format.h`` for compatibility but this will be
  disabled in the next major release.

* Added precision overflow detection in floating-point formatting.

* Implemented detection of invalid use of ``fmt::arg``.

* Used ``type_identity`` to block unnecessary template argument deduction.
  Thanks Tim Song.

* Improved UTF-8 handling (`#1109`_):

  .. code:: c++

     fmt::print("┌{0:─^{2}}┐\n"
                "│{1: ^{2}}│\n"
                "└{0:─^{2}}┘\n", "", "Привет, мир!", 20);

  now prints::

     ┌────────────────────┐
     │    Привет, мир!    │
     └────────────────────┘

  on systems that support Unicode.

* Added experimental dynamic argument storage (`#1170`_, `#1584`_):

  .. code:: c++

     fmt::dynamic_format_arg_store<fmt::format_context> store;
     store.push_back("answer");
     store.push_back(42);
     fmt::vprint("The {} is {}.\n", store);
  
  prints::

     The answer is 42.

  Thanks `@vsolontsov-ll (Vladimir Solontsov)
  <https://github.com/vsolontsov-ll>`_.

* Made ``fmt::join`` accept ``initializer_list`` (`#1591`_).
  Thanks `@Rapotkinnik (Nikolay Rapotkin) <https://github.com/Rapotkinnik>`_.

* Fixed handling of empty tuples (`#1588`_).

* Fixed handling of output iterators in ``format_to_n`` (`#1506`_).

* Fixed formatting of ``std::chrono::duration`` types to wide output (`#1533`_).
  Thanks `@zeffy (pilao) <https://github.com/zeffy>`_.

* Added const ``begin`` and ``end`` overload to buffers (`#1553`_).
  Thanks `@dominicpoeschko <https://github.com/dominicpoeschko>`_.

* Added the ability to disable floating-point formatting via ``FMT_USE_FLOAT``,
  ``FMT_USE_DOUBLE`` and ``FMT_USE_LONG_DOUBLE`` macros for extremely
  memory-constrained embedded system (`#1590`_).
  Thanks `@albaguirre (Alberto Aguirre) <https://github.com/albaguirre>`_.

* Made ``FMT_STRING`` work with ``constexpr`` ``string_view`` (`#1589`_).
  Thanks `@scramsby (Scott Ramsby) <https://github.com/scramsby>`_.

* Implemented a minor optimization in the format string parser (`#1560`_).
  Thanks `@IkarusDeveloper <https://github.com/IkarusDeveloper>`_.

* Improved attribute detection (`#1469`_, `#1475`_, `#1576`_).
  Thanks `@federico-busato (Federico) <https://github.com/federico-busato>`_,
  `@chronoxor (Ivan Shynkarenka) <https://github.com/chronoxor>`_,
  `@refnum <https://github.com/refnum>`_.

* Improved documentation (`#1481`_, `#1523`_).
  Thanks `@JackBoosY (Jack·Boos·Yu) <https://github.com/JackBoosY>`_,
  `@imba-tjd (谭九鼎) <https://github.com/imba-tjd>`_.

* Fixed symbol visibility on Linux when compiling with ``-fvisibility=hidden``
  (`#1535`_). Thanks `@milianw (Milian Wolff) <https://github.com/milianw>`_.

* Implemented various build configuration fixes and improvements
  (`#1264`_, `#1460`_, `#1534`_, `#1536`_, `#1545`_, `#1546`_, `#1566`_,
  `#1582`_, `#1597`_, `#1598`_).
  Thanks `@ambitslix (Attila M. Szilagyi) <https://github.com/ambitslix>`_,
  `@jwillikers (Jordan Williams) <https://github.com/jwillikers>`_,
  `@stac47 (Laurent Stacul) <https://github.com/stac47>`_.

* Fixed various warnings and compilation issues
  (`#1433`_, `#1461`_, `#1470`_, `#1480`_, `#1485`_, `#1492`_, `#1493`_,
  `#1504`_, `#1505`_, `#1512`_, `#1515`_, `#1516`_, `#1518`_, `#1519`_,
  `#1520`_, `#1521`_, `#1522`_, `#1524`_, `#1530`_, `#1531`_, `#1532`_,
  `#1539`_, `#1547`_, `#1548`_, `#1554`_, `#1567`_, `#1568`_, `#1569`_,
  `#1571`_, `#1573`_, `#1575`_, `#1581`_, `#1583`_, `#1586`_, `#1587`_,
  `#1594`_, `#1596`_, `#1604`_, `#1606`_, `#1607`_, `#1609`_).
  Thanks `@marti4d (Chris Martin) <https://github.com/marti4d>`_,
  `@iPherian <https://github.com/iPherian>`_,
  `@parkertomatoes <https://github.com/parkertomatoes>`_,
  `@gsjaardema (Greg Sjaardema) <https://github.com/gsjaardema>`_,
  `@chronoxor (Ivan Shynkarenka) <https://github.com/chronoxor>`_,
  `@DanielaE (Daniela Engert) <https://github.com/DanielaE>`_,
  `@torsten48 <https://github.com/torsten48>`_,
  `@tohammer (Tobias Hammer) <https://github.com/tohammer>`_,
  `@lefticus (Jason Turner) <https://github.com/lefticus>`_,
  `@ryusakki (Haise) <https://github.com/ryusakki>`_,
  `@adnsv (Alex Denisov) <https://github.com/adnsv>`_,
  `@fghzxm <https://github.com/fghzxm>`_,
  `@refnum <https://github.com/refnum>`_,
  `@pramodk (Pramod Kumbhar) <https://github.com/pramodk>`_,
  `@Spirrwell <https://github.com/Spirrwell>`_,
  `@scramsby (Scott Ramsby) <https://github.com/scramsby>`_.

6.1.2 - 2019-12-11
------------------

* Fixed ABI compatibility with ``libfmt.so.6.0.0`` (`#1471`_).

* Fixed handling types convertible to ``std::string_view`` (`#1451`_).
  Thanks `@denizevrenci (Deniz Evrenci) <https://github.com/denizevrenci>`_.

* Made CUDA test an opt-in enabled via the ``FMT_CUDA_TEST`` CMake option.

* Fixed sign conversion warnings (`#1440`_).
  Thanks `@0x8000-0000 (Florin Iucha) <https://github.com/0x8000-0000>`_.

6.1.1 - 2019-12-04
------------------

* Fixed shared library build on Windows (`#1443`_, `#1445`_, `#1446`_,
  `#1450`_). Thanks `@egorpugin (Egor Pugin) <https://github.com/egorpugin>`_,
  `@bbolli (Beat Bolli) <https://github.com/bbolli>`_.

* Added a missing decimal point in exponent notation with trailing zeros.

* Removed deprecated ``format_arg_store::TYPES``.

6.1.0 - 2019-12-01
------------------

* {fmt} now formats IEEE 754 ``float`` and ``double`` using the shortest decimal
  representation with correct rounding by default:

  .. code:: c++

     #include <cmath>
     #include <fmt/core.h>

     int main() {
       fmt::print("{}", M_PI);
     }

  prints ``3.141592653589793``.

* Made the fast binary to decimal floating-point formatter the default,
  simplified it and improved performance. {fmt} is now 15 times faster than
  libc++'s ``std::ostringstream``, 11 times faster than ``printf`` and 10%
  faster than double-conversion on `dtoa-benchmark
  <https://github.com/fmtlib/dtoa-benchmark>`_:

  ==================  =========  =======
  Function            Time (ns)  Speedup
  ==================  =========  =======
  ostringstream        1,346.30    1.00x
  ostrstream           1,195.74    1.13x
  sprintf                995.08    1.35x
  doubleconv              99.10   13.59x
  fmt                     88.34   15.24x
  ==================  =========  =======

  .. image:: https://user-images.githubusercontent.com/576385/
             69767160-cdaca400-112f-11ea-9fc5-347c9f83caad.png

* {fmt} no longer converts ``float`` arguments to ``double``. In particular this
  improves the default (shortest) representation of floats and makes
  ``fmt::format`` consistent with ``std::format`` specs
  (`#1336`_, `#1353`_, `#1360`_, `#1361`_):

  .. code:: c++

     fmt::print("{}", 0.1f);

  prints ``0.1`` instead of ``0.10000000149011612``.

  Thanks `@orivej (Orivej Desh) <https://github.com/orivej>`_.

* Made floating-point formatting output consistent with ``printf``/iostreams
  (`#1376`_, `#1417`_).

* Added support for 128-bit integers (`#1287`_):

  .. code:: c++

     fmt::print("{}", std::numeric_limits<__int128_t>::max());

  prints ``170141183460469231731687303715884105727``.

  Thanks `@denizevrenci (Deniz Evrenci) <https://github.com/denizevrenci>`_.

* The overload of ``print`` that takes ``text_style`` is now atomic, i.e. the
  output from different threads doesn't interleave (`#1351`_).
  Thanks `@tankiJong (Tanki Zhang) <https://github.com/tankiJong>`_.

* Made compile time in the header-only mode ~20% faster by reducing the number
  of template instantiations. ``wchar_t`` overload of ``vprint`` was moved from
  ``fmt/core.h`` to ``fmt/format.h``.

* Added an overload of ``fmt::join`` that works with tuples
  (`#1322`_, `#1330`_):

  .. code:: c++

     #include <tuple>
     #include <fmt/ranges.h>

     int main() {
       std::tuple<char, int, float> t{'a', 1, 2.0f};
       fmt::print("{}", t);
     }

  prints ``('a', 1, 2.0)``.

  Thanks `@jeremyong (Jeremy Ong) <https://github.com/jeremyong>`_.

* Changed formatting of octal zero with prefix from "00" to "0":

  .. code:: c++

     fmt::print("{:#o}", 0);

  prints ``0``.

* The locale is now passed to ostream insertion (``<<``) operators (`#1406`_):

  .. code:: c++

     #include <fmt/locale.h>
     #include <fmt/ostream.h>

     struct S {
       double value;
     };

     std::ostream& operator<<(std::ostream& os, S s) {
       return os << s.value;
     }

     int main() {
       auto s = fmt::format(std::locale("fr_FR.UTF-8"), "{}", S{0.42});
       // s == "0,42"
     }

  Thanks `@dlaugt (Daniel Laügt) <https://github.com/dlaugt>`_.

* Locale-specific number formatting now uses grouping (`#1393`_, `#1394`_).
  Thanks `@skrdaniel <https://github.com/skrdaniel>`_.

* Fixed handling of types with deleted implicit rvalue conversion to
  ``const char**`` (`#1421`_):

  .. code:: c++

     struct mystring {
       operator const char*() const&;
       operator const char*() &;
       operator const char*() const&& = delete;
       operator const char*() && = delete;
     };
     mystring str;
     fmt::print("{}", str); // now compiles

* Enums are now mapped to correct underlying types instead of ``int``
  (`#1286`_). Thanks `@agmt (Egor Seredin) <https://github.com/agmt>`_.

* Enum classes are no longer implicitly converted to ``int`` (`#1424`_).

* Added ``basic_format_parse_context`` for consistency with C++20
  ``std::format`` and deprecated ``basic_parse_context``.

* Fixed handling of UTF-8 in precision (`#1389`_, `#1390`_).
  Thanks `@tajtiattila (Attila Tajti) <https://github.com/tajtiattila>`_.

* {fmt} can now be installed on Linux, macOS and Windows with
  `Conda <https://docs.conda.io/en/latest/>`__ using its
  `conda-forge <https://conda-forge.org>`__
  `package <https://github.com/conda-forge/fmt-feedstock>`__ (`#1410`_)::

    conda install -c conda-forge fmt

  Thanks `@tdegeus (Tom de Geus) <https://github.com/tdegeus>`_.

* Added a CUDA test (`#1285`_, `#1317`_).
  Thanks `@luncliff (Park DongHa) <https://github.com/luncliff>`_ and
  `@risa2000 <https://github.com/risa2000>`_.

* Improved documentation (`#1276`_, `#1291`_, `#1296`_, `#1315`_, `#1332`_,
  `#1337`_, `#1395`_ `#1418`_). Thanks
  `@waywardmonkeys (Bruce Mitchener) <https://github.com/waywardmonkeys>`_,
  `@pauldreik (Paul Dreik) <https://github.com/pauldreik>`_,
  `@jackoalan (Jack Andersen) <https://github.com/jackoalan>`_.

* Various code improvements (`#1358`_, `#1407`_).
  Thanks `@orivej (Orivej Desh) <https://github.com/orivej>`_,
  `@dpacbach (David P. Sicilia) <https://github.com/dpacbach>`_,

* Fixed compile-time format string checks for user-defined types (`#1292`_).

* Worked around a false positive in ``unsigned-integer-overflow`` sanitizer
  (`#1377`_).

* Fixed various warnings and compilation issues
  (`#1273`_, `#1278`_, `#1280`_, `#1281`_, `#1288`_, `#1290`_, `#1301`_,
  `#1305`_, `#1306`_, `#1309`_, `#1312`_, `#1313`_, `#1316`_, `#1319`_,
  `#1320`_, `#1326`_, `#1328`_, `#1344`_, `#1345`_, `#1347`_, `#1349`_,
  `#1354`_, `#1362`_, `#1366`_, `#1364`_, `#1370`_, `#1371`_, `#1385`_,
  `#1388`_, `#1397`_, `#1414`_, `#1416`_, `#1422`_ `#1427`_, `#1431`_,
  `#1433`_).
  Thanks `@hhb <https://github.com/hhb>`_,
  `@gsjaardema (Greg Sjaardema) <https://github.com/gsjaardema>`_,
  `@gabime (Gabi Melman) <https://github.com/gabime>`_,
  `@neheb (Rosen Penev) <https://github.com/neheb>`_,
  `@vedranmiletic (Vedran Miletić) <https://github.com/vedranmiletic>`_,
  `@dkavolis (Daumantas Kavolis) <https://github.com/dkavolis>`_,
  `@mwinterb <https://github.com/mwinterb>`_,
  `@orivej (Orivej Desh) <https://github.com/orivej>`_,
  `@denizevrenci (Deniz Evrenci) <https://github.com/denizevrenci>`_
  `@leonklingele <https://github.com/leonklingele>`_,
  `@chronoxor (Ivan Shynkarenka) <https://github.com/chronoxor>`_,
  `@kent-tri <https://github.com/kent-tri>`_,
  `@0x8000-0000 (Florin Iucha) <https://github.com/0x8000-0000>`_,
  `@marti4d (Chris Martin) <https://github.com/marti4d>`_.

6.0.0 - 2019-08-26
------------------

* Switched to the `MIT license
  <https://github.com/fmtlib/fmt/blob/5a4b24613ba16cc689977c3b5bd8274a3ba1dd1f/LICENSE.rst>`_
  with an optional exception that allows distributing binary code without
  attribution.

* Floating-point formatting is now locale-independent by default:

  .. code:: c++

     #include <locale>
     #include <fmt/core.h>

     int main() {
       std::locale::global(std::locale("ru_RU.UTF-8"));
       fmt::print("value = {}", 4.2);
     }

  prints "value = 4.2" regardless of the locale.

  For locale-specific formatting use the ``n`` specifier:

  .. code:: c++

     std::locale::global(std::locale("ru_RU.UTF-8"));
     fmt::print("value = {:n}", 4.2);

  prints "value = 4,2".

* Added an experimental Grisu floating-point formatting algorithm
  implementation (disabled by default). To enable it compile with the
  ``FMT_USE_GRISU`` macro defined to 1:

  .. code:: c++

     #define FMT_USE_GRISU 1
     #include <fmt/format.h>

     auto s = fmt::format("{}", 4.2); // formats 4.2 using Grisu

  With Grisu enabled, {fmt} is 13x faster than ``std::ostringstream`` (libc++)
  and 10x faster than ``sprintf`` on `dtoa-benchmark
  <https://github.com/fmtlib/dtoa-benchmark>`_ (`full results
  <https://fmt.dev/unknown_mac64_clang10.0.html>`_):

  .. image:: https://user-images.githubusercontent.com/576385/
             54883977-9fe8c000-4e28-11e9-8bde-272d122e7c52.jpg

* Separated formatting and parsing contexts for consistency with
  `C++20 std::format <http://eel.is/c++draft/format>`_, removing the
  undocumented ``basic_format_context::parse_context()`` function.

* Added `oss-fuzz <https://github.com/google/oss-fuzz>`_ support (`#1199`_).
  Thanks `@pauldreik (Paul Dreik) <https://github.com/pauldreik>`_.

* ``formatter`` specializations now always take precedence over ``operator<<``
  (`#952`_):

  .. code:: c++

     #include <iostream>
     #include <fmt/ostream.h>

     struct S {};

     std::ostream& operator<<(std::ostream& os, S) {
       return os << 1;
     }

     template <>
     struct fmt::formatter<S> : fmt::formatter<int> {
       auto format(S, format_context& ctx) {
         return formatter<int>::format(2, ctx);
       }
     };

     int main() {
       std::cout << S() << "\n"; // prints 1 using operator<<
       fmt::print("{}\n", S());  // prints 2 using formatter
     }

* Introduced the experimental ``fmt::compile`` function that does format string
  compilation (`#618`_, `#1169`_, `#1171`_):

  .. code:: c++

     #include <fmt/compile.h>

     auto f = fmt::compile<int>("{}");
     std::string s = fmt::format(f, 42); // can be called multiple times to
                                         // format different values
     // s == "42"

  It moves the cost of parsing a format string outside of the format function
  which can be beneficial when identically formatting many objects of the same
  types. Thanks `@stryku (Mateusz Janek) <https://github.com/stryku>`_.

* Added experimental ``%`` format specifier that formats floating-point values
  as percentages (`#1060`_, `#1069`_, `#1071`_):

  .. code:: c++

     auto s = fmt::format("{:.1%}", 0.42); // s == "42.0%"

  Thanks `@gawain-bolton (Gawain Bolton) <https://github.com/gawain-bolton>`_.

* Implemented precision for floating-point durations (`#1004`_, `#1012`_):

  .. code:: c++

     auto s = fmt::format("{:.1}", std::chrono::duration<double>(1.234));
     // s == 1.2s

  Thanks `@DanielaE (Daniela Engert) <https://github.com/DanielaE>`_.

* Implemented ``chrono`` format specifiers ``%Q`` and ``%q`` that give the value
  and the unit respectively (`#1019`_):

  .. code:: c++

     auto value = fmt::format("{:%Q}", 42s); // value == "42"
     auto unit  = fmt::format("{:%q}", 42s); // unit == "s"

  Thanks `@DanielaE (Daniela Engert) <https://github.com/DanielaE>`_.

* Fixed handling of dynamic width in chrono formatter:

  .. code:: c++

     auto s = fmt::format("{0:{1}%H:%M:%S}", std::chrono::seconds(12345), 12);
     //                        ^ width argument index                     ^ width
     // s == "03:25:45    "

  Thanks Howard Hinnant.

* Removed deprecated ``fmt/time.h``. Use ``fmt/chrono.h`` instead.

* Added ``fmt::format`` and ``fmt::vformat`` overloads that take ``text_style``
  (`#993`_, `#994`_):

  .. code:: c++

     #include <fmt/color.h>

     std::string message = fmt::format(fmt::emphasis::bold | fg(fmt::color::red),
                                       "The answer is {}.", 42);

  Thanks `@Naios (Denis Blank) <https://github.com/Naios>`_.

* Removed the deprecated color API (``print_colored``). Use the new API, namely
  ``print`` overloads that take ``text_style`` instead.

* Made ``std::unique_ptr`` and ``std::shared_ptr`` formattable as pointers via
  ``fmt::ptr`` (`#1121`_):

  .. code:: c++

     std::unique_ptr<int> p = ...;
     fmt::print("{}", fmt::ptr(p)); // prints p as a pointer

  Thanks `@sighingnow (Tao He) <https://github.com/sighingnow>`_.

* Made ``print`` and ``vprint`` report I/O errors (`#1098`_, `#1099`_).
  Thanks `@BillyDonahue (Billy Donahue) <https://github.com/BillyDonahue>`_.

* Marked deprecated APIs with the ``[[deprecated]]`` attribute and removed
  internal uses of deprecated APIs (`#1022`_).
  Thanks `@eliaskosunen (Elias Kosunen) <https://github.com/eliaskosunen>`_.

* Modernized the codebase using more C++11 features and removing workarounds.
  Most importantly, ``buffer_context`` is now an alias template, so
  use ``buffer_context<T>`` instead of ``buffer_context<T>::type``.
  These features require GCC 4.8 or later.

* ``formatter`` specializations now always take precedence over implicit
  conversions to ``int`` and the undocumented ``convert_to_int`` trait
  is now deprecated.

* Moved the undocumented ``basic_writer``, ``writer``, and ``wwriter`` types
  to the ``internal`` namespace.

* Removed deprecated ``basic_format_context::begin()``. Use ``out()`` instead.

* Disallowed passing the result of ``join`` as an lvalue to prevent misuse.

* Refactored the undocumented structs that represent parsed format specifiers
  to simplify the API and allow multibyte fill.

* Moved SFINAE to template parameters to reduce symbol sizes.

* Switched to ``fputws`` for writing wide strings so that it's no longer
  required to call ``_setmode`` on Windows (`#1229`_, `#1243`_).
  Thanks `@jackoalan (Jack Andersen) <https://github.com/jackoalan>`_.

* Improved literal-based API (`#1254`_).
  Thanks `@sylveon (Charles Milette) <https://github.com/sylveon>`_.

* Added support for exotic platforms without ``uintptr_t`` such as IBM i
  (AS/400) which has 128-bit pointers and only 64-bit integers (`#1059`_).

* Added `Sublime Text syntax highlighting config
  <https://github.com/fmtlib/fmt/blob/master/support/C%2B%2B.sublime-syntax>`_
  (`#1037`_).
  Thanks `@Kronuz (Germán Méndez Bravo) <https://github.com/Kronuz>`_.

* Added the ``FMT_ENFORCE_COMPILE_STRING`` macro to enforce the use of
  compile-time format strings (`#1231`_).
  Thanks `@jackoalan (Jack Andersen) <https://github.com/jackoalan>`_.

* Stopped setting ``CMAKE_BUILD_TYPE`` if {fmt} is a subproject (`#1081`_).

* Various build improvements (`#1039`_, `#1078`_, `#1091`_, `#1103`_, `#1177`_).
  Thanks `@luncliff (Park DongHa) <https://github.com/luncliff>`_,
  `@jasonszang (Jason Shuo Zang) <https://github.com/jasonszang>`_,
  `@olafhering (Olaf Hering) <https://github.com/olafhering>`_,
  `@Lecetem <https://github.com/Lectem>`_,
  `@pauldreik (Paul Dreik) <https://github.com/pauldreik>`_.

* Improved documentation (`#1049`_, `#1051`_, `#1083`_, `#1113`_, `#1114`_,
  `#1146`_, `#1180`_, `#1250`_, `#1252`_, `#1265`_).
  Thanks `@mikelui (Michael Lui) <https://github.com/mikelui>`_,
  `@foonathan (Jonathan Müller) <https://github.com/foonathan>`_,
  `@BillyDonahue (Billy Donahue) <https://github.com/BillyDonahue>`_,
  `@jwakely (Jonathan Wakely) <https://github.com/jwakely>`_,
  `@kaisbe (Kais Ben Salah) <https://github.com/kaisbe>`_,
  `@sdebionne (Samuel Debionne) <https://github.com/sdebionne>`_.

* Fixed ambiguous formatter specialization in ``fmt/ranges.h`` (`#1123`_).

* Fixed formatting of a non-empty ``std::filesystem::path`` which is an
  infinitely deep range of its components (`#1268`_).

* Fixed handling of general output iterators when formatting characters
  (`#1056`_, `#1058`_).
  Thanks `@abolz (Alexander Bolz) <https://github.com/abolz>`_.

* Fixed handling of output iterators in ``formatter`` specialization for
  ranges (`#1064`_).

* Fixed handling of exotic character types (`#1188`_).

* Made chrono formatting work with exceptions disabled (`#1062`_).

* Fixed DLL visibility issues (`#1134`_, `#1147`_).
  Thanks `@denchat <https://github.com/denchat>`_.

* Disabled the use of UDL template extension on GCC 9 (`#1148`_).

* Removed misplaced ``format`` compile-time checks from ``printf`` (`#1173`_).

* Fixed issues in the experimental floating-point formatter
  (`#1072`_, `#1129`_, `#1153`_, `#1155`_, `#1210`_, `#1222`_).
  Thanks `@alabuzhev (Alex Alabuzhev) <https://github.com/alabuzhev>`_.

* Fixed bugs discovered by fuzzing or during fuzzing integration
  (`#1124`_, `#1127`_, `#1132`_, `#1135`_, `#1136`_, `#1141`_, `#1142`_,
  `#1178`_, `#1179`_, `#1194`_).
  Thanks `@pauldreik (Paul Dreik) <https://github.com/pauldreik>`_.

* Fixed building tests on FreeBSD and Hurd (`#1043`_).
  Thanks `@jackyf (Eugene V. Lyubimkin) <https://github.com/jackyf>`_.

* Fixed various warnings and compilation issues (`#998`_, `#1006`_, `#1008`_,
  `#1011`_, `#1025`_, `#1027`_, `#1028`_, `#1029`_, `#1030`_, `#1031`_,
  `#1054`_, `#1063`_, `#1068`_, `#1074`_, `#1075`_, `#1079`_, `#1086`_,
  `#1088`_, `#1089`_, `#1094`_, `#1101`_, `#1102`_, `#1105`_, `#1107`_,
  `#1115`_, `#1117`_, `#1118`_, `#1120`_, `#1123`_, `#1139`_, `#1140`_,
  `#1143`_, `#1144`_, `#1150`_, `#1151`_, `#1152`_, `#1154`_, `#1156`_,
  `#1159`_, `#1175`_, `#1181`_, `#1186`_, `#1187`_, `#1191`_, `#1197`_,
  `#1200`_, `#1203`_, `#1205`_, `#1206`_, `#1213`_, `#1214`_, `#1217`_,
  `#1228`_, `#1230`_, `#1232`_, `#1235`_, `#1236`_, `#1240`_).
  Thanks `@DanielaE (Daniela Engert) <https://github.com/DanielaE>`_,
  `@mwinterb <https://github.com/mwinterb>`_,
  `@eliaskosunen (Elias Kosunen) <https://github.com/eliaskosunen>`_,
  `@morinmorin <https://github.com/morinmorin>`_,
  `@ricco19 (Brian Ricciardelli) <https://github.com/ricco19>`_,
  `@waywardmonkeys (Bruce Mitchener) <https://github.com/waywardmonkeys>`_,
  `@chronoxor (Ivan Shynkarenka) <https://github.com/chronoxor>`_,
  `@remyabel <https://github.com/remyabel>`_,
  `@pauldreik (Paul Dreik) <https://github.com/pauldreik>`_,
  `@gsjaardema (Greg Sjaardema) <https://github.com/gsjaardema>`_,
  `@rcane (Ronny Krüger) <https://github.com/rcane>`_,
  `@mocabe <https://github.com/mocabe>`_,
  `@denchat <https://github.com/denchat>`_,
  `@cjdb (Christopher Di Bella) <https://github.com/cjdb>`_,
  `@HazardyKnusperkeks (Björn Schäpers) <https://github.com/HazardyKnusperkeks>`_,
  `@vedranmiletic (Vedran Miletić) <https://github.com/vedranmiletic>`_,
  `@jackoalan (Jack Andersen) <https://github.com/jackoalan>`_,
  `@DaanDeMeyer (Daan De Meyer) <https://github.com/DaanDeMeyer>`_,
  `@starkmapper (Mark Stapper) <https://github.com/starkmapper>`_.

5.3.0 - 2018-12-28
------------------

* Introduced experimental chrono formatting support:

  .. code:: c++

     #include <fmt/chrono.h>

     int main() {
       using namespace std::literals::chrono_literals;
       fmt::print("Default format: {} {}\n", 42s, 100ms);
       fmt::print("strftime-like format: {:%H:%M:%S}\n", 3h + 15min + 30s);
     }

  prints::

     Default format: 42s 100ms
     strftime-like format: 03:15:30

* Added experimental support for emphasis (bold, italic, underline,
  strikethrough), colored output to a file stream, and improved colored
  formatting API (`#961`_, `#967`_, `#973`_):

  .. code:: c++

     #include <fmt/color.h>

     int main() {
       print(fg(fmt::color::crimson) | fmt::emphasis::bold,
             "Hello, {}!\n", "world");
       print(fg(fmt::color::floral_white) | bg(fmt::color::slate_gray) |
             fmt::emphasis::underline, "Hello, {}!\n", "мир");
       print(fg(fmt::color::steel_blue) | fmt::emphasis::italic,
             "Hello, {}!\n", "世界");
     }

  prints the following on modern terminals with RGB color support:

  .. image:: https://user-images.githubusercontent.com/576385/
             50405788-b66e7500-076e-11e9-9592-7324d1f951d8.png

  Thanks `@Rakete1111 (Nicolas) <https://github.com/Rakete1111>`_.

* Added support for 4-bit terminal colors (`#968`_, `#974`_)

  .. code:: c++

     #include <fmt/color.h>

     int main() {
       print(fg(fmt::terminal_color::red), "stop\n");
     }

  Note that these colors vary by terminal:

  .. image:: https://user-images.githubusercontent.com/576385/
             50405925-dbfc7e00-0770-11e9-9b85-333fab0af9ac.png

  Thanks `@Rakete1111 (Nicolas) <https://github.com/Rakete1111>`_.

* Parameterized formatting functions on the type of the format string
  (`#880`_, `#881`_, `#883`_, `#885`_, `#897`_, `#920`_).
  Any object of type ``S`` that has an overloaded ``to_string_view(const S&)``
  returning ``fmt::string_view`` can be used as a format string:

  .. code:: c++

     namespace my_ns {
     inline string_view to_string_view(const my_string& s) {
       return {s.data(), s.length()};
     }
     }

     std::string message = fmt::format(my_string("The answer is {}."), 42);

  Thanks `@DanielaE (Daniela Engert) <https://github.com/DanielaE>`_.

* Made ``std::string_view`` work as a format string (`#898`_):

  .. code:: c++

     auto message = fmt::format(std::string_view("The answer is {}."), 42);

  Thanks `@DanielaE (Daniela Engert) <https://github.com/DanielaE>`_.

* Added wide string support to compile-time format string checks (`#924`_):

  .. code:: c++

     print(fmt(L"{:f}"), 42); // compile-time error: invalid type specifier

  Thanks `@XZiar <https://github.com/XZiar>`_.

* Made colored print functions work with wide strings (`#867`_):

  .. code:: c++

     #include <fmt/color.h>

     int main() {
       print(fg(fmt::color::red), L"{}\n", 42);
     }

  Thanks `@DanielaE (Daniela Engert) <https://github.com/DanielaE>`_.

* Introduced experimental Unicode support (`#628`_, `#891`_):

  .. code:: c++

     using namespace fmt::literals;
     auto s = fmt::format("{:*^5}"_u, "🤡"_u); // s == "**🤡**"_u

* Improved locale support:

  .. code:: c++

     #include <fmt/locale.h>

     struct numpunct : std::numpunct<char> {
      protected:
       char do_thousands_sep() const override { return '~'; }
     };

     std::locale loc;
     auto s = fmt::format(std::locale(loc, new numpunct()), "{:n}", 1234567);
     // s == "1~234~567"

* Constrained formatting functions on proper iterator types (`#921`_).
  Thanks `@DanielaE (Daniela Engert) <https://github.com/DanielaE>`_.

* Added ``make_printf_args`` and ``make_wprintf_args`` functions (`#934`_).
  Thanks `@tnovotny <https://github.com/tnovotny>`_.

* Deprecated ``fmt::visit``, ``parse_context``, and ``wparse_context``.
  Use ``fmt::visit_format_arg``, ``format_parse_context``, and
  ``wformat_parse_context`` instead.

* Removed undocumented ``basic_fixed_buffer`` which has been superseded by the
  iterator-based API (`#873`_, `#902`_).
  Thanks `@superfunc (hollywood programmer) <https://github.com/superfunc>`_.

* Disallowed repeated leading zeros in an argument ID:

  .. code:: c++

     fmt::print("{000}", 42); // error

* Reintroduced support for gcc 4.4.

* Fixed compilation on platforms with exotic ``double`` (`#878`_).

* Improved documentation (`#164`_, `#877`_, `#901`_, `#906`_, `#979`_).
  Thanks `@kookjr (Mathew Cucuzella) <https://github.com/kookjr>`_,
  `@DarkDimius (Dmitry Petrashko) <https://github.com/DarkDimius>`_,
  `@HecticSerenity <https://github.com/HecticSerenity>`_.

* Added pkgconfig support which makes it easier to consume the library from
  meson and other build systems (`#916`_).
  Thanks `@colemickens (Cole Mickens) <https://github.com/colemickens>`_.

* Various build improvements (`#909`_, `#926`_, `#937`_, `#953`_, `#959`_).
  Thanks `@tchaikov (Kefu Chai) <https://github.com/tchaikov>`_,
  `@luncliff (Park DongHa) <https://github.com/luncliff>`_,
  `@AndreasSchoenle (Andreas Schönle) <https://github.com/AndreasSchoenle>`_,
  `@hotwatermorning <https://github.com/hotwatermorning>`_,
  `@Zefz (JohanJansen) <https://github.com/Zefz>`_.

* Improved ``string_view`` construction performance (`#914`_).
  Thanks `@gabime (Gabi Melman) <https://github.com/gabime>`_.

* Fixed non-matching char types (`#895`_).
  Thanks `@DanielaE (Daniela Engert) <https://github.com/DanielaE>`_.

* Fixed ``format_to_n`` with ``std::back_insert_iterator`` (`#913`_).
  Thanks `@DanielaE (Daniela Engert) <https://github.com/DanielaE>`_.

* Fixed locale-dependent formatting (`#905`_).

* Fixed various compiler warnings and errors (`#882`_, `#886`_, `#933`_,
  `#941`_, `#931`_, `#943`_, `#954`_, `#956`_, `#962`_, `#965`_, `#977`_,
  `#983`_, `#989`_).
  Thanks `@Luthaf (Guillaume Fraux) <https://github.com/Luthaf>`_,
  `@stevenhoving (Steven Hoving) <https://github.com/stevenhoving>`_,
  `@christinaa (Kristina Brooks) <https://github.com/christinaa>`_,
  `@lgritz (Larry Gritz) <https://github.com/lgritz>`_,
  `@DanielaE (Daniela Engert) <https://github.com/DanielaE>`_,
  `@0x8000-0000 (Sign Bit) <https://github.com/0x8000-0000>`_,
  `@liuping1997 <https://github.com/liuping1997>`_.

5.2.1 - 2018-09-21
------------------

* Fixed ``visit`` lookup issues on gcc 7 & 8 (`#870`_).
  Thanks `@medithe <https://github.com/medithe>`_.

* Fixed linkage errors on older gcc.

* Prevented ``fmt/range.h`` from specializing ``fmt::basic_string_view``
  (`#865`_, `#868`_).
  Thanks `@hhggit (dual) <https://github.com/hhggit>`_.

* Improved error message when formatting unknown types (`#872`_).
  Thanks `@foonathan (Jonathan Müller) <https://github.com/foonathan>`_,

* Disabled templated user-defined literals when compiled under nvcc (`#875`_).
  Thanks `@CandyGumdrop (Candy Gumdrop) <https://github.com/CandyGumdrop>`_,

* Fixed ``format_to`` formatting to ``wmemory_buffer`` (`#874`_).

5.2.0 - 2018-09-13
------------------

* Optimized format string parsing and argument processing which resulted in up
  to 5x speed up on long format strings and significant performance boost on
  various benchmarks. For example, version 5.2 is 2.22x faster than 5.1 on
  decimal integer formatting with ``format_to`` (macOS, clang-902.0.39.2):

  ==================  =======  =======
  Method              Time, s  Speedup
  ==================  =======  =======
  fmt::format 5.1      0.58
  fmt::format 5.2      0.35     1.66x
  fmt::format_to 5.1   0.51
  fmt::format_to 5.2   0.23     2.22x
  sprintf              0.71
  std::to_string       1.01
  std::stringstream    1.73
  ==================  =======  =======

* Changed the ``fmt`` macro from opt-out to opt-in to prevent name collisions.
  To enable it define the ``FMT_STRING_ALIAS`` macro to 1 before including
  ``fmt/format.h``:

  .. code:: c++

     #define FMT_STRING_ALIAS 1
     #include <fmt/format.h>
     std::string answer = format(fmt("{}"), 42);

* Added compile-time format string checks to ``format_to`` overload that takes
  ``fmt::memory_buffer`` (`#783`_):

  .. code:: c++

     fmt::memory_buffer buf;
     // Compile-time error: invalid type specifier.
     fmt::format_to(buf, fmt("{:d}"), "foo");

* Moved experimental color support to ``fmt/color.h`` and enabled the
  new API by default. The old API can be enabled by defining the
  ``FMT_DEPRECATED_COLORS`` macro.

* Added formatting support for types explicitly convertible to
  ``fmt::string_view``:

  .. code:: c++

     struct foo {
       explicit operator fmt::string_view() const { return "foo"; }
     };
     auto s = format("{}", foo());

  In particular, this makes formatting function work with
  ``folly::StringPiece``.

* Implemented preliminary support for ``char*_t`` by replacing the ``format``
  function overloads with a single function template parameterized on the string
  type.

* Added support for dynamic argument lists (`#814`_, `#819`_).
  Thanks `@MikePopoloski (Michael Popoloski)
  <https://github.com/MikePopoloski>`_.

* Reduced executable size overhead for embedded targets using newlib nano by
  making locale dependency optional (`#839`_).
  Thanks `@teajay-fr (Thomas Benard) <https://github.com/teajay-fr>`_.

* Keep ``noexcept`` specifier when exceptions are disabled (`#801`_, `#810`_).
  Thanks `@qis (Alexej Harm) <https://github.com/qis>`_.

* Fixed formatting of user-defined types providing ``operator<<`` with
  ``format_to_n`` (`#806`_).
  Thanks `@mkurdej (Marek Kurdej) <https://github.com/mkurdej>`_.

* Fixed dynamic linkage of new symbols (`#808`_).

* Fixed global initialization issue (`#807`_):

  .. code:: c++

     // This works on compilers with constexpr support.
     static const std::string answer = fmt::format("{}", 42);

* Fixed various compiler warnings and errors (`#804`_, `#809`_, `#811`_,
  `#822`_, `#827`_, `#830`_, `#838`_, `#843`_, `#844`_, `#851`_, `#852`_,
  `#854`_).
  Thanks `@henryiii (Henry Schreiner) <https://github.com/henryiii>`_,
  `@medithe <https://github.com/medithe>`_, and
  `@eliasdaler (Elias Daler) <https://github.com/eliasdaler>`_.

5.1.0 - 2018-07-05
------------------

* Added experimental support for RGB color output enabled with
  the ``FMT_EXTENDED_COLORS`` macro:

  .. code:: c++

     #define FMT_EXTENDED_COLORS
     #define FMT_HEADER_ONLY // or compile fmt with FMT_EXTENDED_COLORS defined
     #include <fmt/format.h>

     fmt::print(fmt::color::steel_blue, "Some beautiful text");

  The old API (the ``print_colored`` and ``vprint_colored`` functions and the
  ``color`` enum) is now deprecated. (`#762`_ `#767`_).
  thanks `@Remotion (Remo) <https://github.com/Remotion>`_.

* Added quotes to strings in ranges and tuples (`#766`_).
  Thanks `@Remotion (Remo) <https://github.com/Remotion>`_.

* Made ``format_to`` work with ``basic_memory_buffer`` (`#776`_).

* Added ``vformat_to_n`` and ``wchar_t`` overload of ``format_to_n``
  (`#764`_, `#769`_).

* Made ``is_range`` and ``is_tuple_like`` part of public (experimental) API
  to allow specialization for user-defined types (`#751`_, `#759`_).
  Thanks `@drrlvn (Dror Levin) <https://github.com/drrlvn>`_.

* Added more compilers to continuous integration and increased ``FMT_PEDANTIC``
  warning levels (`#736`_).
  Thanks `@eliaskosunen (Elias Kosunen) <https://github.com/eliaskosunen>`_.

* Fixed compilation with MSVC 2013.

* Fixed handling of user-defined types in ``format_to`` (`#793`_).

* Forced linking of inline ``vformat`` functions into the library (`#795`_).

* Fixed incorrect call to on_align in ``'{:}='`` (`#750`_).

* Fixed floating-point formatting to a non-back_insert_iterator with sign &
  numeric alignment specified (`#756`_).

* Fixed formatting to an array with ``format_to_n`` (`#778`_).

* Fixed formatting of more than 15 named arguments (`#754`_).

* Fixed handling of compile-time strings when including ``fmt/ostream.h``.
  (`#768`_).

* Fixed various compiler warnings and errors (`#742`_, `#748`_, `#752`_,
  `#770`_, `#775`_, `#779`_, `#780`_, `#790`_, `#792`_, `#800`_).
  Thanks `@Remotion (Remo) <https://github.com/Remotion>`_,
  `@gabime (Gabi Melman) <https://github.com/gabime>`_,
  `@foonathan (Jonathan Müller) <https://github.com/foonathan>`_,
  `@Dark-Passenger (Dhruv Paranjape) <https://github.com/Dark-Passenger>`_, and
  `@0x8000-0000 (Sign Bit) <https://github.com/0x8000-0000>`_.

5.0.0 - 2018-05-21
------------------

* Added a requirement for partial C++11 support, most importantly variadic
  templates and type traits, and dropped ``FMT_VARIADIC_*`` emulation macros.
  Variadic templates are available since GCC 4.4, Clang 2.9 and MSVC 18.0 (2013).
  For older compilers use {fmt} `version 4.x
  <https://github.com/fmtlib/fmt/releases/tag/4.1.0>`_ which continues to be
  maintained and works with C++98 compilers.

* Renamed symbols to follow standard C++ naming conventions and proposed a subset
  of the library for standardization in `P0645R2 Text Formatting
  <https://wg21.link/P0645>`_.

* Implemented ``constexpr`` parsing of format strings and `compile-time format
  string checks
  <https://fmt.dev/dev/api.html#compile-time-format-string-checks>`_. For
  example

  .. code:: c++

     #include <fmt/format.h>

     std::string s = format(fmt("{:d}"), "foo");

  gives a compile-time error because ``d`` is an invalid specifier for strings
  (`godbolt <https://godbolt.org/g/rnCy9Q>`__)::

     ...
     <source>:4:19: note: in instantiation of function template specialization 'fmt::v5::format<S, char [4]>' requested here
       std::string s = format(fmt("{:d}"), "foo");
                       ^
     format.h:1337:13: note: non-constexpr function 'on_error' cannot be used in a constant expression
         handler.on_error("invalid type specifier");

  Compile-time checks require relaxed ``constexpr`` (C++14 feature) support. If
  the latter is not available, checks will be performed at runtime.

* Separated format string parsing and formatting in the extension API to enable
  compile-time format string processing. For example

  .. code:: c++

     struct Answer {};

     namespace fmt {
     template <>
     struct formatter<Answer> {
       constexpr auto parse(parse_context& ctx) {
         auto it = ctx.begin();
         spec = *it;
         if (spec != 'd' && spec != 's')
           throw format_error("invalid specifier");
         return ++it;
       }

       template <typename FormatContext>
       auto format(Answer, FormatContext& ctx) {
         return spec == 's' ?
           format_to(ctx.begin(), "{}", "fourty-two") :
           format_to(ctx.begin(), "{}", 42);
       }

       char spec = 0;
     };
     }

     std::string s = format(fmt("{:x}"), Answer());

  gives a compile-time error due to invalid format specifier (`godbolt
  <https://godbolt.org/g/2jQ1Dv>`__)::

     ...
     <source>:12:45: error: expression '<throw-expression>' is not a constant expression
            throw format_error("invalid specifier");

* Added `iterator support
  <https://fmt.dev/dev/api.html#output-iterator-support>`_:

  .. code:: c++

     #include <vector>
     #include <fmt/format.h>

     std::vector<char> out;
     fmt::format_to(std::back_inserter(out), "{}", 42);

* Added the `format_to_n
  <https://fmt.dev/dev/api.html#_CPPv2N3fmt11format_to_nE8OutputItNSt6size_tE11string_viewDpRK4Args>`_
  function that restricts the output to the specified number of characters
  (`#298`_):

  .. code:: c++

     char out[4];
     fmt::format_to_n(out, sizeof(out), "{}", 12345);
     // out == "1234" (without terminating '\0')

* Added the `formatted_size
  <https://fmt.dev/dev/api.html#_CPPv2N3fmt14formatted_sizeE11string_viewDpRK4Args>`_
  function for computing the output size:

  .. code:: c++

     #include <fmt/format.h>

     auto size = fmt::formatted_size("{}", 12345); // size == 5

* Improved compile times by reducing dependencies on standard headers and
  providing a lightweight `core API <https://fmt.dev/dev/api.html#core-api>`_:

  .. code:: c++

     #include <fmt/core.h>

     fmt::print("The answer is {}.", 42);

  See `Compile time and code bloat
  <https://github.com/fmtlib/fmt#compile-time-and-code-bloat>`_.

* Added the `make_format_args
  <https://fmt.dev/dev/api.html#_CPPv2N3fmt16make_format_argsEDpRK4Args>`_
  function for capturing formatting arguments:

  .. code:: c++
  
     // Prints formatted error message.
     void vreport_error(const char *format, fmt::format_args args) {
       fmt::print("Error: ");
       fmt::vprint(format, args);
     }
     template <typename... Args>
     void report_error(const char *format, const Args & ... args) {
       vreport_error(format, fmt::make_format_args(args...));
     }

* Added the ``make_printf_args`` function for capturing ``printf`` arguments
  (`#687`_, `#694`_).
  Thanks `@Kronuz (Germán Méndez Bravo) <https://github.com/Kronuz>`_.

* Added prefix ``v`` to non-variadic functions taking ``format_args`` to
  distinguish them from variadic ones:

  .. code:: c++

     std::string vformat(string_view format_str, format_args args);
     
     template <typename... Args>
     std::string format(string_view format_str, const Args & ... args);

* Added experimental support for formatting ranges, containers and tuple-like
  types in ``fmt/ranges.h`` (`#735`_):

  .. code:: c++

     #include <fmt/ranges.h>

     std::vector<int> v = {1, 2, 3};
     fmt::print("{}", v); // prints {1, 2, 3}

  Thanks `@Remotion (Remo) <https://github.com/Remotion>`_.

* Implemented ``wchar_t`` date and time formatting (`#712`_):

  .. code:: c++

     #include <fmt/time.h>

     std::time_t t = std::time(nullptr);
     auto s = fmt::format(L"The date is {:%Y-%m-%d}.", *std::localtime(&t));

  Thanks `@DanielaE (Daniela Engert) <https://github.com/DanielaE>`_.

* Provided more wide string overloads (`#724`_).
  Thanks `@DanielaE (Daniela Engert) <https://github.com/DanielaE>`_.

* Switched from a custom null-terminated string view class to ``string_view``
  in the format API and provided ``fmt::string_view`` which implements a subset
  of ``std::string_view`` API for pre-C++17 systems.

* Added support for ``std::experimental::string_view`` (`#607`_):

  .. code:: c++

     #include <fmt/core.h>
     #include <experimental/string_view>

     fmt::print("{}", std::experimental::string_view("foo"));

  Thanks `@virgiliofornazin (Virgilio Alexandre Fornazin)
  <https://github.com/virgiliofornazin>`__.

* Allowed mixing named and automatic arguments:

  .. code:: c++

     fmt::format("{} {two}", 1, fmt::arg("two", 2));

* Removed the write API in favor of the `format API
  <https://fmt.dev/dev/api.html#format-api>`_ with compile-time handling of
  format strings.

* Disallowed formatting of multibyte strings into a wide character target
  (`#606`_).

* Improved documentation (`#515`_, `#614`_, `#617`_, `#661`_, `#680`_).
  Thanks `@ibell (Ian Bell) <https://github.com/ibell>`_,
  `@mihaitodor (Mihai Todor) <https://github.com/mihaitodor>`_, and
  `@johnthagen <https://github.com/johnthagen>`_.

* Implemented more efficient handling of large number of format arguments.

* Introduced an inline namespace for symbol versioning.

* Added debug postfix ``d`` to the ``fmt`` library name (`#636`_).

* Removed unnecessary ``fmt/`` prefix in includes (`#397`_).
  Thanks `@chronoxor (Ivan Shynkarenka) <https://github.com/chronoxor>`_.

* Moved ``fmt/*.h`` to ``include/fmt/*.h`` to prevent irrelevant files and
  directories appearing on the include search paths when fmt is used as a
  subproject and moved source files to the ``src`` directory.

* Added qmake project file ``support/fmt.pro`` (`#641`_).
  Thanks `@cowo78 (Giuseppe Corbelli) <https://github.com/cowo78>`_.

* Added Gradle build file ``support/build.gradle`` (`#649`_).
  Thanks `@luncliff (Park DongHa) <https://github.com/luncliff>`_.

* Removed ``FMT_CPPFORMAT`` CMake option.

* Fixed a name conflict with the macro ``CHAR_WIDTH`` in glibc (`#616`_).
  Thanks `@aroig (Abdó Roig-Maranges) <https://github.com/aroig>`_.

* Fixed handling of nested braces in ``fmt::join`` (`#638`_).

* Added ``SOURCELINK_SUFFIX`` for compatibility with Sphinx 1.5 (`#497`_).
  Thanks `@ginggs (Graham Inggs) <https://github.com/ginggs>`_.

* Added a missing ``inline`` in the header-only mode (`#626`_).
  Thanks `@aroig (Abdó Roig-Maranges) <https://github.com/aroig>`_.

* Fixed various compiler warnings (`#640`_, `#656`_, `#679`_, `#681`_, `#705`__,
  `#715`_, `#717`_, `#720`_, `#723`_, `#726`_, `#730`_, `#739`_).
  Thanks `@peterbell10 <https://github.com/peterbell10>`_,
  `@LarsGullik <https://github.com/LarsGullik>`_,
  `@foonathan (Jonathan Müller) <https://github.com/foonathan>`_,
  `@eliaskosunen (Elias Kosunen) <https://github.com/eliaskosunen>`_,
  `@christianparpart (Christian Parpart)
  <https://github.com/christianparpart>`_,
  `@DanielaE (Daniela Engert) <https://github.com/DanielaE>`_,
  and `@mwinterb <https://github.com/mwinterb>`_.

* Worked around an MSVC bug and fixed several warnings (`#653`_).
  Thanks `@alabuzhev (Alex Alabuzhev) <https://github.com/alabuzhev>`_.

* Worked around GCC bug 67371 (`#682`_).

* Fixed compilation with ``-fno-exceptions`` (`#655`_).
  Thanks `@chenxiaolong (Andrew Gunnerson) <https://github.com/chenxiaolong>`_.

* Made ``constexpr remove_prefix`` gcc version check tighter (`#648`_).

* Renamed internal type enum constants to prevent collision with poorly written
  C libraries (`#644`_).

* Added detection of ``wostream operator<<`` (`#650`_).

* Fixed compilation on OpenBSD (`#660`_).
  Thanks `@hubslave <https://github.com/hubslave>`_.

* Fixed compilation on FreeBSD 12 (`#732`_).
  Thanks `@dankm <https://github.com/dankm>`_.

* Fixed compilation when there is a mismatch between ``-std`` options between
  the library and user code (`#664`_).

* Fixed compilation with GCC 7 and ``-std=c++11`` (`#734`_).

* Improved generated binary code on GCC 7 and older (`#668`_).

* Fixed handling of numeric alignment with no width (`#675`_).

* Fixed handling of empty strings in UTF8/16 converters (`#676`_).
  Thanks `@vgalka-sl (Vasili Galka) <https://github.com/vgalka-sl>`_.

* Fixed formatting of an empty ``string_view`` (`#689`_).

* Fixed detection of ``string_view`` on libc++ (`#686`_).

* Fixed DLL issues (`#696`_).
  Thanks `@sebkoenig <https://github.com/sebkoenig>`_.

* Fixed compile checks for mixing narrow and wide strings (`#690`_).

* Disabled unsafe implicit conversion to ``std::string`` (`#729`_).

* Fixed handling of reused format specs (as in ``fmt::join``) for pointers
  (`#725`_). Thanks `@mwinterb <https://github.com/mwinterb>`_.

* Fixed installation of ``fmt/ranges.h`` (`#738`_).
  Thanks `@sv1990 <https://github.com/sv1990>`_.

4.1.0 - 2017-12-20
------------------

* Added ``fmt::to_wstring()`` in addition to ``fmt::to_string()`` (`#559`_).
  Thanks `@alabuzhev (Alex Alabuzhev) <https://github.com/alabuzhev>`_.

* Added support for C++17 ``std::string_view`` (`#571`_ and `#578`_).
  Thanks `@thelostt (Mário Feroldi) <https://github.com/thelostt>`_ and
  `@mwinterb <https://github.com/mwinterb>`_.

* Enabled stream exceptions to catch errors (`#581`_).
  Thanks `@crusader-mike <https://github.com/crusader-mike>`_.

* Allowed formatting of class hierarchies with ``fmt::format_arg()`` (`#547`_).
  Thanks `@rollbear (Björn Fahller) <https://github.com/rollbear>`_.

* Removed limitations on character types (`#563`_).
  Thanks `@Yelnats321 (Elnar Dakeshov) <https://github.com/Yelnats321>`_.

* Conditionally enabled use of ``std::allocator_traits`` (`#583`_).
  Thanks `@mwinterb <https://github.com/mwinterb>`_.

* Added support for ``const`` variadic member function emulation with
  ``FMT_VARIADIC_CONST`` (`#591`_).
  Thanks `@ludekvodicka (Ludek Vodicka) <https://github.com/ludekvodicka>`_.

* Various bugfixes: bad overflow check, unsupported implicit type conversion
  when determining formatting function, test segfaults (`#551`_), ill-formed
  macros (`#542`_) and ambiguous overloads (`#580`_).
  Thanks `@xylosper (Byoung-young Lee) <https://github.com/xylosper>`_.

* Prevented warnings on MSVC (`#605`_, `#602`_, and `#545`_), clang (`#582`_),
  GCC (`#573`_), various conversion warnings (`#609`_, `#567`_, `#553`_ and
  `#553`_), and added ``override`` and ``[[noreturn]]`` (`#549`_ and `#555`_).
  Thanks `@alabuzhev (Alex Alabuzhev) <https://github.com/alabuzhev>`_,
  `@virgiliofornazin (Virgilio Alexandre Fornazin)
  <https://gihtub.com/virgiliofornazin>`_,
  `@alexanderbock (Alexander Bock) <https://github.com/alexanderbock>`_,
  `@yumetodo <https://github.com/yumetodo>`_,
  `@VaderY (Császár Mátyás) <https://github.com/VaderY>`_,
  `@jpcima (JP Cimalando) <https://github.com/jpcima>`_,
  `@thelostt (Mário Feroldi) <https://github.com/thelostt>`_, and
  `@Manu343726 (Manu Sánchez) <https://github.com/Manu343726>`_.

* Improved CMake: Used ``GNUInstallDirs`` to set installation location
  (`#610`_) and fixed warnings (`#536`_ and `#556`_).
  Thanks `@mikecrowe (Mike Crowe) <https://github.com/mikecrowe>`_,
  `@evgen231 <https://github.com/evgen231>`_ and
  `@henryiii (Henry Schreiner) <https://github.com/henryiii>`_.

4.0.0 - 2017-06-27
------------------

* Removed old compatibility headers ``cppformat/*.h`` and CMake options
  (`#527`_).
  Thanks `@maddinat0r (Alex Martin) <https://github.com/maddinat0r>`_.

* Added ``string.h`` containing ``fmt::to_string()`` as alternative to
  ``std::to_string()`` as well as other string writer functionality
  (`#326`_ and `#441`_):

  .. code:: c++

    #include "fmt/string.h"
  
    std::string answer = fmt::to_string(42);

  Thanks to `@glebov-andrey (Andrey Glebov)
  <https://github.com/glebov-andrey>`_.

* Moved ``fmt::printf()`` to new ``printf.h`` header and allowed ``%s`` as
  generic specifier (`#453`_), made ``%.f`` more conformant to regular
  ``printf()`` (`#490`_), added custom writer support (`#476`_) and implemented
  missing custom argument formatting (`#339`_ and `#340`_):

  .. code:: c++

    #include "fmt/printf.h"
 
    // %s format specifier can be used with any argument type.
    fmt::printf("%s", 42);

  Thanks `@mojoBrendan <https://github.com/mojoBrendan>`_,
  `@manylegged (Arthur Danskin) <https://github.com/manylegged>`_ and
  `@spacemoose (Glen Stark) <https://github.com/spacemoose>`_.
  See also `#360`_, `#335`_ and `#331`_.

* Added ``container.h`` containing a ``BasicContainerWriter``
  to write to containers like ``std::vector`` (`#450`_).
  Thanks `@polyvertex (Jean-Charles Lefebvre) <https://github.com/polyvertex>`_.

* Added ``fmt::join()`` function that takes a range and formats
  its elements separated by a given string (`#466`_):

  .. code:: c++

    #include "fmt/format.h"
 
    std::vector<double> v = {1.2, 3.4, 5.6};
    // Prints "(+01.20, +03.40, +05.60)".
    fmt::print("({:+06.2f})", fmt::join(v.begin(), v.end(), ", "));

  Thanks `@olivier80 <https://github.com/olivier80>`_.

* Added support for custom formatting specifications to simplify customization
  of built-in formatting (`#444`_).
  Thanks `@polyvertex (Jean-Charles Lefebvre) <https://github.com/polyvertex>`_.
  See also `#439`_.

* Added ``fmt::format_system_error()`` for error code formatting
  (`#323`_ and `#526`_).
  Thanks `@maddinat0r (Alex Martin) <https://github.com/maddinat0r>`_.

* Added thread-safe ``fmt::localtime()`` and ``fmt::gmtime()``
  as replacement   for the standard version to ``time.h`` (`#396`_).
  Thanks `@codicodi <https://github.com/codicodi>`_.

* Internal improvements to ``NamedArg`` and ``ArgLists`` (`#389`_ and `#390`_).
  Thanks `@chronoxor <https://github.com/chronoxor>`_.

* Fixed crash due to bug in ``FormatBuf`` (`#493`_).
  Thanks `@effzeh <https://github.com/effzeh>`_. See also `#480`_ and `#491`_.

* Fixed handling of wide strings in ``fmt::StringWriter``.

* Improved compiler error messages (`#357`_).

* Fixed various warnings and issues with various compilers
  (`#494`_, `#499`_, `#483`_, `#485`_, `#482`_, `#475`_, `#473`_ and `#414`_).
  Thanks `@chronoxor <https://github.com/chronoxor>`_,
  `@zhaohuaxishi <https://github.com/zhaohuaxishi>`_,
  `@pkestene (Pierre Kestener) <https://github.com/pkestene>`_,
  `@dschmidt (Dominik Schmidt) <https://github.com/dschmidt>`_ and
  `@0x414c (Alexey Gorishny) <https://github.com/0x414c>`_ .

* Improved CMake: targets are now namespaced (`#511`_ and `#513`_), added
  support for header-only ``printf.h`` (`#354`_), fixed issue with minimal
  supported library subset (`#418`_, `#419`_ and `#420`_).
  Thanks `@bjoernthiel (Bjoern Thiel) <https://github.com/bjoernthiel>`_,
  `@niosHD (Mario Werner) <https://github.com/niosHD>`_,
  `@LogicalKnight (Sean LK) <https://github.com/LogicalKnight>`_ and
  `@alabuzhev (Alex Alabuzhev) <https://github.com/alabuzhev>`_.

* Improved documentation. Thanks to
  `@pwm1234 (Phil) <https://github.com/pwm1234>`_ for `#393`_.

3.0.2 - 2017-06-14
------------------

* Added ``FMT_VERSION`` macro (`#411`_).

* Used ``FMT_NULL`` instead of literal ``0`` (`#409`_).
  Thanks `@alabuzhev (Alex Alabuzhev) <https://github.com/alabuzhev>`_.

* Added extern templates for ``format_float`` (`#413`_).

* Fixed implicit conversion issue (`#507`_).

* Fixed signbit detection (`#423`_).

* Fixed naming collision (`#425`_).

* Fixed missing intrinsic for C++/CLI (`#457`_).
  Thanks `@calumr (Calum Robinson) <https://github.com/calumr>`_

* Fixed Android detection (`#458`_).
  Thanks `@Gachapen (Magnus Bjerke Vik) <https://github.com/Gachapen>`_.

* Use lean ``windows.h`` if not in header-only mode (`#503`_).
  Thanks `@Quentin01 (Quentin Buathier) <https://github.com/Quentin01>`_.

* Fixed issue with CMake exporting C++11 flag (`#445`_).
  Thanks `@EricWF (Eric) <https://github.com/EricWF>`_.

* Fixed issue with nvcc and MSVC compiler bug and MinGW (`#505`_).

* Fixed DLL issues (`#469`_ and `#502`_).
  Thanks `@richardeakin (Richard Eakin) <https://github.com/richardeakin>`_ and
  `@AndreasSchoenle (Andreas Schönle) <https://github.com/AndreasSchoenle>`_.

* Fixed test compilation under FreeBSD (`#433`_).

* Fixed various warnings (`#403`_, `#410`_ and `#510`_).
  Thanks `@Lecetem <https://github.com/Lectem>`_,
  `@chenhayat (Chen Hayat) <https://github.com/chenhayat>`_ and
  `@trozen <https://github.com/trozen>`_.

* Worked around a broken ``__builtin_clz`` in clang with MS codegen (`#519`_).

* Removed redundant include (`#479`_).

* Fixed documentation issues.

3.0.1 - 2016-11-01
------------------
* Fixed handling of thousands separator (`#353`_).

* Fixed handling of ``unsigned char`` strings (`#373`_).

* Corrected buffer growth when formatting time (`#367`_).

* Removed warnings under MSVC and clang (`#318`_, `#250`_, also merged
  `#385`_ and `#361`_).
  Thanks `@jcelerier (Jean-Michaël Celerier) <https://github.com/jcelerier>`_
  and `@nmoehrle (Nils Moehrle) <https://github.com/nmoehrle>`_.

* Fixed compilation issues under Android (`#327`_, `#345`_ and `#381`_),
  FreeBSD (`#358`_), Cygwin (`#388`_), MinGW (`#355`_) as well as other
  issues (`#350`_, `#366`_, `#348`_, `#402`_, `#405`_).
  Thanks to `@dpantele (Dmitry) <https://github.com/dpantele>`_,
  `@hghwng (Hugh Wang) <https://github.com/hghwng>`_,
  `@arvedarved (Tilman Keskinöz) <https://github.com/arvedarved>`_,
  `@LogicalKnight (Sean) <https://github.com/LogicalKnight>`_ and
  `@JanHellwig (Jan Hellwig) <https://github.com/janhellwig>`_.

* Fixed some documentation issues and extended specification
  (`#320`_, `#333`_, `#347`_, `#362`_).
  Thanks to `@smellman (Taro Matsuzawa aka. btm)
  <https://github.com/smellman>`_.

3.0.0 - 2016-05-07
------------------

* The project has been renamed from C++ Format (cppformat) to fmt for
  consistency with the used namespace and macro prefix (`#307`_).
  Library headers are now located in the ``fmt`` directory:

  .. code:: c++

    #include "fmt/format.h"

  Including ``format.h`` from the ``cppformat`` directory is deprecated
  but works via a proxy header which will be removed in the next major version.
  
  The documentation is now available at https://fmt.dev.

* Added support for `strftime
  <http://en.cppreference.com/w/cpp/chrono/c/strftime>`_-like
  `date and time formatting
  <https://fmt.dev/3.0.0/api.html#date-and-time-formatting>`_
  (`#283`_):

  .. code:: c++

    #include "fmt/time.h"

    std::time_t t = std::time(nullptr);
    // Prints "The date is 2016-04-29." (with the current date)
    fmt::print("The date is {:%Y-%m-%d}.", *std::localtime(&t));

* ``std::ostream`` support including formatting of user-defined types that
  provide overloaded ``operator<<`` has been moved to ``fmt/ostream.h``:

  .. code:: c++

    #include "fmt/ostream.h"

    class Date {
      int year_, month_, day_;
    public:
      Date(int year, int month, int day)
        : year_(year), month_(month), day_(day) {}

      friend std::ostream &operator<<(std::ostream &os, const Date &d) {
        return os << d.year_ << '-' << d.month_ << '-' << d.day_;
      }
    };

    std::string s = fmt::format("The date is {}", Date(2012, 12, 9));
    // s == "The date is 2012-12-9"

* Added support for `custom argument formatters
  <https://fmt.dev/3.0.0/api.html#argument-formatters>`_ (`#235`_).

* Added support for locale-specific integer formatting with the ``n`` specifier
  (`#305`_):

  .. code:: c++

    std::setlocale(LC_ALL, "en_US.utf8");
    fmt::print("cppformat: {:n}\n", 1234567); // prints 1,234,567

* Sign is now preserved when formatting an integer with an incorrect ``printf``
  format specifier (`#265`_):

  .. code:: c++

    fmt::printf("%lld", -42); // prints -42

  Note that it would be an undefined behavior in ``std::printf``.

* Length modifiers such as ``ll`` are now optional in printf formatting
  functions and the correct type is determined automatically (`#255`_):

  .. code:: c++

    fmt::printf("%d", std::numeric_limits<long long>::max());

  Note that it would be an undefined behavior in ``std::printf``.

* Added initial support for custom formatters (`#231`_).

* Fixed detection of user-defined literal support on Intel C++ compiler
  (`#311`_, `#312`_).
  Thanks to `@dean0x7d (Dean Moldovan) <https://github.com/dean0x7d>`_ and
  `@speth (Ray Speth) <https://github.com/speth>`_.

* Reduced compile time (`#243`_, `#249`_, `#317`_):

  .. image:: https://cloud.githubusercontent.com/assets/4831417/11614060/
             b9e826d2-9c36-11e5-8666-d4131bf503ef.png

  .. image:: https://cloud.githubusercontent.com/assets/4831417/11614080/
             6ac903cc-9c37-11e5-8165-26df6efae364.png

  Thanks to `@dean0x7d (Dean Moldovan) <https://github.com/dean0x7d>`_.

* Compile test fixes (`#313`_).
  Thanks to `@dean0x7d (Dean Moldovan) <https://github.com/dean0x7d>`_.

* Documentation fixes
  (`#239`_, `#248`_, `#252`_, `#258`_, `#260`_, `#301`_, `#309`_).
  Thanks to `@ReadmeCritic <https://github.com/ReadmeCritic>`_
  `@Gachapen (Magnus Bjerke Vik) <https://github.com/Gachapen>`_ and
  `@jwilk (Jakub Wilk) <https://github.com/jwilk>`_.

* Fixed compiler and sanitizer warnings (`#244`_, `#256`_, `#259`_, `#263`_,
  `#274`_, `#277`_, `#286`_, `#291`_, `#296`_, `#308`_)
  Thanks to `@mwinterb <https://github.com/mwinterb>`_,
  `@pweiskircher (Patrik Weiskircher) <https://github.com/pweiskircher>`_,
  `@Naios <https://github.com/Naios>`_.

* Improved compatibility with Windows Store apps (`#280`_, `#285`_).
  Thanks to `@mwinterb <https://github.com/mwinterb>`_.

* Added tests of compatibility with older C++ standards (`#273`_).
  Thanks to `@niosHD <https://github.com/niosHD>`_.

* Fixed Android build (`#271`_).
  Thanks to `@newnon <https://github.com/newnon>`_.

* Changed ``ArgMap`` to be backed by a vector instead of a map.
  (`#261`_, `#262`_).
  Thanks to `@mwinterb <https://github.com/mwinterb>`_.

* Added ``fprintf`` overload that writes to a ``std::ostream`` (`#251`_).
  Thanks to `nickhutchinson (Nicholas Hutchinson)
  <https://github.com/nickhutchinson>`_.

* Export symbols when building a Windows DLL (`#245`_).
  Thanks to `macdems (Maciek Dems) <https://github.com/macdems>`_.

* Fixed compilation on Cygwin (`#304`_).

* Implemented a workaround for a bug in Apple LLVM version 4.2 of clang
  (`#276`_).

* Implemented a workaround for Google Test bug
  `#705 <https://github.com/google/googletest/issues/705>`_ on gcc 6 (`#268`_).
  Thanks to `octoploid <https://github.com/octoploid>`_.

* Removed Biicode support because the latter has been discontinued.

2.1.1 - 2016-04-11
------------------

* The install location for generated CMake files is now configurable via
  the ``FMT_CMAKE_DIR`` CMake variable (`#299`_).
  Thanks to `@niosHD <https://github.com/niosHD>`_.

* Documentation fixes (`#252`_).

2.1.0 - 2016-03-21
------------------

* Project layout and build system improvements (`#267`_):

  * The code have been moved to the ``cppformat`` directory.
    Including ``format.h`` from the top-level directory is deprecated
    but works via a proxy header which will be removed in the next
    major version.

  * C++ Format CMake targets now have proper interface definitions.

  * Installed version of the library now supports the header-only
    configuration.

  * Targets ``doc``, ``install``, and ``test`` are now disabled if C++ Format
    is included as a CMake subproject. They can be enabled by setting
    ``FMT_DOC``, ``FMT_INSTALL``, and ``FMT_TEST`` in the parent project.

  Thanks to `@niosHD <https://github.com/niosHD>`_.

2.0.1 - 2016-03-13
------------------

* Improved CMake find and package support (`#264`_).
  Thanks to `@niosHD <https://github.com/niosHD>`_.

* Fix compile error with Android NDK and mingw32 (`#241`_).
  Thanks to `@Gachapen (Magnus Bjerke Vik) <https://github.com/Gachapen>`_.

* Documentation fixes (`#248`_, `#260`_).

2.0.0 - 2015-12-01
------------------

General
~~~~~~~

* [Breaking] Named arguments (`#169`_, `#173`_, `#174`_):

  .. code:: c++

    fmt::print("The answer is {answer}.", fmt::arg("answer", 42));

  Thanks to `@jamboree <https://github.com/jamboree>`_.

* [Experimental] User-defined literals for format and named arguments
  (`#204`_, `#206`_, `#207`_):

  .. code:: c++

    using namespace fmt::literals;
    fmt::print("The answer is {answer}.", "answer"_a=42);

  Thanks to `@dean0x7d (Dean Moldovan) <https://github.com/dean0x7d>`_.

* [Breaking] Formatting of more than 16 arguments is now supported when using
  variadic templates (`#141`_).
  Thanks to `@Shauren <https://github.com/Shauren>`_.

* Runtime width specification (`#168`_):

  .. code:: c++

    fmt::format("{0:{1}}", 42, 5); // gives "   42"

  Thanks to `@jamboree <https://github.com/jamboree>`_.

* [Breaking] Enums are now formatted with an overloaded ``std::ostream``
  insertion operator (``operator<<``) if available (`#232`_).

* [Breaking] Changed default ``bool`` format to textual, "true" or "false"
  (`#170`_):

  .. code:: c++
  
    fmt::print("{}", true); // prints "true"

  To print ``bool`` as a number use numeric format specifier such as ``d``:

  .. code:: c++

    fmt::print("{:d}", true); // prints "1"

* ``fmt::printf`` and ``fmt::sprintf`` now support formatting of ``bool`` with
  the ``%s`` specifier giving textual output, "true" or "false" (`#223`_):

  .. code:: c++

    fmt::printf("%s", true); // prints "true"

  Thanks to `@LarsGullik <https://github.com/LarsGullik>`_.

* [Breaking] ``signed char`` and ``unsigned char`` are now formatted as integers
  by default (`#217`_).

* [Breaking] Pointers to C strings can now be formatted with the ``p`` specifier
  (`#223`_):

  .. code:: c++

    fmt::print("{:p}", "test"); // prints pointer value

  Thanks to `@LarsGullik <https://github.com/LarsGullik>`_.

* [Breaking] ``fmt::printf`` and ``fmt::sprintf`` now print null pointers as
  ``(nil)`` and null strings as ``(null)`` for consistency with glibc (`#226`_).
  Thanks to `@LarsGullik <https://github.com/LarsGullik>`_.

* [Breaking] ``fmt::(s)printf`` now supports formatting of objects of
  user-defined types that provide an overloaded ``std::ostream`` insertion
  operator (``operator<<``) (`#201`_):

  .. code:: c++

    fmt::printf("The date is %s", Date(2012, 12, 9));

* [Breaking] The ``Buffer`` template is now part of the public API and can be
  used to implement custom memory buffers (`#140`_). Thanks to
  `@polyvertex (Jean-Charles Lefebvre) <https://github.com/polyvertex>`_.

* [Breaking] Improved compatibility between ``BasicStringRef`` and
  `std::experimental::basic_string_view
  <http://en.cppreference.com/w/cpp/experimental/basic_string_view>`_
  (`#100`_, `#159`_, `#183`_):

  - Comparison operators now compare string content, not pointers
  - ``BasicStringRef::c_str`` replaced by ``BasicStringRef::data``
  - ``BasicStringRef`` is no longer assumed to be null-terminated

  References to null-terminated strings are now represented by a new class,
  ``BasicCStringRef``.

* Dependency on pthreads introduced by Google Test is now optional (`#185`_).

* New CMake options ``FMT_DOC``, ``FMT_INSTALL`` and ``FMT_TEST`` to control
  generation of ``doc``, ``install`` and ``test`` targets respectively, on by
  default (`#197`_, `#198`_, `#200`_).
  Thanks to `@maddinat0r (Alex Martin) <https://github.com/maddinat0r>`_.

* ``noexcept`` is now used when compiling with MSVC2015 (`#215`_).
  Thanks to `@dmkrepo (Dmitriy) <https://github.com/dmkrepo>`_.

* Added an option to disable use of ``windows.h`` when ``FMT_USE_WINDOWS_H``
  is defined as 0 before including ``format.h`` (`#171`_).
  Thanks to `@alfps (Alf P. Steinbach) <https://github.com/alfps>`_.

* [Breaking] ``windows.h`` is now included with ``NOMINMAX`` unless
  ``FMT_WIN_MINMAX`` is defined. This is done to prevent breaking code using
  ``std::min`` and ``std::max`` and only affects the header-only configuration
  (`#152`_, `#153`_, `#154`_).
  Thanks to `@DevO2012 <https://github.com/DevO2012>`_.

* Improved support for custom character types (`#171`_).
  Thanks to `@alfps (Alf P. Steinbach) <https://github.com/alfps>`_.

* Added an option to disable use of IOStreams when ``FMT_USE_IOSTREAMS``
  is defined as 0 before including ``format.h`` (`#205`_, `#208`_).
  Thanks to `@JodiTheTigger <https://github.com/JodiTheTigger>`_.

* Improved detection of ``isnan``, ``isinf`` and ``signbit``.

Optimization
~~~~~~~~~~~~

* Made formatting of user-defined types more efficient with a custom stream
  buffer (`#92`_, `#230`_).
  Thanks to `@NotImplemented <https://github.com/NotImplemented>`_.

* Further improved performance of ``fmt::Writer`` on integer formatting
  and fixed a minor regression. Now it is ~7% faster than ``karma::generate``
  on Karma's benchmark (`#186`_).

* [Breaking] Reduced `compiled code size
  <https://github.com/fmtlib/fmt#compile-time-and-code-bloat>`_
  (`#143`_, `#149`_).

Distribution
~~~~~~~~~~~~

* [Breaking] Headers are now installed in
  ``${CMAKE_INSTALL_PREFIX}/include/cppformat`` (`#178`_).
  Thanks to `@jackyf (Eugene V. Lyubimkin) <https://github.com/jackyf>`_.

* [Breaking] Changed the library name from ``format`` to ``cppformat``
  for consistency with the project name and to avoid potential conflicts
  (`#178`_).
  Thanks to `@jackyf (Eugene V. Lyubimkin) <https://github.com/jackyf>`_.

* C++ Format is now available in `Debian <https://www.debian.org/>`_ GNU/Linux
  (`stretch <https://packages.debian.org/source/stretch/cppformat>`_,
  `sid <https://packages.debian.org/source/sid/cppformat>`_) and 
  derived distributions such as
  `Ubuntu <https://launchpad.net/ubuntu/+source/cppformat>`_ 15.10 and later
  (`#155`_)::

    $ sudo apt-get install libcppformat1-dev

  Thanks to `@jackyf (Eugene V. Lyubimkin) <https://github.com/jackyf>`_.

* `Packages for Fedora and RHEL
  <https://admin.fedoraproject.org/pkgdb/package/cppformat/>`_ are now
  available. Thanks to Dave Johansen.
  
* C++ Format can now be installed via `Homebrew <http://brew.sh/>`_ on OS X
  (`#157`_)::

    $ brew install cppformat

  Thanks to `@ortho <https://github.com/ortho>`_, Anatoliy Bulukin.

Documentation
~~~~~~~~~~~~~

* Migrated from ReadTheDocs to GitHub Pages for better responsiveness
  and reliability (`#128`_).
  New documentation address is http://cppformat.github.io/.


* Added `Building the documentation
  <https://fmt.dev/2.0.0/usage.html#building-the-documentation>`_
  section to the documentation.

* Documentation build script is now compatible with Python 3 and newer pip
  versions (`#189`_, `#209`_).
  Thanks to `@JodiTheTigger <https://github.com/JodiTheTigger>`_ and
  `@xentec <https://github.com/xentec>`_.
  
* Documentation fixes and improvements
  (`#36`_, `#75`_, `#125`_, `#160`_, `#161`_, `#162`_, `#165`_, `#210`_).
  Thanks to `@syohex (Syohei YOSHIDA) <https://github.com/syohex>`_ and
  bug reporters.

* Fixed out-of-tree documentation build (`#177`_).
  Thanks to `@jackyf (Eugene V. Lyubimkin) <https://github.com/jackyf>`_.

Fixes
~~~~~

* Fixed ``initializer_list`` detection (`#136`_).
  Thanks to `@Gachapen (Magnus Bjerke Vik) <https://github.com/Gachapen>`_.

* [Breaking] Fixed formatting of enums with numeric format specifiers in
  ``fmt::(s)printf`` (`#131`_, `#139`_):

  .. code:: c++

    enum { ANSWER = 42 };
    fmt::printf("%d", ANSWER);

  Thanks to `@Naios <https://github.com/Naios>`_.

* Improved compatibility with old versions of MinGW
  (`#129`_, `#130`_, `#132`_).
  Thanks to `@cstamford (Christopher Stamford) <https://github.com/cstamford>`_.

* Fixed a compile error on MSVC with disabled exceptions (`#144`_).

* Added a workaround for broken implementation of variadic templates in MSVC2012
  (`#148`_).

* Placed the anonymous namespace within ``fmt`` namespace for the header-only
  configuration (`#171`_).
  Thanks to `@alfps (Alf P. Steinbach) <https://github.com/alfps>`_.

* Fixed issues reported by Coverity Scan (`#187`_, `#192`_).

* Implemented a workaround for a name lookup bug in MSVC2010 (`#188`_).

* Fixed compiler warnings (`#95`_, `#96`_, `#114`_, `#135`_, `#142`_, `#145`_,
  `#146`_, `#158`_, `#163`_, `#175`_, `#190`_, `#191`_, `#194`_, `#196`_,
  `#216`_, `#218`_, `#220`_, `#229`_, `#233`_, `#234`_, `#236`_, `#281`_,
  `#289`_). Thanks to
  `@seanmiddleditch (Sean Middleditch) <https://github.com/seanmiddleditch>`_,
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

* Fixed portability issues (mostly causing test failures) on ARM, ppc64,
  ppc64le, s390x and SunOS 5.11 i386 (`#138`_, `#179`_, `#180`_, `#202`_,
  `#225`_, `Red Hat Bugzilla Bug 1260297
  <https://bugzilla.redhat.com/show_bug.cgi?id=1260297>`_).
  Thanks to `@Naios <https://github.com/Naios>`_,
  `@jackyf (Eugene V. Lyubimkin) <https://github.com/jackyf>`_ and
  Dave Johansen.

* Fixed a name conflict with macro ``free`` defined in ``crtdbg.h`` when
  ``_CRTDBG_MAP_ALLOC`` is set (`#211`_).

* Fixed shared library build on OS X (`#212`_).
  Thanks to `@dean0x7d (Dean Moldovan) <https://github.com/dean0x7d>`_.

* Fixed an overload conflict on MSVC when ``/Zc:wchar_t-`` option is specified
  (`#214`_).
  Thanks to `@slavanap (Vyacheslav Napadovsky) <https://github.com/slavanap>`_.

* Improved compatibility with MSVC 2008 (`#236`_).
  Thanks to `@Jopie64 (Johan) <https://github.com/Jopie64>`_.

* Improved compatibility with bcc32 (`#227`_).

* Fixed ``static_assert`` detection on Clang (`#228`_).
  Thanks to `@dean0x7d (Dean Moldovan) <https://github.com/dean0x7d>`_.

1.1.0 - 2015-03-06
------------------

* Added ``BasicArrayWriter``, a class template that provides operations for
  formatting and writing data into a fixed-size array
  (`#105`_ and `#122`_):

  .. code:: c++
  
    char buffer[100];
    fmt::ArrayWriter w(buffer);
    w.write("The answer is {}", 42);

* Added `0 A.D. <http://play0ad.com/>`_ and `PenUltima Online (POL)
  <http://www.polserver.com/>`_ to the list of notable projects using C++ Format.

* C++ Format now uses MSVC intrinsics for better formatting performance
  (`#115`_, `#116`_, `#118`_ and `#121`_).
  Previously these optimizations where only used on GCC and Clang.
  Thanks to `@CarterLi <https://github.com/CarterLi>`_ and
  `@objectx <https://github.com/objectx>`_.

* CMake install target (`#119`_).
  Thanks to `@TrentHouliston <https://github.com/TrentHouliston>`_.

  You can now install C++ Format with ``make install`` command.

* Improved `Biicode <http://www.biicode.com/>`_ support
  (`#98`_ and `#104`_). Thanks to
  `@MariadeAnton <https://github.com/MariadeAnton>`_ and
  `@franramirez688 <https://github.com/franramirez688>`_.

* Improved support for building with `Android NDK
  <https://developer.android.com/tools/sdk/ndk/index.html>`_ (`#107`_).
  Thanks to `@newnon <https://github.com/newnon>`_.
  
  The `android-ndk-example <https://github.com/fmtlib/android-ndk-example>`_
  repository provides and example of using C++ Format with Android NDK:

  .. image:: https://raw.githubusercontent.com/fmtlib/android-ndk-example/
            master/screenshot.png

* Improved documentation of ``SystemError`` and ``WindowsError`` (`#54`_).

* Various code improvements (`#110`_, `#111`_ `#112`_).
  Thanks to `@CarterLi <https://github.com/CarterLi>`_.

* Improved compile-time errors when formatting wide into narrow strings
  (`#117`_).

* Fixed ``BasicWriter::write`` without formatting arguments when C++11 support
  is disabled (`#109`_).

* Fixed header-only build on OS X with GCC 4.9 (`#124`_).

* Fixed packaging issues (`#94`_).

* Added `changelog <https://github.com/fmtlib/fmt/blob/master/ChangeLog.rst>`_
  (`#103`_).

1.0.0 - 2015-02-05
------------------

* Add support for a header-only configuration when ``FMT_HEADER_ONLY`` is
  defined before including ``format.h``:

  .. code:: c++

    #define FMT_HEADER_ONLY
    #include "format.h"

* Compute string length in the constructor of ``BasicStringRef``
  instead of the ``size`` method
  (`#79`_).
  This eliminates size computation for string literals on reasonable optimizing
  compilers.

* Fix formatting of types with overloaded ``operator <<`` for ``std::wostream``
  (`#86`_):

  .. code:: c++

    fmt::format(L"The date is {0}", Date(2012, 12, 9));

* Fix linkage of tests on Arch Linux (`#89`_).

* Allow precision specifier for non-float arguments (`#90`_):

  .. code:: c++

    fmt::print("{:.3}\n", "Carpet"); // prints "Car"

* Fix build on Android NDK (`#93`_)

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

* Support for custom memory allocators (`#69`_)

* Formatting functions now accept `signed char` and `unsigned char` strings as
  arguments (`#73`_):

  .. code:: c++

    auto s = format("GLSL version: {}", glGetString(GL_VERSION));

* Reduced code bloat. According to the new `benchmark results
  <https://github.com/fmtlib/fmt#compile-time-and-code-bloat>`_,
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

* Arguments of ``char`` type can now be formatted as integers (`#55`_):

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
  lifetimes. Because the wrapper functions are very lightweight, this doesn't
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
  Issue `#50`_.

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
