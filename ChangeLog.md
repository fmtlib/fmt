7.0.0 - TBD
===========

* Reduced the library size. For example, on macOS a stripped binary
  statically linked with {fmt} [shrank from \~368k to less than 100k](
  http://www.zverovich.net/2020/05/21/reducing-library-size.html).

* Added a simpler and more efficient [format string compilation API](
  https://fmt.dev/dev/api.html#compile-api):

  ```c++
  #include <fmt/compile.h>

  // Converts 42 into std::string using the most efficient method and no
  // runtime format string processing.
  std::string s = fmt::format(FMT_COMPILE("{}"), 42);
  ```

  The old `fmt::compile` API is now deprecated.

* Optimized integer formatting: `format_to` with format string compilation and
  a stack-allocated buffer is now [faster than `to_chars` on both libc++ and
  libstdc++](
  http://www.zverovich.net/2020/06/13/fast-int-to-string-revisited.html).

* Applied extern templates to improve compile times when using the core API and
  `fmt/format.h` (#1452). For example, on macOS with clang the compile time
  of a test TU dropped from 2.3s to 0.3s with `-O2` and from 0.6s to 0.3s with
  the default settings (`-O0`).

  Before (`-O2`):

      % time c++ -c test.cc -I include -std=c++17 -O2
      c++ -c test.cc -I include -std=c++17 -O2  2.22s user 0.08s system 99% cpu 2.311 total

  After (`-O2`):

      % time c++ -c test.cc -I include -std=c++17 -O2
      c++ -c test.cc -I include -std=c++17 -O2  0.26s user 0.04s system 98% cpu 0.303 total

  Before (default):

      % time c++ -c test.cc -I include -std=c++17
      c++ -c test.cc -I include -std=c++17  0.53s user 0.06s system 98% cpu 0.601 total

  After (default):

      % time c++ -c test.cc -I include -std=c++17
      c++ -c test.cc -I include -std=c++17  0.24s user 0.06s system 98% cpu 0.301 total

  It is still recommended to use `fmt/core.h` instead of `fmt/format.h` but the
  compile time difference is now smaller.

  Thanks [\@alex3d](https://github.com/alex3d) for the suggestion.

* Named arguments are now stored on stack (no dynamic memory allocations) and
  the generated binary code is more compact and efficient. For example

  ```c++
  #include <fmt/core.h>

  int main() {
    fmt::print("The answer is {answer}\n", fmt::arg("answer", 42));
  }
  ```

  compiles to just ([godbolt](https://godbolt.org/z/NcfEp_))

  ```asm
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
  ```

* Implemented compile-time checks for dynamic width and precision (#1614):

  ```c++
  #include <fmt/format.h>

  int main() {
    fmt::print(FMT_STRING("{0:{1}}"), 42);
  }
  ```

  now gives a compilation error because argument 1 doesn\'t exist:

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

* Added sentinel support to `fmt::join` (#1689):

  ```c++
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
  ```

  Thanks [\@BRevzin (Barry Revzin)](https://github.com/BRevzin).

* Added support for named args, `clear` and `reserve` to
  `dynamic_format_arg_store` (#1655, #1663, #1674, #1677).
  Thanks [\@vsolontsov-ll (Vladimir
  Solontsov)](https://github.com/vsolontsov-ll).

* Added support for the `'c'` format specifier to integral types for
  compatibility with `std::format` (#1652).

* Implemented the `'L'` format specifier for locale-specific formatting of
  floating-point numbers for compatibility with `std::format` (#1624). The
  `'n'` specifier is now disabled by default but can be enabled via the
  `FMT_DEPRECATED_N_SPECIFIER` macro.

* The `'='` format specifier is now disabled by default for compatibility with
  `std::format`. It can be enabled via the `FMT_DEPRECATED_NUMERIC_ALIGN` macro.

* Optimized handling of small format strings. For example,

  ```c++
  fmt::format("Result: {}: ({},{},{},{})", str1, str2, str3, str4, str5)
  ```

  is now \~40% faster (#1685).

* Improved compatibility between `fmt::printf` with the standard specs
  (#1595, #1682, #1683, #1687, #1699, #1717).
  Thanks [\@rimathia](https://github.com/rimathia).

* Fixed handling of `` operator<<` overloads that use ``copyfmt\`[
  (]{.title-ref}\#1666\`\_).

* Removed the following deprecated APIs:

  * `FMT_STRING_ALIAS` and `fmt` macros - replaced by `FMT_STRING`
  * `fmt::basic_string_view::char_type` - replaced by
      `fmt::basic_string_view::value_type`
  * `convert_to_int`
  * `format_arg_store::types`
  * `*parse_context` - replaced by `*format_parse_context`
  * `FMT_DEPRECATED_INCLUDE_OS`
  * `FMT_DEPRECATED_PERCENT` - incompatible with `std::format`
  * `*writer` - replaced by compiled format API

* Deprecated `arg_formatter`.

* Renamed the `internal` namespace to `detail` (#1538). The former is still
  provided as an alias if the `FMT_USE_INTERNAL` macro is defined.

* Added the `FMT_OS` CMake option to control inclusion of OS-specific APIs in
  the fmt target. This can be useful for embedded platforms (#1654,
  #1656). Thanks [\@kwesolowski (Krzysztof
  Wesolowski)](https://github.com/kwesolowski).

* Replaced `FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION` with the `FMT_FUZZ` macro
  to prevent interferring with fuzzing of projects using {fmt} (#1650).
  Thanks [\@asraa (Asra Ali)](https://github.com/asraa).

* Fixed compatibility with emscripten (#1736, #1737). Thanks
  [\@ArthurSonzogni (Arthur Sonzogni)](https://github.com/ArthurSonzogni).

* Improved documentation (#704, #1643, #1660, #1681,
  #1691, #1706, #1714, #1721, #1739, #1740,
  #1741). Thanks [\@senior7515 (Alexander
  Gallego)](https://github.com/senior7515), [\@lsr0 (Lindsay
  Roberts)](https://github.com/lsr0), [\@puetzk (Kevin
  Puetz)](https://github.com/puetzk), [\@fpelliccioni (Fernando
  Pelliccioni)](https://github.com/fpelliccioni), Alexey Kuzmenko, [\@jelly
  (jelle van der Waa)](https://github.com/jelly), [\@claremacrae (Clare
  Macrae)](https://github.com/claremacrae), [\@jiapengwen
  (文佳鹏)](https://github.com/jiapengwen), [\@gsjaardema (Greg
  Sjaardema)](https://github.com/gsjaardema).

* Implemented various build configuration fixes and improvements (#1603,
  #1657, #1702, #1728). Thanks [\@scramsby (Scott
  Ramsby)](https://github.com/scramsby), [\@jtojnar (Jan
  Tojnar)](https://github.com/jtojnar), [\@orivej (Orivej
  Desh)](https://github.com/orivej), [\@flagarde](https://github.com/flagarde).

* Fixed various warnings and compilation issues (#1616, #1620,
  #1622, #1625, #1627, #1628, #1629, #1631,
  #1633, #1649, #1658, #1661, #1667, #1668,
  #1669, #1692, #1696, #1697, #1707, #1712,
  #1716, #1722, #1724, #1729, #1738, #1742,
  #1743, #1744, #1747, #1750). Thanks [\@gsjaardema (Greg
  Sjaardema)](https://github.com/gsjaardema), [\@gabime (Gabi
  Melman)](https://github.com/gabime), [\@johnor
  (Johan)](https://github.com/johnor), [\@gabime (Dmitry
  Kurkin)](https://github.com/Kurkin), [\@invexed (James
  Beach)](https://github.com/invexed),
  [\@peterbell10](https://github.com/peterbell10), [\@daixtrose (Markus
  Werle)](https://github.com/daixtrose), [\@petrutlucian94 (Lucian
  Petrut)](https://github.com/petrutlucian94), [\@Neargye (Daniil
  Goncharov)](https://github.com/Neargye), [\@ambitslix (Attila M.
  Szilagyi)](https://github.com/ambitslix), [\@gabime (Gabi
  Melman)](https://github.com/gabime), [\@erthink (Leonid
  Yuriev)](https://github.com/erthink), [\@tohammer (Tobias Hammer
  )](https://github.com/tohammer), [\@0x8000-0000 (Florin
  Iucha)](https://github.com/0x8000-0000).

6.2.1 - 2020-05-09
==================

* Fixed ostream support in `sprintf` (#1631).
* Fixed type detection when using implicit conversion to `string_view` and
  ostream `operator<<` inconsistently (#1662).

6.2.0 - 2020-04-05
==================

* Improved error reporting when trying to format an object of a non-formattable
  type:

  ```c++
  fmt::format("{}", S());
  ```

  now gives:

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

  if `S` is not formattable.

* Reduced the library size by \~10%.

* Always print decimal point if `#` is specified (#1476, #1498):

  ```c++
  fmt::print("{:#.0f}", 42.0);
  ```

  now prints `42.`

* Implemented the `'L'` specifier for locale-specific numeric formatting to
  improve compatibility with `std::format`. The `'n'` specifier is now
  deprecated and will be removed in the next major release.

* Moved OS-specific APIs such as `windows_error` from `fmt/format.h` to
  `fmt/os.h`. You can define `FMT_DEPRECATED_INCLUDE_OS` to automatically
  include `fmt/os.h` from `fmt/format.h` for compatibility but this will be
  disabled in the next major release.

* Added precision overflow detection in floating-point formatting.

* Implemented detection of invalid use of `fmt::arg`.

* Used `type_identity` to block unnecessary template argument deduction. Thanks
  Tim Song.

* Improved UTF-8 handling (#1109):

  ```c++
  fmt::print("┌{0:─^{2}}┐\n"
              "│{1: ^{2}}│\n"
              "└{0:─^{2}}┘\n", "", "Привет, мир!", 20);
  ```

  now prints:

      ┌────────────────────┐
      │    Привет, мир!    │
      └────────────────────┘

  on systems that support Unicode.

* Added experimental dynamic argument storage (#1170, #1584):

  ```c++
  fmt::dynamic_format_arg_store<fmt::format_context> store;
  store.push_back("answer");
  store.push_back(42);
  fmt::vprint("The {} is {}.\n", store);
  ```

  prints:

      The answer is 42.

  Thanks [\@vsolontsov-ll (Vladimir
  Solontsov)](https://github.com/vsolontsov-ll).

* Made `fmt::join` accept `initializer_list` (#1591). Thanks [\@Rapotkinnik
  (Nikolay Rapotkin)](https://github.com/Rapotkinnik).

* Fixed handling of empty tuples (#1588).

* Fixed handling of output iterators in `format_to_n` (#1506).

* Fixed formatting of `std::chrono::duration` types to wide output (#1533).
  Thanks [\@zeffy (pilao)](https://github.com/zeffy).

* Added const `begin` and `end` overload to buffers (#1553). Thanks
  [\@dominicpoeschko](https://github.com/dominicpoeschko).

* Added the ability to disable floating-point formatting via `FMT_USE_FLOAT`,
  `FMT_USE_DOUBLE` and `FMT_USE_LONG_DOUBLE` macros for extremely
  memory-constrained embedded system (#1590). Thanks [\@albaguirre (Alberto
  Aguirre)](https://github.com/albaguirre).

* Made `FMT_STRING` work with `constexpr` `string_view` (#1589). Thanks
  [\@scramsby (Scott Ramsby)](https://github.com/scramsby).

* Implemented a minor optimization in the format string parser (#1560).
  Thanks [\@IkarusDeveloper](https://github.com/IkarusDeveloper).

* Improved attribute detection (#1469, #1475, #1576). Thanks
  [\@federico-busato (Federico)](https://github.com/federico-busato),
  [\@chronoxor (Ivan Shynkarenka)](https://github.com/chronoxor),
  [\@refnum](https://github.com/refnum).

* Improved documentation (#1481, #1523). Thanks [\@JackBoosY
  (Jack·Boos·Yu)](https://github.com/JackBoosY), [\@imba-tjd
  (谭九鼎)](https://github.com/imba-tjd).

* Fixed symbol visibility on Linux when compiling with `-fvisibility=hidden`
  (#1535). Thanks [\@milianw (Milian Wolff)](https://github.com/milianw).

* Implemented various build configuration fixes and improvements (#1264,
  #1460, #1534, #1536, #1545, #1546, #1566,
  #1582, #1597, #1598). Thanks [\@ambitslix (Attila M.
  Szilagyi)](https://github.com/ambitslix), [\@jwillikers (Jordan
  Williams)](https://github.com/jwillikers), [\@stac47 (Laurent
  Stacul)](https://github.com/stac47).

* Fixed various warnings and compilation issues (#1433, #1461,
  #1470, #1480, #1485, #1492, #1493, #1504,
  #1505, #1512, #1515, #1516, #1518, #1519,
  #1520, #1521, #1522, #1524, #1530, #1531,
  #1532, #1539, #1547, #1548, #1554, #1567,
  #1568, #1569, #1571, #1573, #1575, #1581,
  #1583, #1586, #1587, #1594, #1596, #1604,
  #1606, #1607, #1609). Thanks [\@marti4d (Chris
  Martin)](https://github.com/marti4d),
  [\@iPherian](https://github.com/iPherian),
  [\@parkertomatoes](https://github.com/parkertomatoes), [\@gsjaardema (Greg
  Sjaardema)](https://github.com/gsjaardema), [\@chronoxor (Ivan
  Shynkarenka)](https://github.com/chronoxor), [\@DanielaE (Daniela
  Engert)](https://github.com/DanielaE),
  [\@torsten48](https://github.com/torsten48), [\@tohammer (Tobias
  Hammer)](https://github.com/tohammer), [\@lefticus (Jason
  Turner)](https://github.com/lefticus), [\@ryusakki
  (Haise)](https://github.com/ryusakki), [\@adnsv (Alex
  Denisov)](https://github.com/adnsv), [\@fghzxm](https://github.com/fghzxm),
  [\@refnum](https://github.com/refnum), [\@pramodk (Pramod
  Kumbhar)](https://github.com/pramodk),
  [\@Spirrwell](https://github.com/Spirrwell), [\@scramsby (Scott
  Ramsby)](https://github.com/scramsby).

6.1.2 - 2019-12-11
==================

* Fixed ABI compatibility with `libfmt.so.6.0.0` (#1471).
* Fixed handling types convertible to `std::string_view` (#1451). Thanks
  [\@denizevrenci (Deniz Evrenci)](https://github.com/denizevrenci).
* Made CUDA test an opt-in enabled via the `FMT_CUDA_TEST` CMake option.
* Fixed sign conversion warnings (#1440). Thanks [\@0x8000-0000 (Florin
  Iucha)](https://github.com/0x8000-0000).

6.1.1 - 2019-12-04
==================

* Fixed shared library build on Windows (#1443, #1445, #1446,
  #1450). Thanks [\@egorpugin (Egor Pugin)](https://github.com/egorpugin),
  [\@bbolli (Beat Bolli)](https://github.com/bbolli).
* Added a missing decimal point in exponent notation with trailing zeros.
* Removed deprecated `format_arg_store::TYPES`.

6.1.0 - 2019-12-01
==================

* {fmt} now formats IEEE 754 `float` and `double` using the shortest decimal
  representation with correct rounding by default:

  ```c++
  #include <cmath>
  #include <fmt/core.h>

  int main() {
    fmt::print("{}", M_PI);
  }
  ```

  prints `3.141592653589793`.

* Made the fast binary to decimal floating-point formatter the default,
  simplified it and improved performance. {fmt} is now 15 times faster than
  libc++\'s `std::ostringstream`, 11 times faster than `printf` and 10% faster
  than double-conversion on
  [dtoa-benchmark](https://github.com/fmtlib/dtoa-benchmark):

  +---------------+------------+----------+
  | Function      | Time (ns)  | Speedup  |
  +===============+============+==========+
  | ostringstream | > 1,346.30 | > 1.00x  |
  +---------------+------------+----------+
  | ostrstream    | > 1,195.74 | > 1.13x  |
  +---------------+------------+----------+
  | sprintf       | > 995.08   | > 1.35x  |
  +---------------+------------+----------+
  | doubleconv    | > 99.10    | > 13.59x |
  +---------------+------------+----------+
  | fmt           | > 88.34    | > 15.24x |
  +---------------+------------+----------+

  ![image](https://user-images.githubusercontent.com/576385/%0A69767160-cdaca400-112f-11ea-9fc5-347c9f83caad.png)

* {fmt} no longer converts `float` arguments to `double`. In particular this
  improves the default (shortest) representation of floats and makes
  `fmt::format` consistent with `std::format` specs (#1336, #1353,
  #1360, #1361):

  ```c++
  fmt::print("{}", 0.1f);
  ```

  prints `0.1` instead of `0.10000000149011612`.

  Thanks [\@orivej (Orivej Desh)](https://github.com/orivej).

* Made floating-point formatting output consistent with `printf`/iostreams
  (#1376, #1417).

* Added support for 128-bit integers (#1287):

  ```c++
  fmt::print("{}", std::numeric_limits<__int128_t>::max());
  ```

  prints `170141183460469231731687303715884105727`.

  Thanks [\@denizevrenci (Deniz Evrenci)](https://github.com/denizevrenci).

* The overload of `print` that takes `text_style` is now atomic, i.e. the output
  from different threads doesn\'t interleave (#1351). Thanks [\@tankiJong
  (Tanki Zhang)](https://github.com/tankiJong).

* Made compile time in the header-only mode \~20% faster by reducing the number
  of template instantiations. `wchar_t` overload of `vprint` was moved from
  `fmt/core.h` to `fmt/format.h`.

* Added an overload of `fmt::join` that works with tuples (#1322,
  #1330):

  ```c++
  #include <tuple>
  #include <fmt/ranges.h>

  int main() {
    std::tuple<char, int, float> t{'a', 1, 2.0f};
    fmt::print("{}", t);
  }
  ```

  prints `('a', 1, 2.0)`.

  Thanks [\@jeremyong (Jeremy Ong)](https://github.com/jeremyong).

* Changed formatting of octal zero with prefix from \"00\" to \"0\":

  ```c++
  fmt::print("{:#o}", 0);
  ```

  prints `0`.

* The locale is now passed to ostream insertion (`<<`) operators (#1406):

  ```c++
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
  ```

  Thanks [\@dlaugt (Daniel Laügt)](https://github.com/dlaugt).

* Locale-specific number formatting now uses grouping (#1393, #1394).
  Thanks [\@skrdaniel](https://github.com/skrdaniel).

* Fixed handling of types with deleted implicit rvalue conversion to
  `const char**` (#1421):

  ```c++
  struct mystring {
    operator const char*() const&;
    operator const char*() &;
    operator const char*() const&& = delete;
    operator const char*() && = delete;
  };
  mystring str;
  fmt::print("{}", str); // now compiles
  ```

* Enums are now mapped to correct underlying types instead of `int`
  (#1286). Thanks [\@agmt (Egor Seredin)](https://github.com/agmt).

* Enum classes are no longer implicitly converted to `int` (#1424).

* Added `basic_format_parse_context` for consistency with C++20 `std::format`
  and deprecated `basic_parse_context`.

* Fixed handling of UTF-8 in precision (#1389, #1390). Thanks
  [\@tajtiattila (Attila Tajti)](https://github.com/tajtiattila).

* {fmt} can now be installed on Linux, macOS and Windows with
  [Conda](https://docs.conda.io/en/latest/) using its
  [conda-forge](https://conda-forge.org)
  [package](https://github.com/conda-forge/fmt-feedstock) (#1410):

      conda install -c conda-forge fmt

  Thanks [\@tdegeus (Tom de Geus)](https://github.com/tdegeus).

* Added a CUDA test (#1285, #1317). Thanks [\@luncliff (Park
  DongHa)](https://github.com/luncliff) and
  [\@risa2000](https://github.com/risa2000).

* Improved documentation (#1276, #1291, #1296, #1315,
  #1332, #1337, #1395 #1418). Thanks [\@waywardmonkeys
  (Bruce Mitchener)](https://github.com/waywardmonkeys), [\@pauldreik (Paul
  Dreik)](https://github.com/pauldreik), [\@jackoalan (Jack
  Andersen)](https://github.com/jackoalan).

* Various code improvements (#1358, #1407). Thanks [\@orivej (Orivej
  Desh)](https://github.com/orivej), [\@dpacbach (David P.
  Sicilia)](https://github.com/dpacbach),

* Fixed compile-time format string checks for user-defined types (#1292).

* Worked around a false positive in `unsigned-integer-overflow` sanitizer
  (#1377).

* Fixed various warnings and compilation issues (#1273, #1278,
  #1280, #1281, #1288, #1290, #1301, #1305,
  #1306, #1309, #1312, #1313, #1316, #1319,
  #1320, #1326, #1328, #1344, #1345, #1347,
  #1349, #1354, #1362, #1366, #1364, #1370,
  #1371, #1385, #1388, #1397, #1414, #1416,
  #1422 #1427, #1431, #1433). Thanks
  [\@hhb](https://github.com/hhb), [\@gsjaardema (Greg
  Sjaardema)](https://github.com/gsjaardema), [\@gabime (Gabi
  Melman)](https://github.com/gabime), [\@neheb (Rosen
  Penev)](https://github.com/neheb), [\@vedranmiletic (Vedran
  Miletić)](https://github.com/vedranmiletic), [\@dkavolis (Daumantas
  Kavolis)](https://github.com/dkavolis),
  [\@mwinterb](https://github.com/mwinterb), [\@orivej (Orivej
  Desh)](https://github.com/orivej), [\@denizevrenci (Deniz
  Evrenci)](https://github.com/denizevrenci)
  [\@leonklingele](https://github.com/leonklingele), [\@chronoxor (Ivan
  Shynkarenka)](https://github.com/chronoxor),
  [\@kent-tri](https://github.com/kent-tri), [\@0x8000-0000 (Florin
  Iucha)](https://github.com/0x8000-0000), [\@marti4d (Chris
  Martin)](https://github.com/marti4d).

6.0.0 - 2019-08-26
==================

* Switched to the [MIT
  license](https://github.com/fmtlib/fmt/blob/5a4b24613ba16cc689977c3b5bd8274a3ba1dd1f/LICENSE.rst)
  with an optional exception that allows distributing binary code without
  attribution.

* Floating-point formatting is now locale-independent by default:

  ```c++
  #include <locale>
  #include <fmt/core.h>

  int main() {
    std::locale::global(std::locale("ru_RU.UTF-8"));
    fmt::print("value = {}", 4.2);
  }
  ```

  prints \"value = 4.2\" regardless of the locale.

  For locale-specific formatting use the `n` specifier:

  ```c++
  std::locale::global(std::locale("ru_RU.UTF-8"));
  fmt::print("value = {:n}", 4.2);
  ```

  prints \"value = 4,2\".

* Added an experimental Grisu floating-point formatting algorithm implementation
  (disabled by default). To enable it compile with the `FMT_USE_GRISU` macro
  defined to 1:

  ```c++
  #define FMT_USE_GRISU 1
  #include <fmt/format.h>

  auto s = fmt::format("{}", 4.2); // formats 4.2 using Grisu
  ```

  With Grisu enabled, {fmt} is 13x faster than `std::ostringstream` (libc++) and
  10x faster than `sprintf` on
  [dtoa-benchmark](https://github.com/fmtlib/dtoa-benchmark) ([full
  results](https://fmt.dev/unknown_mac64_clang10.0.html)):

  ![image](https://user-images.githubusercontent.com/576385/%0A54883977-9fe8c000-4e28-11e9-8bde-272d122e7c52.jpg)

* Separated formatting and parsing contexts for consistency with [C++20
  std::format](http://eel.is/c++draft/format), removing the undocumented
  `basic_format_context::parse_context()` function.

* Added [oss-fuzz](https://github.com/google/oss-fuzz) support (#1199).
  Thanks [\@pauldreik (Paul Dreik)](https://github.com/pauldreik).

* `formatter` specializations now always take precedence over `operator<<`
  (#952):

  ```c++
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
  ```

* Introduced the experimental `fmt::compile` function that does format string
  compilation (#618, #1169, #1171):

  ```c++
  #include <fmt/compile.h>

  auto f = fmt::compile<int>("{}");
  std::string s = fmt::format(f, 42); // can be called multiple times to
                                      // format different values
  // s == "42"
  ```

  It moves the cost of parsing a format string outside of the format function
  which can be beneficial when identically formatting many objects of the same
  types. Thanks [\@stryku (Mateusz Janek)](https://github.com/stryku).

* Added experimental `%` format specifier that formats floating-point values as
  percentages (#1060, #1069, #1071):

  ```c++
  auto s = fmt::format("{:.1%}", 0.42); // s == "42.0%"
  ```

  Thanks [\@gawain-bolton (Gawain Bolton)](https://github.com/gawain-bolton).

* Implemented precision for floating-point durations (#1004, #1012):

  ```c++
  auto s = fmt::format("{:.1}", std::chrono::duration<double>(1.234));
  // s == 1.2s
  ```

  Thanks [\@DanielaE (Daniela Engert)](https://github.com/DanielaE).

* Implemented `chrono` format specifiers `%Q` and `%q` that give the value and
  the unit respectively (#1019):

  ```c++
  auto value = fmt::format("{:%Q}", 42s); // value == "42"
  auto unit  = fmt::format("{:%q}", 42s); // unit == "s"
  ```

  Thanks [\@DanielaE (Daniela Engert)](https://github.com/DanielaE).

* Fixed handling of dynamic width in chrono formatter:

  ```c++
  auto s = fmt::format("{0:{1}%H:%M:%S}", std::chrono::seconds(12345), 12);
  //                        ^ width argument index                     ^ width
  // s == "03:25:45    "
  ```

  Thanks Howard Hinnant.

* Removed deprecated `fmt/time.h`. Use `fmt/chrono.h` instead.

* Added `fmt::format` and `fmt::vformat` overloads that take `text_style`
  (#993, #994):

  ```c++
  #include <fmt/color.h>

  std::string message = fmt::format(fmt::emphasis::bold | fg(fmt::color::red),
                                    "The answer is {}.", 42);
  ```

  Thanks [\@Naios (Denis Blank)](https://github.com/Naios).

* Removed the deprecated color API (`print_colored`). Use the new API, namely
  `print` overloads that take `text_style` instead.

* Made `std::unique_ptr` and `std::shared_ptr` formattable as pointers via
  `fmt::ptr` (#1121):

  ```c++
  std::unique_ptr<int> p = ...;
  fmt::print("{}", fmt::ptr(p)); // prints p as a pointer
  ```

  Thanks [\@sighingnow (Tao He)](https://github.com/sighingnow).

* Made `print` and `vprint` report I/O errors (#1098, #1099). Thanks
  [\@BillyDonahue (Billy Donahue)](https://github.com/BillyDonahue).

* Marked deprecated APIs with the `[[deprecated]]` attribute and removed
  internal uses of deprecated APIs (#1022). Thanks [\@eliaskosunen (Elias
  Kosunen)](https://github.com/eliaskosunen).

* Modernized the codebase using more C++11 features and removing workarounds.
  Most importantly, `buffer_context` is now an alias template, so use
  `buffer_context<T>` instead of `buffer_context<T>::type`. These features
  require GCC 4.8 or later.

* `formatter` specializations now always take precedence over implicit
  conversions to `int` and the undocumented `convert_to_int` trait is now
  deprecated.

* Moved the undocumented `basic_writer`, `writer`, and `wwriter` types to the
  `internal` namespace.

* Removed deprecated `basic_format_context::begin()`. Use `out()` instead.

* Disallowed passing the result of `join` as an lvalue to prevent misuse.

* Refactored the undocumented structs that represent parsed format specifiers to
  simplify the API and allow multibyte fill.

* Moved SFINAE to template parameters to reduce symbol sizes.

* Switched to `fputws` for writing wide strings so that it\'s no longer required
  to call `_setmode` on Windows (#1229, #1243). Thanks [\@jackoalan
  (Jack Andersen)](https://github.com/jackoalan).

* Improved literal-based API (#1254). Thanks [\@sylveon (Charles
  Milette)](https://github.com/sylveon).

* Added support for exotic platforms without `uintptr_t` such as IBM i (AS/400)
  which has 128-bit pointers and only 64-bit integers (#1059).

* Added [Sublime Text syntax highlighting
  config](https://github.com/fmtlib/fmt/blob/master/support/C%2B%2B.sublime-syntax)
  (#1037). Thanks [\@Kronuz (Germán Méndez
  Bravo)](https://github.com/Kronuz).

* Added the `FMT_ENFORCE_COMPILE_STRING` macro to enforce the use of
  compile-time format strings (#1231). Thanks [\@jackoalan (Jack
  Andersen)](https://github.com/jackoalan).

* Stopped setting `CMAKE_BUILD_TYPE` if {fmt} is a subproject (#1081).

* Various build improvements (#1039, #1078, #1091, #1103,
  #1177). Thanks [\@luncliff (Park DongHa)](https://github.com/luncliff),
  [\@jasonszang (Jason Shuo Zang)](https://github.com/jasonszang), [\@olafhering
  (Olaf Hering)](https://github.com/olafhering),
  [\@Lecetem](https://github.com/Lectem), [\@pauldreik (Paul
  Dreik)](https://github.com/pauldreik).

* Improved documentation (#1049, #1051, #1083, #1113,
  #1114, #1146, #1180, #1250, #1252, #1265).
  Thanks [\@mikelui (Michael Lui)](https://github.com/mikelui), [\@foonathan
  (Jonathan Müller)](https://github.com/foonathan), [\@BillyDonahue (Billy
  Donahue)](https://github.com/BillyDonahue), [\@jwakely (Jonathan
  Wakely)](https://github.com/jwakely), [\@kaisbe (Kais Ben
  Salah)](https://github.com/kaisbe), [\@sdebionne (Samuel
  Debionne)](https://github.com/sdebionne).

* Fixed ambiguous formatter specialization in `fmt/ranges.h` (#1123).

* Fixed formatting of a non-empty `std::filesystem::path` which is an infinitely
  deep range of its components (#1268).

* Fixed handling of general output iterators when formatting characters
  (#1056, #1058). Thanks [\@abolz (Alexander
  Bolz)](https://github.com/abolz).

* Fixed handling of output iterators in `formatter` specialization for ranges
  (#1064).

* Fixed handling of exotic character types (#1188).

* Made chrono formatting work with exceptions disabled (#1062).

* Fixed DLL visibility issues (#1134, #1147). Thanks
  [\@denchat](https://github.com/denchat).

* Disabled the use of UDL template extension on GCC 9 (#1148).

* Removed misplaced `format` compile-time checks from `printf` (#1173).

* Fixed issues in the experimental floating-point formatter (#1072,
  #1129, #1153, #1155, #1210, #1222). Thanks
  [\@alabuzhev (Alex Alabuzhev)](https://github.com/alabuzhev).

* Fixed bugs discovered by fuzzing or during fuzzing integration (#1124,
  #1127, #1132, #1135, #1136, #1141, #1142,
  #1178, #1179, #1194). Thanks [\@pauldreik (Paul
  Dreik)](https://github.com/pauldreik).

* Fixed building tests on FreeBSD and Hurd (#1043). Thanks [\@jackyf
  (Eugene V. Lyubimkin)](https://github.com/jackyf).

* Fixed various warnings and compilation issues (#998, #1006,
  #1008, #1011, #1025, #1027, #1028, #1029,
  #1030, #1031, #1054, #1063, #1068, #1074,
  #1075, #1079, #1086, #1088, #1089, #1094,
  #1101, #1102, #1105, #1107, #1115, #1117,
  #1118, #1120, #1123, #1139, #1140, #1143,
  #1144, #1150, #1151, #1152, #1154, #1156,
  #1159, #1175, #1181, #1186, #1187, #1191,
  #1197, #1200, #1203, #1205, #1206, #1213,
  #1214, #1217, #1228, #1230, #1232, #1235,
  #1236, #1240). Thanks [\@DanielaE (Daniela
  Engert)](https://github.com/DanielaE),
  [\@mwinterb](https://github.com/mwinterb), [\@eliaskosunen (Elias
  Kosunen)](https://github.com/eliaskosunen),
  [\@morinmorin](https://github.com/morinmorin), [\@ricco19 (Brian
  Ricciardelli)](https://github.com/ricco19), [\@waywardmonkeys (Bruce
  Mitchener)](https://github.com/waywardmonkeys), [\@chronoxor (Ivan
  Shynkarenka)](https://github.com/chronoxor),
  [\@remyabel](https://github.com/remyabel), [\@pauldreik (Paul
  Dreik)](https://github.com/pauldreik), [\@gsjaardema (Greg
  Sjaardema)](https://github.com/gsjaardema), [\@rcane (Ronny
  Krüger)](https://github.com/rcane), [\@mocabe](https://github.com/mocabe),
  [\@denchat](https://github.com/denchat), [\@cjdb (Christopher Di
  Bella)](https://github.com/cjdb), [\@HazardyKnusperkeks (Björn
  Schäpers)](https://github.com/HazardyKnusperkeks), [\@vedranmiletic (Vedran
  Miletić)](https://github.com/vedranmiletic), [\@jackoalan (Jack
  Andersen)](https://github.com/jackoalan), [\@DaanDeMeyer (Daan De
  Meyer)](https://github.com/DaanDeMeyer), [\@starkmapper (Mark
  Stapper)](https://github.com/starkmapper).

5.3.0 - 2018-12-28
==================

* Introduced experimental chrono formatting support:

  ```c++
  #include <fmt/chrono.h>

  int main() {
    using namespace std::literals::chrono_literals;
    fmt::print("Default format: {} {}\n", 42s, 100ms);
    fmt::print("strftime-like format: {:%H:%M:%S}\n", 3h + 15min + 30s);
  }
  ```

  prints:

      Default format: 42s 100ms
      strftime-like format: 03:15:30

* Added experimental support for emphasis (bold, italic, underline,
  strikethrough), colored output to a file stream, and improved colored
  formatting API (#961, #967, #973):

  ```c++
  #include <fmt/color.h>

  int main() {
    print(fg(fmt::color::crimson) | fmt::emphasis::bold,
          "Hello, {}!\n", "world");
    print(fg(fmt::color::floral_white) | bg(fmt::color::slate_gray) |
          fmt::emphasis::underline, "Hello, {}!\n", "мир");
    print(fg(fmt::color::steel_blue) | fmt::emphasis::italic,
          "Hello, {}!\n", "世界");
  }
  ```

  prints the following on modern terminals with RGB color support:

  ![image](https://user-images.githubusercontent.com/576385/%0A50405788-b66e7500-076e-11e9-9592-7324d1f951d8.png)

  Thanks [\@Rakete1111 (Nicolas)](https://github.com/Rakete1111).

* Added support for 4-bit terminal colors (#968, #974)

  ```c++
  #include <fmt/color.h>

  int main() {
    print(fg(fmt::terminal_color::red), "stop\n");
  }
  ```

  Note that these colors vary by terminal:

  ![image](https://user-images.githubusercontent.com/576385/%0A50405925-dbfc7e00-0770-11e9-9b85-333fab0af9ac.png)

  Thanks [\@Rakete1111 (Nicolas)](https://github.com/Rakete1111).

* Parameterized formatting functions on the type of the format string
  (#880, #881, #883, #885, #897, #920). Any object
  of type `S` that has an overloaded `to_string_view(const S&)` returning
  `fmt::string_view` can be used as a format string:

  ```c++
  namespace my_ns {
  inline string_view to_string_view(const my_string& s) {
    return {s.data(), s.length()};
  }
  }

  std::string message = fmt::format(my_string("The answer is {}."), 42);
  ```

  Thanks [\@DanielaE (Daniela Engert)](https://github.com/DanielaE).

* Made `std::string_view` work as a format string (#898):

  ```c++
  auto message = fmt::format(std::string_view("The answer is {}."), 42);
  ```

  Thanks [\@DanielaE (Daniela Engert)](https://github.com/DanielaE).

* Added wide string support to compile-time format string checks (#924):

  ```c++
  print(fmt(L"{:f}"), 42); // compile-time error: invalid type specifier
  ```

  Thanks [\@XZiar](https://github.com/XZiar).

* Made colored print functions work with wide strings (#867):

  ```c++
  #include <fmt/color.h>

  int main() {
    print(fg(fmt::color::red), L"{}\n", 42);
  }
  ```

  Thanks [\@DanielaE (Daniela Engert)](https://github.com/DanielaE).

* Introduced experimental Unicode support (#628, #891):

  ```c++
  using namespace fmt::literals;
  auto s = fmt::format("{:*^5}"_u, "🤡"_u); // s == "**🤡**"_u
  ```

* Improved locale support:

  ```c++
  #include <fmt/locale.h>

  struct numpunct : std::numpunct<char> {
    protected:
    char do_thousands_sep() const override { return '~'; }
  };

  std::locale loc;
  auto s = fmt::format(std::locale(loc, new numpunct()), "{:n}", 1234567);
  // s == "1~234~567"
  ```

* Constrained formatting functions on proper iterator types (#921). Thanks
  [\@DanielaE (Daniela Engert)](https://github.com/DanielaE).

* Added `make_printf_args` and `make_wprintf_args` functions (#934). Thanks
  [\@tnovotny](https://github.com/tnovotny).

* Deprecated `fmt::visit`, `parse_context`, and `wparse_context`. Use
  `fmt::visit_format_arg`, `format_parse_context`, and `wformat_parse_context`
  instead.

* Removed undocumented `basic_fixed_buffer` which has been superseded by the
  iterator-based API (#873, #902). Thanks [\@superfunc (hollywood
  programmer)](https://github.com/superfunc).

* Disallowed repeated leading zeros in an argument ID:

  ```c++
  fmt::print("{000}", 42); // error
  ```

* Reintroduced support for gcc 4.4.

* Fixed compilation on platforms with exotic `double` (#878).

* Improved documentation (#164, #877, #901, #906,
  #979). Thanks [\@kookjr (Mathew Cucuzella)](https://github.com/kookjr),
  [\@DarkDimius (Dmitry Petrashko)](https://github.com/DarkDimius),
  [\@HecticSerenity](https://github.com/HecticSerenity).

* Added pkgconfig support which makes it easier to consume the library from
  meson and other build systems (#916). Thanks [\@colemickens (Cole
  Mickens)](https://github.com/colemickens).

* Various build improvements (#909, #926, #937, #953,
  #959). Thanks [\@tchaikov (Kefu Chai)](https://github.com/tchaikov),
  [\@luncliff (Park DongHa)](https://github.com/luncliff), [\@AndreasSchoenle
  (Andreas Schönle)](https://github.com/AndreasSchoenle),
  [\@hotwatermorning](https://github.com/hotwatermorning), [\@Zefz
  (JohanJansen)](https://github.com/Zefz).

* Improved `string_view` construction performance (#914). Thanks [\@gabime
  (Gabi Melman)](https://github.com/gabime).

* Fixed non-matching char types (#895). Thanks [\@DanielaE (Daniela
  Engert)](https://github.com/DanielaE).

* Fixed `format_to_n` with `std::back_insert_iterator` (#913). Thanks
  [\@DanielaE (Daniela Engert)](https://github.com/DanielaE).

* Fixed locale-dependent formatting (#905).

* Fixed various compiler warnings and errors (#882, #886, #933,
  #941, #931, #943, #954, #956, #962, #965,
  #977, #983, #989). Thanks [\@Luthaf (Guillaume
  Fraux)](https://github.com/Luthaf), [\@stevenhoving (Steven
  Hoving)](https://github.com/stevenhoving), [\@christinaa (Kristina
  Brooks)](https://github.com/christinaa), [\@lgritz (Larry
  Gritz)](https://github.com/lgritz), [\@DanielaE (Daniela
  Engert)](https://github.com/DanielaE), [\@0x8000-0000 (Sign
  Bit)](https://github.com/0x8000-0000),
  [\@liuping1997](https://github.com/liuping1997).

5.2.1 - 2018-09-21
==================

* Fixed `visit` lookup issues on gcc 7 & 8 (#870). Thanks
  [\@medithe](https://github.com/medithe).
* Fixed linkage errors on older gcc.
* Prevented `fmt/range.h` from specializing `fmt::basic_string_view` (#865,
  #868). Thanks [\@hhggit (dual)](https://github.com/hhggit).
* Improved error message when formatting unknown types (#872). Thanks
  [\@foonathan (Jonathan Müller)](https://github.com/foonathan),
* Disabled templated user-defined literals when compiled under nvcc (#875).
  Thanks [\@CandyGumdrop (Candy Gumdrop)](https://github.com/CandyGumdrop),
* Fixed `format_to` formatting to `wmemory_buffer` (#874).

5.2.0 - 2018-09-13
==================

* Optimized format string parsing and argument processing which resulted in up
  to 5x speed up on long format strings and significant performance boost on
  various benchmarks. For example, version 5.2 is 2.22x faster than 5.1 on
  decimal integer formatting with `format_to` (macOS, clang-902.0.39.2):

  +----------------------------------------------+-----------------------+---------+
  | Method                                       | Time, s               | Speedup |
  +==============================================+=======================+=========+
  | fmt::format 5.1                              | > 0.58                |         |
  +----------------------------------------------+-----------------------+---------+
  | fmt::format 5.2 fmt::format\_to 5.1          | > 0.35 0.51           | > 1.66x |
  +----------------------------------------------+-----------------------+---------+
  | fmt::format\_to 5.2 sprintf std::to\_string  | > 0.23 0.71 1.01 1.73 | > 2.22x |
  | std::stringstream                            |                       |         |
  +----------------------------------------------+-----------------------+---------+

* Changed the `fmt` macro from opt-out to opt-in to prevent name collisions. To
  enable it define the `FMT_STRING_ALIAS` macro to 1 before including
  `fmt/format.h`:

  ```c++
  #define FMT_STRING_ALIAS 1
  #include <fmt/format.h>
  std::string answer = format(fmt("{}"), 42);
  ```

* Added compile-time format string checks to `format_to` overload that takes
  `fmt::memory_buffer` (#783):

  ```c++
  fmt::memory_buffer buf;
  // Compile-time error: invalid type specifier.
  fmt::format_to(buf, fmt("{:d}"), "foo");
  ```

* Moved experimental color support to `fmt/color.h` and enabled the new API by
  default. The old API can be enabled by defining the `FMT_DEPRECATED_COLORS`
  macro.

* Added formatting support for types explicitly convertible to
  `fmt::string_view`:

  ```c++
  struct foo {
    explicit operator fmt::string_view() const { return "foo"; }
  };
  auto s = format("{}", foo());
  ```

  In particular, this makes formatting function work with `folly::StringPiece`.

* Implemented preliminary support for `char*_t` by replacing the `format`
  function overloads with a single function template parameterized on the string
  type.

* Added support for dynamic argument lists (#814, #819). Thanks
  [\@MikePopoloski (Michael Popoloski)](https://github.com/MikePopoloski).

* Reduced executable size overhead for embedded targets using newlib nano by
  making locale dependency optional (#839). Thanks [\@teajay-fr (Thomas
  Benard)](https://github.com/teajay-fr).

* Keep `noexcept` specifier when exceptions are disabled (#801, #810).
  Thanks [\@qis (Alexej Harm)](https://github.com/qis).

* Fixed formatting of user-defined types providing `operator<<` with
  `format_to_n` (#806). Thanks [\@mkurdej (Marek
  Kurdej)](https://github.com/mkurdej).

* Fixed dynamic linkage of new symbols (#808).

* Fixed global initialization issue (#807):

  ```c++
  // This works on compilers with constexpr support.
  static const std::string answer = fmt::format("{}", 42);
  ```

* Fixed various compiler warnings and errors (#804, #809, #811,
  #822, #827, #830, #838, #843, #844, #851,
  #852, #854). Thanks [\@henryiii (Henry
  Schreiner)](https://github.com/henryiii),
  [\@medithe](https://github.com/medithe), and [\@eliasdaler (Elias
  Daler)](https://github.com/eliasdaler).

5.1.0 - 2018-07-05
==================

* Added experimental support for RGB color output enabled with the
  `FMT_EXTENDED_COLORS` macro:

  ```c++
  #define FMT_EXTENDED_COLORS
  #define FMT_HEADER_ONLY // or compile fmt with FMT_EXTENDED_COLORS defined
  #include <fmt/format.h>

  fmt::print(fmt::color::steel_blue, "Some beautiful text");
  ```

  The old API (the `print_colored` and `vprint_colored` functions and the
  `color` enum) is now deprecated. (#762 #767). thanks [\@Remotion
  (Remo)](https://github.com/Remotion).

* Added quotes to strings in ranges and tuples (#766). Thanks [\@Remotion
  (Remo)](https://github.com/Remotion).

* Made `format_to` work with `basic_memory_buffer` (#776).

* Added `vformat_to_n` and `wchar_t` overload of `format_to_n` (#764,
  #769).

* Made `is_range` and `is_tuple_like` part of public (experimental) API to allow
  specialization for user-defined types (#751, #759). Thanks [\@drrlvn
  (Dror Levin)](https://github.com/drrlvn).

* Added more compilers to continuous integration and increased `FMT_PEDANTIC`
  warning levels (#736). Thanks [\@eliaskosunen (Elias
  Kosunen)](https://github.com/eliaskosunen).

* Fixed compilation with MSVC 2013.

* Fixed handling of user-defined types in `format_to` (#793).

* Forced linking of inline `vformat` functions into the library (#795).

* Fixed incorrect call to on\_align in `'{:}='` (#750).

* Fixed floating-point formatting to a non-back\_insert\_iterator with sign &
  numeric alignment specified (#756).

* Fixed formatting to an array with `format_to_n` (#778).

* Fixed formatting of more than 15 named arguments (#754).

* Fixed handling of compile-time strings when including `fmt/ostream.h`.
  (#768).

* Fixed various compiler warnings and errors (#742, #748, #752,
  #770, #775, #779, #780, #790, #792, #800).
  Thanks [\@Remotion (Remo)](https://github.com/Remotion), [\@gabime (Gabi
  Melman)](https://github.com/gabime), [\@foonathan (Jonathan
  Müller)](https://github.com/foonathan), [\@Dark-Passenger (Dhruv
  Paranjape)](https://github.com/Dark-Passenger), and [\@0x8000-0000 (Sign
  Bit)](https://github.com/0x8000-0000).

5.0.0 - 2018-05-21
==================

* Added a requirement for partial C++11 support, most importantly variadic
  templates and type traits, and dropped `FMT_VARIADIC_*` emulation macros.
  Variadic templates are available since GCC 4.4, Clang 2.9 and MSVC 18.0
  (2013). For older compilers use {fmt} [version
  4.x](https://github.com/fmtlib/fmt/releases/tag/4.1.0) which continues to be
  maintained and works with C++98 compilers.

* Renamed symbols to follow standard C++ naming conventions and proposed a
  subset of the library for standardization in [P0645R2 Text
  Formatting](https://wg21.link/P0645).

* Implemented `constexpr` parsing of format strings and [compile-time format
  string
  checks](https://fmt.dev/dev/api.html#compile-time-format-string-checks). For
  example

  ```c++
  #include <fmt/format.h>

  std::string s = format(fmt("{:d}"), "foo");
  ```

  gives a compile-time error because `d` is an invalid specifier for strings
  ([godbolt](https://godbolt.org/g/rnCy9Q)):

      ...
      <source>:4:19: note: in instantiation of function template specialization 'fmt::v5::format<S, char [4]>' requested here
        std::string s = format(fmt("{:d}"), "foo");
                        ^
      format.h:1337:13: note: non-constexpr function 'on_error' cannot be used in a constant expression
          handler.on_error("invalid type specifier");

  Compile-time checks require relaxed `constexpr` (C++14 feature) support. If
  the latter is not available, checks will be performed at runtime.

* Separated format string parsing and formatting in the extension API to enable
  compile-time format string processing. For example

  ```c++
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
  ```

  gives a compile-time error due to invalid format specifier
  ([godbolt](https://godbolt.org/g/2jQ1Dv)):

      ...
      <source>:12:45: error: expression '<throw-expression>' is not a constant expression
              throw format_error("invalid specifier");

* Added [iterator
  support](https://fmt.dev/dev/api.html#output-iterator-support):

  ```c++
  #include <vector>
  #include <fmt/format.h>

  std::vector<char> out;
  fmt::format_to(std::back_inserter(out), "{}", 42);
  ```

* Added the
  [format\_to\_n](https://fmt.dev/dev/api.html#_CPPv2N3fmt11format_to_nE8OutputItNSt6size_tE11string_viewDpRK4Args)
  function that restricts the output to the specified number of characters
  (#298):

  ```c++
  char out[4];
  fmt::format_to_n(out, sizeof(out), "{}", 12345);
  // out == "1234" (without terminating '\0')
  ```

* Added the
  [formatted\_size](https://fmt.dev/dev/api.html#_CPPv2N3fmt14formatted_sizeE11string_viewDpRK4Args)
  function for computing the output size:

  ```c++
  #include <fmt/format.h>

  auto size = fmt::formatted_size("{}", 12345); // size == 5
  ```

* Improved compile times by reducing dependencies on standard headers and
  providing a lightweight [core API](https://fmt.dev/dev/api.html#core-api):

  ```c++
  #include <fmt/core.h>

  fmt::print("The answer is {}.", 42);
  ```

  See [Compile time and code
  bloat](https://github.com/fmtlib/fmt#compile-time-and-code-bloat).

* Added the
  [make\_format\_args](https://fmt.dev/dev/api.html#_CPPv2N3fmt16make_format_argsEDpRK4Args)
  function for capturing formatting arguments:

  ```c++
  // Prints formatted error message.
  void vreport_error(const char *format, fmt::format_args args) {
    fmt::print("Error: ");
    fmt::vprint(format, args);
  }
  template <typename... Args>
  void report_error(const char *format, const Args & ... args) {
    vreport_error(format, fmt::make_format_args(args...));
  }
  ```

* Added the `make_printf_args` function for capturing `printf` arguments
  (#687, #694). Thanks [\@Kronuz (Germán Méndez
  Bravo)](https://github.com/Kronuz).

* Added prefix `v` to non-variadic functions taking `format_args` to distinguish
  them from variadic ones:

  ```c++
  std::string vformat(string_view format_str, format_args args);

  template <typename... Args>
  std::string format(string_view format_str, const Args & ... args);
  ```

* Added experimental support for formatting ranges, containers and tuple-like
  types in `fmt/ranges.h` (#735):

  ```c++
  #include <fmt/ranges.h>

  std::vector<int> v = {1, 2, 3};
  fmt::print("{}", v); // prints {1, 2, 3}
  ```

  Thanks [\@Remotion (Remo)](https://github.com/Remotion).

* Implemented `wchar_t` date and time formatting (#712):

  ```c++
  #include <fmt/time.h>

  std::time_t t = std::time(nullptr);
  auto s = fmt::format(L"The date is {:%Y-%m-%d}.", *std::localtime(&t));
  ```

  Thanks [\@DanielaE (Daniela Engert)](https://github.com/DanielaE).

* Provided more wide string overloads (#724). Thanks [\@DanielaE (Daniela
  Engert)](https://github.com/DanielaE).

* Switched from a custom null-terminated string view class to `string_view` in
  the format API and provided `fmt::string_view` which implements a subset of
  `std::string_view` API for pre-C++17 systems.

* Added support for `std::experimental::string_view` (#607):

  ```c++
  #include <fmt/core.h>
  #include <experimental/string_view>

  fmt::print("{}", std::experimental::string_view("foo"));
  ```

  Thanks [\@virgiliofornazin (Virgilio Alexandre
  Fornazin)](https://github.com/virgiliofornazin).

* Allowed mixing named and automatic arguments:

  ```c++
  fmt::format("{} {two}", 1, fmt::arg("two", 2));
  ```

* Removed the write API in favor of the [format
  API](https://fmt.dev/dev/api.html#format-api) with compile-time handling of
  format strings.

* Disallowed formatting of multibyte strings into a wide character target
  (#606).

* Improved documentation (#515, #614, #617, #661,
  #680). Thanks [\@ibell (Ian Bell)](https://github.com/ibell),
  [\@mihaitodor (Mihai Todor)](https://github.com/mihaitodor), and
  [\@johnthagen](https://github.com/johnthagen).

* Implemented more efficient handling of large number of format arguments.

* Introduced an inline namespace for symbol versioning.

* Added debug postfix `d` to the `fmt` library name (#636).

* Removed unnecessary `fmt/` prefix in includes (#397). Thanks [\@chronoxor
  (Ivan Shynkarenka)](https://github.com/chronoxor).

* Moved `fmt/*.h` to `include/fmt/*.h` to prevent irrelevant files and
  directories appearing on the include search paths when fmt is used as a
  subproject and moved source files to the `src` directory.

* Added qmake project file `support/fmt.pro` (#641). Thanks [\@cowo78
  (Giuseppe Corbelli)](https://github.com/cowo78).

* Added Gradle build file `support/build.gradle` (#649). Thanks [\@luncliff
  (Park DongHa)](https://github.com/luncliff).

* Removed `FMT_CPPFORMAT` CMake option.

* Fixed a name conflict with the macro `CHAR_WIDTH` in glibc (#616). Thanks
  [\@aroig (Abdó Roig-Maranges)](https://github.com/aroig).

* Fixed handling of nested braces in `fmt::join` (#638).

* Added `SOURCELINK_SUFFIX` for compatibility with Sphinx 1.5 (#497).
  Thanks [\@ginggs (Graham Inggs)](https://github.com/ginggs).

* Added a missing `inline` in the header-only mode (#626). Thanks [\@aroig
  (Abdó Roig-Maranges)](https://github.com/aroig).

* Fixed various compiler warnings (#640, #656, #679, #681,
  [\#705]{.title-ref}\_\_, #715, #717, #720, #723,
  #726, #730, #739). Thanks
  [\@peterbell10](https://github.com/peterbell10),
  [\@LarsGullik](https://github.com/LarsGullik), [\@foonathan (Jonathan
  Müller)](https://github.com/foonathan), [\@eliaskosunen (Elias
  Kosunen)](https://github.com/eliaskosunen), [\@christianparpart (Christian
  Parpart)](https://github.com/christianparpart), [\@DanielaE (Daniela
  Engert)](https://github.com/DanielaE), and
  [\@mwinterb](https://github.com/mwinterb).

* Worked around an MSVC bug and fixed several warnings (#653). Thanks
  [\@alabuzhev (Alex Alabuzhev)](https://github.com/alabuzhev).

* Worked around GCC bug 67371 (#682).

* Fixed compilation with `-fno-exceptions` (#655). Thanks [\@chenxiaolong
  (Andrew Gunnerson)](https://github.com/chenxiaolong).

* Made `constexpr remove_prefix` gcc version check tighter (#648).

* Renamed internal type enum constants to prevent collision with poorly written
  C libraries (#644).

* Added detection of `wostream operator<<` (#650).

* Fixed compilation on OpenBSD (#660). Thanks
  [\@hubslave](https://github.com/hubslave).

* Fixed compilation on FreeBSD 12 (#732). Thanks
  [\@dankm](https://github.com/dankm).

* Fixed compilation when there is a mismatch between `-std` options between the
  library and user code (#664).

* Fixed compilation with GCC 7 and `-std=c++11` (#734).

* Improved generated binary code on GCC 7 and older (#668).

* Fixed handling of numeric alignment with no width (#675).

* Fixed handling of empty strings in UTF8/16 converters (#676). Thanks
  [\@vgalka-sl (Vasili Galka)](https://github.com/vgalka-sl).

* Fixed formatting of an empty `string_view` (#689).

* Fixed detection of `string_view` on libc++ (#686).

* Fixed DLL issues (#696). Thanks
  [\@sebkoenig](https://github.com/sebkoenig).

* Fixed compile checks for mixing narrow and wide strings (#690).

* Disabled unsafe implicit conversion to `std::string` (#729).

* Fixed handling of reused format specs (as in `fmt::join`) for pointers
  (#725). Thanks [\@mwinterb](https://github.com/mwinterb).

* Fixed installation of `fmt/ranges.h` (#738). Thanks
  [\@sv1990](https://github.com/sv1990).

4.1.0 - 2017-12-20
==================

* Added `fmt::to_wstring()` in addition to `fmt::to_string()` (#559).
  Thanks [\@alabuzhev (Alex Alabuzhev)](https://github.com/alabuzhev).
* Added support for C++17 `std::string_view` (#571 and #578). Thanks
  [\@thelostt (Mário Feroldi)](https://github.com/thelostt) and
  [\@mwinterb](https://github.com/mwinterb).
* Enabled stream exceptions to catch errors (#581). Thanks
  [\@crusader-mike](https://github.com/crusader-mike).
* Allowed formatting of class hierarchies with `fmt::format_arg()` (#547).
  Thanks [\@rollbear (Björn Fahller)](https://github.com/rollbear).
* Removed limitations on character types (#563). Thanks [\@Yelnats321
  (Elnar Dakeshov)](https://github.com/Yelnats321).
* Conditionally enabled use of `std::allocator_traits` (#583). Thanks
  [\@mwinterb](https://github.com/mwinterb).
* Added support for `const` variadic member function emulation with
  `FMT_VARIADIC_CONST` (#591). Thanks [\@ludekvodicka (Ludek
  Vodicka)](https://github.com/ludekvodicka).
* Various bugfixes: bad overflow check, unsupported implicit type conversion
  when determining formatting function, test segfaults (#551), ill-formed
  macros (#542) and ambiguous overloads (#580). Thanks [\@xylosper
  (Byoung-young Lee)](https://github.com/xylosper).
* Prevented warnings on MSVC (#605, #602, and #545), clang
  (#582), GCC (#573), various conversion warnings (#609,
  #567, #553 and #553), and added `override` and `[[noreturn]]`
  (#549 and #555). Thanks [\@alabuzhev (Alex
  Alabuzhev)](https://github.com/alabuzhev), [\@virgiliofornazin (Virgilio
  Alexandre Fornazin)](https://gihtub.com/virgiliofornazin), [\@alexanderbock
  (Alexander Bock)](https://github.com/alexanderbock),
  [\@yumetodo](https://github.com/yumetodo), [\@VaderY (Császár
  Mátyás)](https://github.com/VaderY), [\@jpcima (JP
  Cimalando)](https://github.com/jpcima), [\@thelostt (Mário
  Feroldi)](https://github.com/thelostt), and [\@Manu343726 (Manu
  Sánchez)](https://github.com/Manu343726).
* Improved CMake: Used `GNUInstallDirs` to set installation location (#610)
  and fixed warnings (#536 and #556). Thanks [\@mikecrowe (Mike
  Crowe)](https://github.com/mikecrowe),
  [\@evgen231](https://github.com/evgen231) and [\@henryiii (Henry
  Schreiner)](https://github.com/henryiii).

4.0.0 - 2017-06-27
==================

* Removed old compatibility headers `cppformat/*.h` and CMake options
  (#527). Thanks [\@maddinat0r (Alex
  Martin)](https://github.com/maddinat0r).

* Added `string.h` containing `fmt::to_string()` as alternative to
  `std::to_string()` as well as other string writer functionality (#326 and
  #441):

  ```c++
  #include "fmt/string.h"

  std::string answer = fmt::to_string(42);
  ```

  Thanks to [\@glebov-andrey (Andrey Glebov)](https://github.com/glebov-andrey).

* Moved `fmt::printf()` to new `printf.h` header and allowed `%s` as generic
  specifier (#453), made `%.f` more conformant to regular `printf()`
  (#490), added custom writer support (#476) and implemented missing
  custom argument formatting (#339 and #340):

  ```c++
  #include "fmt/printf.h"

  // %s format specifier can be used with any argument type.
  fmt::printf("%s", 42);
  ```

  Thanks [\@mojoBrendan](https://github.com/mojoBrendan), [\@manylegged (Arthur
  Danskin)](https://github.com/manylegged) and [\@spacemoose (Glen
  Stark)](https://github.com/spacemoose). See also #360, #335 and
  #331.

* Added `container.h` containing a `BasicContainerWriter` to write to containers
  like `std::vector` (#450). Thanks [\@polyvertex (Jean-Charles
  Lefebvre)](https://github.com/polyvertex).

* Added `fmt::join()` function that takes a range and formats its elements
  separated by a given string (#466):

  ```c++
  #include "fmt/format.h"

  std::vector<double> v = {1.2, 3.4, 5.6};
  // Prints "(+01.20, +03.40, +05.60)".
  fmt::print("({:+06.2f})", fmt::join(v.begin(), v.end(), ", "));
  ```

  Thanks [\@olivier80](https://github.com/olivier80).

* Added support for custom formatting specifications to simplify customization
  of built-in formatting (#444). Thanks [\@polyvertex (Jean-Charles
  Lefebvre)](https://github.com/polyvertex). See also #439.

* Added `fmt::format_system_error()` for error code formatting (#323 and
  #526). Thanks [\@maddinat0r (Alex
  Martin)](https://github.com/maddinat0r).

* Added thread-safe `fmt::localtime()` and `fmt::gmtime()` as replacement for
  the standard version to `time.h` (#396). Thanks
  [\@codicodi](https://github.com/codicodi).

* Internal improvements to `NamedArg` and `ArgLists` (#389 and #390).
  Thanks [\@chronoxor](https://github.com/chronoxor).

* Fixed crash due to bug in `FormatBuf` (#493). Thanks
  [\@effzeh](https://github.com/effzeh). See also #480 and #491.

* Fixed handling of wide strings in `fmt::StringWriter`.

* Improved compiler error messages (#357).

* Fixed various warnings and issues with various compilers (#494,
  #499, #483, #485, #482, #475, #473 and
  #414). Thanks [\@chronoxor](https://github.com/chronoxor),
  [\@zhaohuaxishi](https://github.com/zhaohuaxishi), [\@pkestene (Pierre
  Kestener)](https://github.com/pkestene), [\@dschmidt (Dominik
  Schmidt)](https://github.com/dschmidt) and [\@0x414c (Alexey
  Gorishny)](https://github.com/0x414c) .

* Improved CMake: targets are now namespaced (#511 and #513), added
  support for header-only `printf.h` (#354), fixed issue with minimal
  supported library subset (#418, #419 and #420). Thanks
  [\@bjoernthiel (Bjoern Thiel)](https://github.com/bjoernthiel), [\@niosHD
  (Mario Werner)](https://github.com/niosHD), [\@LogicalKnight (Sean
  LK)](https://github.com/LogicalKnight) and [\@alabuzhev (Alex
  Alabuzhev)](https://github.com/alabuzhev).

* Improved documentation. Thanks to [\@pwm1234
  (Phil)](https://github.com/pwm1234) for #393.

3.0.2 - 2017-06-14
==================

* Added `FMT_VERSION` macro (#411).
* Used `FMT_NULL` instead of literal `0` (#409). Thanks [\@alabuzhev (Alex
  Alabuzhev)](https://github.com/alabuzhev).
* Added extern templates for `format_float` (#413).
* Fixed implicit conversion issue (#507).
* Fixed signbit detection (#423).
* Fixed naming collision (#425).
* Fixed missing intrinsic for C++/CLI (#457). Thanks [\@calumr (Calum
  Robinson)](https://github.com/calumr)
* Fixed Android detection (#458). Thanks [\@Gachapen (Magnus Bjerke
  Vik)](https://github.com/Gachapen).
* Use lean `windows.h` if not in header-only mode (#503). Thanks
  [\@Quentin01 (Quentin Buathier)](https://github.com/Quentin01).
* Fixed issue with CMake exporting C++11 flag (#445). Thanks [\@EricWF
  (Eric)](https://github.com/EricWF).
* Fixed issue with nvcc and MSVC compiler bug and MinGW (#505).
* Fixed DLL issues (#469 and #502). Thanks [\@richardeakin (Richard
  Eakin)](https://github.com/richardeakin) and [\@AndreasSchoenle (Andreas
  Schönle)](https://github.com/AndreasSchoenle).
* Fixed test compilation under FreeBSD (#433).
* Fixed various warnings (#403, #410 and #510). Thanks
  [\@Lecetem](https://github.com/Lectem), [\@chenhayat (Chen
  Hayat)](https://github.com/chenhayat) and
  [\@trozen](https://github.com/trozen).
* Worked around a broken `__builtin_clz` in clang with MS codegen (#519).
* Removed redundant include (#479).
* Fixed documentation issues.

3.0.1 - 2016-11-01
==================

* Fixed handling of thousands separator (#353).
* Fixed handling of `unsigned char` strings (#373).
* Corrected buffer growth when formatting time (#367).
* Removed warnings under MSVC and clang (#318, #250, also merged
  #385 and #361). Thanks [\@jcelerier (Jean-Michaël
  Celerier)](https://github.com/jcelerier) and [\@nmoehrle (Nils
  Moehrle)](https://github.com/nmoehrle).
* Fixed compilation issues under Android (#327, #345 and #381),
  FreeBSD (#358), Cygwin (#388), MinGW (#355) as well as other
  issues (#350, #366, #348, #402, #405). Thanks to
  [\@dpantele (Dmitry)](https://github.com/dpantele), [\@hghwng (Hugh
  Wang)](https://github.com/hghwng), [\@arvedarved (Tilman
  Keskinöz)](https://github.com/arvedarved), [\@LogicalKnight
  (Sean)](https://github.com/LogicalKnight) and [\@JanHellwig (Jan
  Hellwig)](https://github.com/janhellwig).
* Fixed some documentation issues and extended specification (#320,
  #333, #347, #362). Thanks to [\@smellman (Taro Matsuzawa aka.
  btm)](https://github.com/smellman).

3.0.0 - 2016-05-07
==================

* The project has been renamed from C++ Format (cppformat) to fmt for
  consistency with the used namespace and macro prefix (#307). Library
  headers are now located in the `fmt` directory:

  ```c++
  #include "fmt/format.h"
  ```

  Including `format.h` from the `cppformat` directory is deprecated but works
  via a proxy header which will be removed in the next major version.

  The documentation is now available at <https://fmt.dev>.

* Added support for
  [strftime](http://en.cppreference.com/w/cpp/chrono/c/strftime)-like [date and
  time formatting](https://fmt.dev/3.0.0/api.html#date-and-time-formatting)
  (#283):

  ```c++
  #include "fmt/time.h"

  std::time_t t = std::time(nullptr);
  // Prints "The date is 2016-04-29." (with the current date)
  fmt::print("The date is {:%Y-%m-%d}.", *std::localtime(&t));
  ```

* `std::ostream` support including formatting of user-defined types that provide
  overloaded `operator<<` has been moved to `fmt/ostream.h`:

  ```c++
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
  ```

* Added support for [custom argument
  formatters](https://fmt.dev/3.0.0/api.html#argument-formatters) (#235).

* Added support for locale-specific integer formatting with the `n` specifier
  (#305):

  ```c++
  std::setlocale(LC_ALL, "en_US.utf8");
  fmt::print("cppformat: {:n}\n", 1234567); // prints 1,234,567
  ```

* Sign is now preserved when formatting an integer with an incorrect `printf`
  format specifier (#265):

  ```c++
  fmt::printf("%lld", -42); // prints -42
  ```

  Note that it would be an undefined behavior in `std::printf`.

* Length modifiers such as `ll` are now optional in printf formatting functions
  and the correct type is determined automatically (#255):

  ```c++
  fmt::printf("%d", std::numeric_limits<long long>::max());
  ```

  Note that it would be an undefined behavior in `std::printf`.

* Added initial support for custom formatters (#231).

* Fixed detection of user-defined literal support on Intel C++ compiler
  (#311, #312). Thanks to [\@dean0x7d (Dean
  Moldovan)](https://github.com/dean0x7d) and [\@speth (Ray
  Speth)](https://github.com/speth).

* Reduced compile time (#243, #249, #317):

  ![image](https://cloud.githubusercontent.com/assets/4831417/11614060/%0Ab9e826d2-9c36-11e5-8666-d4131bf503ef.png)

  ![image](https://cloud.githubusercontent.com/assets/4831417/11614080/%0A6ac903cc-9c37-11e5-8165-26df6efae364.png)

  Thanks to [\@dean0x7d (Dean Moldovan)](https://github.com/dean0x7d).

* Compile test fixes (#313). Thanks to [\@dean0x7d (Dean
  Moldovan)](https://github.com/dean0x7d).

* Documentation fixes (#239, #248, #252, #258, #260,
  #301, #309). Thanks to
  [\@ReadmeCritic](https://github.com/ReadmeCritic) [\@Gachapen (Magnus Bjerke
  Vik)](https://github.com/Gachapen) and [\@jwilk (Jakub
  Wilk)](https://github.com/jwilk).

* Fixed compiler and sanitizer warnings (#244, #256, #259,
  #263, #274, #277, #286, #291, #296, #308)
  Thanks to [\@mwinterb](https://github.com/mwinterb), [\@pweiskircher (Patrik
  Weiskircher)](https://github.com/pweiskircher),
  [\@Naios](https://github.com/Naios).

* Improved compatibility with Windows Store apps (#280, #285). Thanks
  to [\@mwinterb](https://github.com/mwinterb).

* Added tests of compatibility with older C++ standards (#273). Thanks to
  [\@niosHD](https://github.com/niosHD).

* Fixed Android build (#271). Thanks to
  [\@newnon](https://github.com/newnon).

* Changed `ArgMap` to be backed by a vector instead of a map. (#261,
  #262). Thanks to [\@mwinterb](https://github.com/mwinterb).

* Added `fprintf` overload that writes to a `std::ostream` (#251). Thanks
  to [nickhutchinson (Nicholas Hutchinson)](https://github.com/nickhutchinson).

* Export symbols when building a Windows DLL (#245). Thanks to [macdems
  (Maciek Dems)](https://github.com/macdems).

* Fixed compilation on Cygwin (#304).

* Implemented a workaround for a bug in Apple LLVM version 4.2 of clang
  (#276).

* Implemented a workaround for Google Test bug
  [\#705](https://github.com/google/googletest/issues/705) on gcc 6 (#268).
  Thanks to [octoploid](https://github.com/octoploid).

* Removed Biicode support because the latter has been discontinued.

2.1.1 - 2016-04-11
==================

* The install location for generated CMake files is now configurable via the
  `FMT_CMAKE_DIR` CMake variable (#299). Thanks to
  [\@niosHD](https://github.com/niosHD).
* Documentation fixes (#252).

2.1.0 - 2016-03-21
==================

* Project layout and build system improvements (#267):

  * The code have been moved to the `cppformat` directory. Including
      `format.h` from the top-level directory is deprecated but works via a
      proxy header which will be removed in the next major version.
  * C++ Format CMake targets now have proper interface definitions.
  * Installed version of the library now supports the header-only
      configuration.
  * Targets `doc`, `install`, and `test` are now disabled if C++ Format is
      included as a CMake subproject. They can be enabled by setting `FMT_DOC`,
      `FMT_INSTALL`, and `FMT_TEST` in the parent project.

  Thanks to [\@niosHD](https://github.com/niosHD).

2.0.1 - 2016-03-13
==================

* Improved CMake find and package support (#264). Thanks to
  [\@niosHD](https://github.com/niosHD).
* Fix compile error with Android NDK and mingw32 (#241). Thanks to
  [\@Gachapen (Magnus Bjerke Vik)](https://github.com/Gachapen).
* Documentation fixes (#248, #260).

2.0.0 - 2015-12-01
==================

General
-------

* \[Breaking\] Named arguments (#169, #173, #174):

  ```c++
  fmt::print("The answer is {answer}.", fmt::arg("answer", 42));
  ```

  Thanks to [\@jamboree](https://github.com/jamboree).

* \[Experimental\] User-defined literals for format and named arguments
  (#204, #206, #207):

  ```c++
  using namespace fmt::literals;
  fmt::print("The answer is {answer}.", "answer"_a=42);
  ```

  Thanks to [\@dean0x7d (Dean Moldovan)](https://github.com/dean0x7d).

* \[Breaking\] Formatting of more than 16 arguments is now supported when using
  variadic templates (#141). Thanks to
  [\@Shauren](https://github.com/Shauren).

* Runtime width specification (#168):

  ```c++
  fmt::format("{0:{1}}", 42, 5); // gives "   42"
  ```

  Thanks to [\@jamboree](https://github.com/jamboree).

* \[Breaking\] Enums are now formatted with an overloaded `std::ostream`
  insertion operator (`operator<<`) if available (#232).

* \[Breaking\] Changed default `bool` format to textual, \"true\" or \"false\"
  (#170):

  ```c++
  fmt::print("{}", true); // prints "true"
  ```

  To print `bool` as a number use numeric format specifier such as `d`:

  ```c++
  fmt::print("{:d}", true); // prints "1"
  ```

* `fmt::printf` and `fmt::sprintf` now support formatting of `bool` with the
  `%s` specifier giving textual output, \"true\" or \"false\" (#223):

  ```c++
  fmt::printf("%s", true); // prints "true"
  ```

  Thanks to [\@LarsGullik](https://github.com/LarsGullik).

* \[Breaking\] `signed char` and `unsigned char` are now formatted as integers
  by default (#217).

* \[Breaking\] Pointers to C strings can now be formatted with the `p` specifier
  (#223):

  ```c++
  fmt::print("{:p}", "test"); // prints pointer value
  ```

  Thanks to [\@LarsGullik](https://github.com/LarsGullik).

* \[Breaking\] `fmt::printf` and `fmt::sprintf` now print null pointers as
  `(nil)` and null strings as `(null)` for consistency with glibc (#226).
  Thanks to [\@LarsGullik](https://github.com/LarsGullik).

* \[Breaking\] `fmt::(s)printf` now supports formatting of objects of
  user-defined types that provide an overloaded `std::ostream` insertion
  operator (`operator<<`) (#201):

  ```c++
  fmt::printf("The date is %s", Date(2012, 12, 9));
  ```

* \[Breaking\] The `Buffer` template is now part of the public API and can be
  used to implement custom memory buffers (#140). Thanks to [\@polyvertex
  (Jean-Charles Lefebvre)](https://github.com/polyvertex).

* \[Breaking\] Improved compatibility between `BasicStringRef` and
  [std::experimental::basic\_string\_view](http://en.cppreference.com/w/cpp/experimental/basic_string_view)
  (#100, #159, #183):

  * Comparison operators now compare string content, not pointers
  * `BasicStringRef::c_str` replaced by `BasicStringRef::data`
  * `BasicStringRef` is no longer assumed to be null-terminated

  References to null-terminated strings are now represented by a new class,
  `BasicCStringRef`.

* Dependency on pthreads introduced by Google Test is now optional (#185).

* New CMake options `FMT_DOC`, `FMT_INSTALL` and `FMT_TEST` to control
  generation of `doc`, `install` and `test` targets respectively, on by default
  (#197, #198, #200). Thanks to [\@maddinat0r (Alex
  Martin)](https://github.com/maddinat0r).

* `noexcept` is now used when compiling with MSVC2015 (#215). Thanks to
  [\@dmkrepo (Dmitriy)](https://github.com/dmkrepo).

* Added an option to disable use of `windows.h` when `FMT_USE_WINDOWS_H` is
  defined as 0 before including `format.h` (#171). Thanks to [\@alfps
  (Alf P. Steinbach)](https://github.com/alfps).

* \[Breaking\] `windows.h` is now included with `NOMINMAX` unless
  `FMT_WIN_MINMAX` is defined. This is done to prevent breaking code using
  `std::min` and `std::max` and only affects the header-only configuration
  (#152, #153, #154). Thanks to
  [\@DevO2012](https://github.com/DevO2012).

* Improved support for custom character types (#171). Thanks to [\@alfps
  (Alf P. Steinbach)](https://github.com/alfps).

* Added an option to disable use of IOStreams when `FMT_USE_IOSTREAMS` is
  defined as 0 before including `format.h` (#205, #208). Thanks to
  [\@JodiTheTigger](https://github.com/JodiTheTigger).

* Improved detection of `isnan`, `isinf` and `signbit`.

Optimization
------------

* Made formatting of user-defined types more efficient with a custom stream
  buffer (#92, #230). Thanks to
  [\@NotImplemented](https://github.com/NotImplemented).
* Further improved performance of `fmt::Writer` on integer formatting and fixed
  a minor regression. Now it is \~7% faster than `karma::generate` on Karma\'s
  benchmark (#186).
* \[Breaking\] Reduced [compiled code
  size](https://github.com/fmtlib/fmt#compile-time-and-code-bloat) (#143,
  #149).

Distribution
------------

* \[Breaking\] Headers are now installed in
  `${CMAKE_INSTALL_PREFIX}/include/cppformat` (#178). Thanks to [\@jackyf
  (Eugene V. Lyubimkin)](https://github.com/jackyf).

* \[Breaking\] Changed the library name from `format` to `cppformat` for
  consistency with the project name and to avoid potential conflicts
  (#178). Thanks to [\@jackyf (Eugene V.
  Lyubimkin)](https://github.com/jackyf).

* C++ Format is now available in [Debian](https://www.debian.org/) GNU/Linux
  ([stretch](https://packages.debian.org/source/stretch/cppformat),
  [sid](https://packages.debian.org/source/sid/cppformat)) and derived
  distributions such as [Ubuntu](https://launchpad.net/ubuntu/+source/cppformat)
  15.10 and later (#155):

      $ sudo apt-get install libcppformat1-dev

  Thanks to [\@jackyf (Eugene V. Lyubimkin)](https://github.com/jackyf).

* [Packages for Fedora and
  RHEL](https://admin.fedoraproject.org/pkgdb/package/cppformat/) are now
  available. Thanks to Dave Johansen.

* C++ Format can now be installed via [Homebrew](http://brew.sh/) on OS X
  (#157):

      $ brew install cppformat

  Thanks to [\@ortho](https://github.com/ortho), Anatoliy Bulukin.

Documentation
-------------

* Migrated from ReadTheDocs to GitHub Pages for better responsiveness and
  reliability (#128). New documentation address is
  <http://cppformat.github.io/>.
* Added [Building the
  documentation](https://fmt.dev/2.0.0/usage.html#building-the-documentation)
  section to the documentation.
* Documentation build script is now compatible with Python 3 and newer pip
  versions (#189, #209). Thanks to
  [\@JodiTheTigger](https://github.com/JodiTheTigger) and
  [\@xentec](https://github.com/xentec).
* Documentation fixes and improvements (#36, #75, #125,
  #160, #161, #162, #165, #210). Thanks to [\@syohex
  (Syohei YOSHIDA)](https://github.com/syohex) and bug reporters.
* Fixed out-of-tree documentation build (#177). Thanks to [\@jackyf
  (Eugene V. Lyubimkin)](https://github.com/jackyf).

Fixes
-----

* Fixed `initializer_list` detection (#136). Thanks to [\@Gachapen (Magnus
  Bjerke Vik)](https://github.com/Gachapen).

* \[Breaking\] Fixed formatting of enums with numeric format specifiers in
  `fmt::(s)printf` (#131, #139):

  ```c++
  enum { ANSWER = 42 };
  fmt::printf("%d", ANSWER);
  ```

  Thanks to [\@Naios](https://github.com/Naios).

* Improved compatibility with old versions of MinGW (#129, #130,
  #132). Thanks to [\@cstamford (Christopher
  Stamford)](https://github.com/cstamford).

* Fixed a compile error on MSVC with disabled exceptions (#144).

* Added a workaround for broken implementation of variadic templates in MSVC2012
  (#148).

* Placed the anonymous namespace within `fmt` namespace for the header-only
  configuration (#171). Thanks to [\@alfps (Alf P.
  Steinbach)](https://github.com/alfps).

* Fixed issues reported by Coverity Scan (#187, #192).

* Implemented a workaround for a name lookup bug in MSVC2010 (#188).

* Fixed compiler warnings (#95, #96, #114, #135, #142,
  #145, #146, #158, #163, #175, #190, #191,
  #194, #196, #216, #218, #220, #229, #233,
  #234, #236, #281, #289). Thanks to [\@seanmiddleditch
  (Sean Middleditch)](https://github.com/seanmiddleditch), [\@dixlorenz (Dix
  Lorenz)](https://github.com/dixlorenz), [\@CarterLi
  (李通洲)](https://github.com/CarterLi), [\@Naios](https://github.com/Naios),
  [\@fmatthew5876 (Matthew Fioravante)](https://github.com/fmatthew5876),
  [\@LevskiWeng (Levski Weng)](https://github.com/LevskiWeng),
  [\@rpopescu](https://github.com/rpopescu), [\@gabime (Gabi
  Melman)](https://github.com/gabime), [\@cubicool (Jeremy
  Moles)](https://github.com/cubicool), [\@jkflying (Julian
  Kent)](https://github.com/jkflying), [\@LogicalKnight
  (Sean L)](https://github.com/LogicalKnight), [\@inguin (Ingo van
  Lil)](https://github.com/inguin) and [\@Jopie64
  (Johan)](https://github.com/Jopie64).

* Fixed portability issues (mostly causing test failures) on ARM, ppc64,
  ppc64le, s390x and SunOS 5.11 i386 (#138, #179, #180,
  #202, #225, [Red Hat Bugzilla Bug
  1260297](https://bugzilla.redhat.com/show_bug.cgi?id=1260297)). Thanks to
  [\@Naios](https://github.com/Naios), [\@jackyf (Eugene V.
  Lyubimkin)](https://github.com/jackyf) and Dave Johansen.

* Fixed a name conflict with macro `free` defined in `crtdbg.h` when
  `_CRTDBG_MAP_ALLOC` is set (#211).

* Fixed shared library build on OS X (#212). Thanks to [\@dean0x7d (Dean
  Moldovan)](https://github.com/dean0x7d).

* Fixed an overload conflict on MSVC when `/Zc:wchar_t-` option is specified
  (#214). Thanks to [\@slavanap (Vyacheslav
  Napadovsky)](https://github.com/slavanap).

* Improved compatibility with MSVC 2008 (#236). Thanks to [\@Jopie64
  (Johan)](https://github.com/Jopie64).

* Improved compatibility with bcc32 (#227).

* Fixed `static_assert` detection on Clang (#228). Thanks to [\@dean0x7d
  (Dean Moldovan)](https://github.com/dean0x7d).

1.1.0 - 2015-03-06
==================

* Added `BasicArrayWriter`, a class template that provides operations for
  formatting and writing data into a fixed-size array (#105 and #122):

  ```c++
  char buffer[100];
  fmt::ArrayWriter w(buffer);
  w.write("The answer is {}", 42);
  ```

* Added [0 A.D.](http://play0ad.com/) and [PenUltima Online
  (POL)](http://www.polserver.com/) to the list of notable projects using C++
  Format.

* C++ Format now uses MSVC intrinsics for better formatting performance
  (#115, #116, #118 and #121). Previously these
  optimizations where only used on GCC and Clang. Thanks to
  [\@CarterLi](https://github.com/CarterLi) and
  [\@objectx](https://github.com/objectx).

* CMake install target (#119). Thanks to
  [\@TrentHouliston](https://github.com/TrentHouliston).

  You can now install C++ Format with `make install` command.

* Improved [Biicode](http://www.biicode.com/) support (#98 and #104).
  Thanks to [\@MariadeAnton](https://github.com/MariadeAnton) and
  [\@franramirez688](https://github.com/franramirez688).

* Improved support for building with [Android
  NDK](https://developer.android.com/tools/sdk/ndk/index.html) (#107).
  Thanks to [\@newnon](https://github.com/newnon).

  The [android-ndk-example](https://github.com/fmtlib/android-ndk-example)
  repository provides and example of using C++ Format with Android NDK:

  ![image](https://raw.githubusercontent.com/fmtlib/android-ndk-example/%0Amaster/screenshot.png)

* Improved documentation of `SystemError` and `WindowsError` (#54).

* Various code improvements (#110, #111 #112). Thanks to
  [\@CarterLi](https://github.com/CarterLi).

* Improved compile-time errors when formatting wide into narrow strings
  (#117).

* Fixed `BasicWriter::write` without formatting arguments when C++11 support is
  disabled (#109).

* Fixed header-only build on OS X with GCC 4.9 (#124).

* Fixed packaging issues (#94).

* Added [changelog](https://github.com/fmtlib/fmt/blob/master/ChangeLog.rst)
  (#103).

1.0.0 - 2015-02-05
==================

* Add support for a header-only configuration when `FMT_HEADER_ONLY` is defined
  before including `format.h`:

  ```c++
  #define FMT_HEADER_ONLY
  #include "format.h"
  ```

* Compute string length in the constructor of `BasicStringRef` instead of the
  `size` method (#79). This eliminates size computation for string literals
  on reasonable optimizing compilers.

* Fix formatting of types with overloaded `operator <<` for `std::wostream`
  (#86):

  ```c++
  fmt::format(L"The date is {0}", Date(2012, 12, 9));
  ```

* Fix linkage of tests on Arch Linux (#89).

* Allow precision specifier for non-float arguments (#90):

  ```c++
  fmt::print("{:.3}\n", "Carpet"); // prints "Car"
  ```

* Fix build on Android NDK (#93)

* Improvements to documentation build procedure.

* Remove `FMT_SHARED` CMake variable in favor of standard
  [BUILD\_SHARED\_LIBS](http://www.cmake.org/cmake/help/v3.0/variable/BUILD_SHARED_LIBS.html).

* Fix error handling in `fmt::fprintf`.

* Fix a number of warnings.

0.12.0 - 2014-10-25
===================

* \[Breaking\] Improved separation between formatting and buffer management.
  `Writer` is now a base class that cannot be instantiated directly. The new
  `MemoryWriter` class implements the default buffer management with small
  allocations done on stack. So `fmt::Writer` should be replaced with
  `fmt::MemoryWriter` in variable declarations.

  Old code:

  ```c++
  fmt::Writer w;
  ```

  New code:

  ```c++
  fmt::MemoryWriter w;
  ```

  If you pass `fmt::Writer` by reference, you can continue to do so:

  ```c++
  void f(fmt::Writer &w);
  ```

  This doesn\'t affect the formatting API.

* Support for custom memory allocators (#69)

* Formatting functions now accept [signed char]{.title-ref} and [unsigned
  char]{.title-ref} strings as arguments (#73):

  ```c++
  auto s = format("GLSL version: {}", glGetString(GL_VERSION));
  ```

* Reduced code bloat. According to the new [benchmark
  results](https://github.com/fmtlib/fmt#compile-time-and-code-bloat), cppformat
  is close to `printf` and by the order of magnitude better than Boost Format in
  terms of compiled code size.

* Improved appearance of the documentation on mobile by using the [Sphinx
  Bootstrap theme](http://ryan-roemer.github.io/sphinx-bootstrap-theme/):

  +-------+-------+
  | > Old | > New |
  +-------+-------+
  |       |       |
  +-------+-------+

0.11.0 - 2014-08-21
===================

* Safe printf implementation with a POSIX extension for positional arguments:

  ```c++
  fmt::printf("Elapsed time: %.2f seconds", 1.23);
  fmt::printf("%1$s, %3$d %2$s", weekday, month, day);
  ```

* Arguments of `char` type can now be formatted as integers (#55):

  ```c++
  fmt::format("0x{0:02X}", 'a');
  ```

* Deprecated parts of the API removed.

* The library is now built and tested on MinGW with Appveyor in addition to
  existing test platforms Linux/GCC, OS X/Clang, Windows/MSVC.

0.10.0 - 2014-07-01
===================

**Improved API**

* All formatting methods are now implemented as variadic functions instead of
  using `operator<<` for feeding arbitrary arguments into a temporary formatter
  object. This works both with C++11 where variadic templates are used and with
  older standards where variadic functions are emulated by providing lightweight
  wrapper functions defined with the `FMT_VARIADIC` macro. You can use this
  macro for defining your own portable variadic functions:

  ```c++
  void report_error(const char *format, const fmt::ArgList &args) {
    fmt::print("Error: {}");
    fmt::print(format, args);
  }
  FMT_VARIADIC(void, report_error, const char *)

  report_error("file not found: {}", path);
  ```

  Apart from a more natural syntax, this also improves performance as there is
  no need to construct temporary formatter objects and control arguments\'
  lifetimes. Because the wrapper functions are very lightweight, this doesn\'t
  cause code bloat even in pre-C++11 mode.

* Simplified common case of formatting an `std::string`. Now it requires a
  single function call:

  ```c++
  std::string s = format("The answer is {}.", 42);
  ```

  Previously it required 2 function calls:

  ```c++
  std::string s = str(Format("The answer is {}.") << 42);
  ```

  Instead of unsafe `c_str` function, `fmt::Writer` should be used directly to
  bypass creation of `std::string`:

  ```c++
  fmt::Writer w;
  w.write("The answer is {}.", 42);
  w.c_str();  // returns a C string
  ```

  This doesn\'t do dynamic memory allocation for small strings and is less error
  prone as the lifetime of the string is the same as for `std::string::c_str`
  which is well understood (hopefully).

* Improved consistency in naming functions that are a part of the public API.
  Now all public functions are lowercase following the standard library
  conventions. Previously it was a combination of lowercase and
  CapitalizedWords. Issue #50.

* Old functions are marked as deprecated and will be removed in the next
  release.

**Other Changes**

* Experimental support for printf format specifications (work in progress):

  ```c++
  fmt::printf("The answer is %d.", 42);
  std::string s = fmt::sprintf("Look, a %s!", "string");
  ```

* Support for hexadecimal floating point format specifiers `a` and `A`:

  ```c++
  print("{:a}", -42.0); // Prints -0x1.5p+5
  print("{:A}", -42.0); // Prints -0X1.5P+5
  ```

* CMake option `FMT_SHARED` that specifies whether to build format as a shared
  library (off by default).

0.9.0 - 2014-05-13
==================

* More efficient implementation of variadic formatting functions.

* `Writer::Format` now has a variadic overload:

  ```c++
  Writer out;
  out.Format("Look, I'm {}!", "variadic");
  ```

* For efficiency and consistency with other overloads, variadic overload of the
  `Format` function now returns `Writer` instead of `std::string`. Use the `str`
  function to convert it to `std::string`:

  ```c++
  std::string s = str(Format("Look, I'm {}!", "variadic"));
  ```

* Replaced formatter actions with output sinks: `NoAction` -\> `NullSink`,
  `Write` -\> `FileSink`, `ColorWriter` -\> `ANSITerminalSink`. This improves
  naming consistency and shouldn\'t affect client code unless these classes are
  used directly which should be rarely needed.

* Added `ThrowSystemError` function that formats a message and throws
  `SystemError` containing the formatted message and system-specific error
  description. For example, the following code

  ```c++
  FILE *f = fopen(filename, "r");
  if (!f)
    ThrowSystemError(errno, "Failed to open file '{}'") << filename;
  ```

  will throw `SystemError` exception with description \"Failed to open file
  \'\<filename\>\': No such file or directory\" if file doesn\'t exist.

* Support for AppVeyor continuous integration platform.

* `Format` now throws `SystemError` in case of I/O errors.

* Improve test infrastructure. Print functions are now tested by redirecting the
  output to a pipe.

0.8.0 - 2014-04-14
==================

* Initial release
