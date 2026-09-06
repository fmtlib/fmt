<img src="https://user-images.githubusercontent.com/576385/156254208-f5b743a9-88cf-439d-b0c0-923d53e8d551.png" alt="{fmt}" width="25%"/>

[![image](https://github.com/fmtlib/fmt/actions/workflows/linux.yml/badge.svg?branch=master)](
https://github.com/fmtlib/fmt/actions?query=workflow%3Alinux)
[![image](https://github.com/fmtlib/fmt/actions/workflows/macos.yml/badge.svg?branch=master)](
https://github.com/fmtlib/fmt/actions?query=workflow%3Amacos)
[![image](https://github.com/fmtlib/fmt/actions/workflows/windows.yml/badge.svg?branch=master)](
https://github.com/fmtlib/fmt/actions?query=workflow%3Awindows)
[![fmt is continuously fuzzed at oss-fuzz](https://oss-fuzz-build-logs.storage.googleapis.com/badges/fmt.svg)](
https://issues.oss-fuzz.com/issues?q=title:fmt%20cc:victor.zverovich@gmail.com)
[![OpenSSF Best Practices](https://www.bestpractices.dev/projects/8880/badge)](
https://www.bestpractices.dev/projects/8880)
[![image](https://api.securityscorecards.dev/projects/github.com/fmtlib/fmt/badge)](
https://securityscorecards.dev/viewer/?uri=github.com/fmtlib/fmt)
[![Ask questions at StackOverflow with the tag fmt](
https://img.shields.io/badge/stackoverflow-fmt-blue.svg)](https://stackoverflow.com/questions/tagged/fmt)
[![Support Ukraine](
https://img.shields.io/badge/Support-Ukraine-005BBB?labelColor=FFD500)](https://novaukraine.org/)

**{fmt}** is an open-source formatting library providing a fast and safe
alternative to C stdio and C++ iostreams.

[Documentation](https://fmt.dev)

[Cheat Sheets](https://hackingcpp.com/cpp/libs/fmt.html)

Q&A: ask questions on [StackOverflow with the tag
fmt](https://stackoverflow.com/questions/tagged/fmt).

Try {fmt} in [Compiler Explorer](https://godbolt.org/z/8Mx1EW73v).

[![Live demo by Demoshell](https://build.demoshell.com/v1/embed/badge.svg)](https://build.demoshell.com/launch?snapshot=demoshell%2Ftools%3Afmt)

# Features

- Simple [format API](https://fmt.dev/latest/api/) with positional
  arguments for localization
- Implementation of [C++20
  std::format](https://en.cppreference.com/w/cpp/utility/format) and
  [C++23 std::print](https://en.cppreference.com/w/cpp/io/print)
- [Format string syntax](https://fmt.dev/latest/syntax/) similar
  to Python\'s
  [format](https://docs.python.org/3/library/stdtypes.html#str.format)
- Fast IEEE 754 floating-point formatter with correct rounding,
  shortness and round-trip guarantees using the
  [Dragonbox](https://github.com/jk-jeon/dragonbox) algorithm
- Portable Unicode support
- Safe [printf
  implementation](https://fmt.dev/latest/api/#printf-api)
  including the POSIX extension for positional arguments
- Extensibility: [support for user-defined
  types](https://fmt.dev/latest/api/#formatting-user-defined-types)
- High performance: faster than common standard library
  implementations of `(s)printf`, iostreams, `to_string` and
  `to_chars`, see [Speed tests](#speed-tests) and [Converting a
  hundred million integers to strings per
  second](https://vitaut.net/posts/2020/fast-int-to-string-revisited/)
- Small code size both in terms of source code with the minimum
  configuration consisting of just three files, `base.h`, `format.h`
  and `format-inl.h`, and compiled code; see [Compile time and code
  bloat](#compile-time-and-code-bloat)
- Reliability: the library has an extensive set of
  [tests](https://github.com/fmtlib/fmt/tree/master/test) and is
  [continuously fuzzed](https://bugs.chromium.org/p/oss-fuzz/issues/list?colspec=ID%20Type%20Component%20Status%20Proj%20Reported%20Owner%20Summary&q=proj%3Dfmt&can=1)
- Safety: the library is fully type-safe, errors in format strings can
  be reported at compile time, automatic memory management prevents
  buffer overflow errors
- Ease of use: small self-contained code base, no external
  dependencies, permissive MIT
  [license](https://github.com/fmtlib/fmt/blob/master/LICENSE)
- [Portability](https://fmt.dev/latest/#portability) with
  consistent output across platforms and support for older compilers
- Clean warning-free codebase even on high warning levels such as
  `-Wall -Wextra -pedantic`
- Locale independence by default
- Optional header-only configuration enabled with the
  `FMT_HEADER_ONLY` macro

See the [documentation](https://fmt.dev) for more details.

# Examples

**Print to stdout** ([run](https://godbolt.org/z/Tevcjh))

``` c++
#include <fmt/base.h>

int main() {
  fmt::print("Hello, world!\n");
}
```

**Format a string** ([run](https://godbolt.org/z/oK8h33))

``` c++
std::string s = fmt::format("The answer is {}.", 42);
// s == "The answer is 42."
```

**Format a string using positional arguments**
([run](https://godbolt.org/z/Yn7Txe))

``` c++
std::string s = fmt::format("I'd rather be {1} than {0}.", "right", "happy");
// s == "I'd rather be happy than right."
```

**Print dates and times** ([run](https://godbolt.org/z/c31ExdY3W))

``` c++
#include <fmt/chrono.h>

int main() {
  auto now = std::chrono::system_clock::now();
  fmt::print("Date and time: {}\n", now);
  fmt::print("Time: {:%H:%M}\n", now);
}
```

Output:

    Date and time: 2023-12-26 19:10:31.557195597
    Time: 19:10

**Print a container** ([run](https://godbolt.org/z/MxM1YqjE7))

``` c++
#include <vector>
#include <fmt/ranges.h>

int main() {
  std::vector<int> v = {1, 2, 3};
  fmt::print("{}\n", v);
}
```

Output:

    [1, 2, 3]

**Check a format string at compile time**

``` c++
std::string s = fmt::format("{:d}", "I am not a number");
```

This gives a compile-time error in C++20 because `d` is an invalid
format specifier for a string.

**Write a file from a single thread**

``` c++
#include <fmt/os.h>

int main() {
  auto out = fmt::output_file("guide.txt");
  out.print("Don't {}", "Panic");
}
```

This can be [up to 9 times faster than `fprintf`](
https://vitaut.net/posts/2020/optimal-file-buffer-size/).

**Print with colors and text styles**

``` c++
#include <fmt/color.h>

int main() {
  fmt::print(fg(fmt::color::crimson) | fmt::emphasis::bold,
             "Hello, {}!\n", "world");
  fmt::print(fg(fmt::color::floral_white) | bg(fmt::color::slate_gray) |
             fmt::emphasis::underline, "Olá, {}!\n", "Mundo");
  fmt::print(fg(fmt::color::steel_blue) | fmt::emphasis::italic,
             "你好{}！\n", "世界");
}
```

Output on a modern terminal with Unicode support:

![image](https://github.com/fmtlib/fmt/assets/576385/2a93c904-d6fa-4aa6-b453-2618e1c327d7)

# Performance

{fmt} can be tens of percent to 20–30 times faster than `sprintf` and
iostreams, especially for numeric formatting. It minimizes dynamic memory
allocations and can optionally [compile format strings](
https://fmt.dev/latest/api/#compile-api) into efficient formatting code.

See [format-benchmark](https://github.com/fmtlib/format-benchmark) and
[dtoa-benchmark](https://github.com/fmtlib/dtoa-benchmark) for benchmarks
and methodology.

**Time per double (smaller is better):**

[![Time per double for floating-point formatting methods](
https://github.com/user-attachments/assets/3678bc4a-9405-489e-8ce1-ca702829cdaa)](
https://github.com/fmtlib/dtoa-benchmark)

`ostringstream` and `sprintf` are omitted because they are an order of
magnitude slower than the other methods.

## Compile time and code bloat

The script [bloat-test.py][test] from [format-benchmark][bench] measures the
compile-time and code-size overhead each formatting method adds to application
code. It generates 100 translation units and uses `printf` or its alternative
five times in each to simulate a medium-sized project. Library and module build
costs are excluded. Results on an Apple M5 Max running macOS 26.6.2 with
Apple Clang 21.0.0 (clang-2100.1.1.101), taking the best of three runs, are
shown in the following tables.

[test]: https://github.com/fmtlib/format-benchmark/blob/master/bloat-test.py
[bench]: https://github.com/fmtlib/format-benchmark

**Optimized build (-O3)**

| Method             | Compile time, s | Binary size, KiB | Stripped size, KiB |
|--------------------|----------------:|-----------------:|-------------------:|
| printf             |             1.6 |               54 |                 50 |
| IOStreams          |            25.5 |               98 |                 84 |
| fmt 12.2 (headers) |             5.1 |               54 |                 50 |
| fmt 12.2 (module)  |             3.7 |               59 |                 50 |
| Boost Format 1.92  |            49.1 |              517 |                317 |

Using modular {fmt} reduces optimized application-code compile time by 27%
without changing the reported stripped binary size.

**Non-optimized build**

| Method             | Compile time, s | Binary size, KiB | Stripped size, KiB |
|--------------------|----------------:|-----------------:|-------------------:|
| printf             |             1.6 |               54 |                 50 |
| IOStreams          |            26.0 |               88 |                 68 |
| fmt 12.2 (headers) |             4.9 |               87 |                 84 |
| fmt 12.2 (module)  |             3.2 |               77 |                 68 |
| Boost Format 1.92  |            35.7 |              741 |                431 |

`libc`, `libc++`, `libfmt`, and `libfmt-module` were linked as shared libraries
to compare formatting function overhead only. Boost Format is header-only.

# Projects using {fmt}

Notable users include:

- [Apple's FoundationDB](https://github.com/apple/foundationdb)
- [Blizzard Battle.net](https://battle.net/)
- [Ceph](https://ceph.com/)
- [ClickHouse](https://github.com/ClickHouse/ClickHouse)
- [Envoy](https://github.com/envoyproxy/envoy)
- [Folly](https://github.com/facebook/folly)
- [MariaDB](https://mariadb.org/)
- [MongoDB](https://mongodb.com/)
- [PyTorch](https://github.com/pytorch/pytorch)
- [Quill](https://github.com/odygrd/quill)
- [Seastar](https://seastar.io/)
- [spdlog](https://github.com/gabime/spdlog)
- [Windows Terminal](https://github.com/microsoft/terminal)

[Find more projects using {fmt} on GitHub](
https://github.com/search?q=fmtlib&type=Code).
