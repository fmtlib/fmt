// Formatting library for C++ - no-locale tests
//
// Copyright (c) 2012 - present, Victor Zverovich and {fmt} contributors
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/format.h"

int main() {
  if (fmt::format("{:L}", 1234567) != "1234567") return 1;
  if (fmt::format("{:L}", 1234.5) != "1234.5") return 1;
}
