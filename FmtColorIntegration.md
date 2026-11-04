# Using FmtColor from Make and CMake

FmtColor extends {fmt} with color and style values, owning styled text, SGR
parsing, and ANSI stripping. Development is on `color-dev`. Use this fork's
headers and matching library, or its header-only target. The complete styling
API supports C++11 and later; the application examples below target C++17.

## Select the headers

| Header                      | Use                                                        |
| --------------------------- | ---------------------------------------------------------- |
| `<fmt/color.h>`             | Color/style values, styled arguments, and SGR helpers.     |
| `<fmt/styled_text.h>`       | Owning narrow/wide text trees and their formatter.         |
| `<fmt/parse_styled_text.h>` | Parse standard ANSI SGR into owned styled text.            |
| `<fmt/strip_ansi.h>`        | Strip ANSI sequences from strings or styled-text leaves.   |
| `<fmt/color-ostream.h>`     | Styled narrow/wide ostream output and stream insertion.    |
| `<fmt/xchar.h>`             | Wide-character formatting and printing entry points.       |

The parser and stripper headers include `styled_text.h`, which includes
`color.h`. Include the headers for the APIs your application uses. The
[user guide](https://tjgolubi.github.io/fmt/color/index.html) covers their
complete interfaces and behavior.

## Select the SGR output mode

There are two SGR output modes:

`FMT_COLOR_COMPACT_SGR` effect table:

| Value | Output                                                                                                                             |
| ----- | ---------------------------------------------------------------------------------------------------------------------------------- |
| `0`   | Legacy output: separate SGR sequences for emphasis, foreground, and background, with three-digit RGB and indexed-color parameters. |
| `1`   | Compact output: one SGR sequence per style, with decimal parameters that have no leading zeros.                                    |

Both modes express the same terminal styling. Nested styled arguments and
styled-text trees inherit and restore the enclosing formatting style. At the
top level, that enclosing style is normally terminal defaults. An absolute
styled-text node emits an SGR reset before applying its own style, unless there
is no output. An explicit absolute target to `to_sgr(target, initial)` resets
in both modes, even when the attributes already match.

`fmt::to_sgr(style)` returns only the style's opening SGR sequences; it returns
an empty string for an empty relative style. It does not append a reset.

For bold text with RGB foreground `(1,2,3)` and terminal-black background,
the opening sequences are:

```text
Legacy:  \x1b[1m\x1b[38;2;001;002;003m\x1b[40m
Compact: \x1b[1;38;2;1;2;3;40m
```

Here `\x1b` denotes the actual ESC character.

If `FMT_COLOR_COMPACT_SGR` is undefined, the effect is the same as `0`.

## Build and install the library

Run these commands from this fork's source directory in a Bash shell.
The commands below use GCC, including the MSYS2 toolchain on Windows. Use a
matching toolchain for the library and consuming application; do not mix an
MSYS2 library with native MSVC objects. Native MSVC builds can use the same
CMake targets with their matching installation.

The development script builds the library and runs tests in either mode.
The default build also generates documentation; the TaskWarrior build uses
`FMT_DOC=OFF` and skips documentation generation:

```bash
./build.bash              # Legacy mode, in build/
./build.bash taskwarrior  # Compact mode, in build/taskwarrior/
```

Both modes prepare the shared documentation-tool environment, even though
TaskWarrior mode does not generate the guide. The script therefore requires
the project's test and documentation tools. For an
application-only installation, build the static library without those tools:

```bash
cmake -S . -B build/install \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=g++ \
  -DFMT_TEST=OFF \
  -DFMT_DOC=OFF \
  -DFMT_INSTALL=ON \
  -DBUILD_SHARED_LIBS=OFF \
  -DCMAKE_INSTALL_LIBDIR=lib \
  -DCMAKE_INSTALL_PREFIX="$HOME/.local/fmtcolor"
cmake --build build/install --parallel 8
cmake --install build/install
```

This installs headers, the static library, and CMake package files under the
chosen prefix. The examples below use that prefix and a static installation.

**Select the SGR mode when compiling the application.** The gated code lives
in `color.h`; the same installed headers and compiled library support either
mode. The installation does not automatically enable compact output in its
consumers, even when it comes from `build/taskwarrior`.

## Example application

Save this as `hello.cpp` in a separate application directory:

```cpp
#include <fmt/color.h>

int main() {
  const auto style = fmt::fg(fmt::rgb{1, 2, 3})
                         .bg(fmt::term_color::black).bold();
  fmt::print(style, "Hello from FmtColor!\n");
}
```

## Use a traditional Makefile

Save the following as `Makefile` alongside `hello.cpp`. Recipe lines must
start with a tab.

```makefile
CXX = g++
FMT_PREFIX ?= $(HOME)/.local/fmtcolor
COMPACT_SGR ?= 0
BUILD_DIR := build/sgr-$(COMPACT_SGR)

CPPFLAGS += -I$(FMT_PREFIX)/include -DFMT_COLOR_COMPACT_SGR=$(COMPACT_SGR)
CXXFLAGS += -O2 -std=c++17
LDFLAGS += -L$(FMT_PREFIX)/lib
LDLIBS += -lfmt

.PHONY: all
all: $(BUILD_DIR)/hello

$(BUILD_DIR)/hello: hello.cpp Makefile
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) hello.cpp $(LDFLAGS) $(LDLIBS) -o $@
```

Build and run legacy output:

```bash
make COMPACT_SGR=0
./build/sgr-0/hello
```

Build and run compact output:

```bash
make COMPACT_SGR=1
./build/sgr-1/hello
```

For another installation location, add `FMT_PREFIX=/path/to/fmtcolor` to the
`make` command. The separate output directories prevent reusing an executable
compiled for the other mode. For a larger application, apply the definition
to every source file that uses the styling header and link `-lfmt` after the
application's object files.

## Use CMake

Save this as `CMakeLists.txt` alongside `hello.cpp`:

```cmake
cmake_minimum_required(VERSION 3.16)
project(fmtcolor_hello LANGUAGES CXX)

option(USE_COMPACT_SGR "Use compact FmtColor SGR output" OFF)

find_package(fmt CONFIG REQUIRED)

add_executable(hello hello.cpp)
target_link_libraries(hello PRIVATE fmt::fmt)
target_compile_features(hello PRIVATE cxx_std_17)
target_compile_definitions(hello PRIVATE
  FMT_COLOR_COMPACT_SGR=$<BOOL:${USE_COMPACT_SGR}>)
```

Build and run legacy output:

```bash
cmake -S . -B build/legacy \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_PREFIX_PATH="$HOME/.local/fmtcolor" \
  -DUSE_COMPACT_SGR=OFF
cmake --build build/legacy --parallel 8
./build/legacy/hello
```

Build and run compact output:

```bash
cmake -S . -B build/compact \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_CXX_COMPILER=g++ \
  -DCMAKE_PREFIX_PATH="$HOME/.local/fmtcolor" \
  -DUSE_COMPACT_SGR=ON
cmake --build build/compact --parallel 8
./build/compact/hello
```

`USE_COMPACT_SGR` is an option introduced by this example. It becomes the
compiler definition `FMT_COLOR_COMPACT_SGR=0` or `1` through
`target_compile_definitions`. Passing `-DFMT_COLOR_COMPACT_SGR=1` to CMake
alone does not define the macro for the compiler.

If CMake finds another installation, use
`-Dfmt_DIR="$HOME/.local/fmtcolor/lib/cmake/fmt"` to select this package
explicitly. On multi-configuration generators, add `--config Release` to the
build command and run the executable from the generator's Release directory.

For a library target whose public headers use FmtColor, use `PUBLIC` for both
the link dependency and the compile definition so its consumers inherit them:

```cmake
target_link_libraries(my_library PUBLIC fmt::fmt)
target_compile_definitions(my_library PUBLIC
  FMT_COLOR_COMPACT_SGR=$<BOOL:${USE_COMPACT_SGR}>)
```

Use the same macro value throughout an application, including its libraries.
The macro changes inline and template definitions in the header; mixing
settings across translation units violates C++'s one-definition rule.

## Header-only use

The same two output modes also work without linking the compiled library.
With Make, define `FMT_HEADER_ONLY=1` and omit `-lfmt`. With CMake, link
`fmt::fmt-header-only` instead of `fmt::fmt`. Keep the SGR mode definition
in either case.

## Owning text, parsing, and stripping

These APIs use the same library target and SGR-mode definition as the example
above. This complete example can replace `hello.cpp`:

```cpp
#include <fmt/parse_styled_text.h>
#include <fmt/strip_ansi.h>

int main() {
  auto phrase = fmt::bold("Hello ")
      + fmt::overlay(fmt::fg(fmt::term_color::green), "world");
  fmt::println(fmt::underline(), "{}", phrase);

  auto parsed = fmt::parse_styled_text("\x1b[31mred\x1b[0m");
  fmt::println("{}", parsed);

  auto text = fmt::bold("\x1b[31mred\x1b[0m");
  auto clean = fmt::strip_ansi(text);
  fmt::println("{} / {}", clean, clean.plain_text());
}
```

The first line is underlined throughout: `Hello ` is also bold and `world` is
green. The parser produces owned styled text from standard SGR; a reset protects
all following parsed text from inheriting an enclosing style. Parsing preserves
appearance rather than the original escape bytes.

The last line prints bold `red`, then plain ` / red`. Stripping a tree removes
raw escapes while preserving its explicit styles and structure. Each leaf is
processed separately; sequences spanning leaves are retained. To discard explicit
styles, use `strip_ansi(text).plain_text()`. To scan all leaves as one string,
use `strip_ansi(text.plain_text())` instead. Neither parsing nor stripping
simulates terminal editing or provides security sanitization.

For the complete nested-style demonstration, run `make -C demo run` from the
library checkout. That demo uses header-only mode, compact SGR, and C++23;
its language setting does not change the library's C++11 minimum.
