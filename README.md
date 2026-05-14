# {fmt}

[![Build status](https://github.com/fmtlib/fmt/actions/workflows/build.yml/badge.svg)](https://github.com/fmtlib/fmt/actions/workflows/build.yml)
[![Documentation](https://img.shields.io/badge/docs-latest-blue.svg)](https://fmt.dev/)

{fmt} is an open-source formatting library providing a fast and safe alternative to C stdio and C++ iostreams.

## Using {fmt} as a C++20 Module

To use {fmt} as a C++20 module, ensure your project is configured with C++20 or later and that your compiler supports modules (e.g., GCC 14+, Clang 16+, or MSVC 19.34+).

### CMake Configuration
The recommended way to use {fmt} is via the provided CMake target. Do not wrap the library in your own module; use the one provided by the build system.

1. Enable module scanning in your `CMakeLists.txt`: