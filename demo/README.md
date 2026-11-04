# FmtColor demo

From this directory, run `make run` using GNU make and a C++23-capable compiler.
From the repository root, run `make -C demo run`.

The Makefile uses `FMT_DIR=..` to read this checkout's headers. It builds in
header-only mode with compact SGR enabled; no CMake or prebuilt library is needed.
Override the path or compiler with `make run FMT_DIR=/path/to/fmt CXX=g++`.
Run `make clean` before changing the fmt path, compiler, or build flags.

`FmtColorDemo2.cpp` exercises nested colors/emphasis and absolute boundaries.
The second line has strikethrough and a gray background except inside the
absolute underline-only span. Output requires an ANSI-capable terminal.
