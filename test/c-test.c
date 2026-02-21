// Formatting library for C++ - the C API tests
//
// Copyright (c) 2012 - present, Victor Zverovich
// All rights reserved.
//
// For the license information refer to format.h.

#include "fmt/fmt-c.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ASSERT_STR_EQ(actual, expected)                                   \
  do {                                                                    \
    if (strcmp(actual, expected) != 0) {                                  \
      fprintf(stderr,                                                     \
              "\nAssertion failed:\n  Expected: \"%s\"\n  Got: \"%s\"\n", \
              expected, actual);                                          \
      exit(1);                                                            \
    }                                                                     \
  } while (0)

#define ASSERT_INT_EQ(actual, expected)                                   \
  do {                                                                    \
    if ((actual) != (expected)) {                                         \
      fprintf(stderr, "\nAssertion failed:\n  Expected: %d\n  Got: %d\n", \
              expected, actual);                                          \
      exit(1);                                                            \
    }                                                                     \
  } while (0)

static inline int fmt_vformat_cstr(char* buf, size_t size, int result) {
  if (result >= 0 && (size_t)result < size)
    buf[result] = '\0';
  else if (size > 0)
    buf[size - 1] = '\0';
  return result;
}

#define fmt_format_cstr(buf, size, ...) \
  fmt_vformat_cstr(buf, size, fmt_format(buf, size, __VA_ARGS__))

void test_types(void) {
  char buf[100];

  fmt_format_cstr(buf, sizeof(buf), "{}", 42);
  ASSERT_STR_EQ(buf, "42");

  fmt_format_cstr(buf, sizeof(buf), "{}", 123u);
  ASSERT_STR_EQ(buf, "123");

  fmt_format_cstr(buf, sizeof(buf), "{}", (bool)true);
  ASSERT_STR_EQ(buf, "true");

  fmt_format_cstr(buf, sizeof(buf), "{}", (char)'x');
  ASSERT_STR_EQ(buf, "x");

  fmt_format_cstr(buf, sizeof(buf), "{}", 1.2f);
  ASSERT_STR_EQ(buf, "1.2");

  fmt_format_cstr(buf, sizeof(buf), "{}", 3.14159);
  ASSERT_STR_EQ(buf, "3.14159");

  fmt_format_cstr(buf, sizeof(buf), "{}", 1.2l);
  ASSERT_STR_EQ(buf, "1.2");

  fmt_format_cstr(buf, sizeof(buf), "{}", "foo");
  ASSERT_STR_EQ(buf, "foo");

  fmt_format_cstr(buf, sizeof(buf), "{}", (void*)0x12345678);
  ASSERT_STR_EQ(buf, "0x12345678");
}

void test_zero_arguments(void) {
  char buf[100];
  int ret = fmt_format_cstr(buf, sizeof(buf), "No arguments");
  ASSERT_STR_EQ(buf, "No arguments");
}

void test_buffer_size_query(void) {
  int size = fmt_format(NULL, 0, "Test string: {}", 42);
  ASSERT_INT_EQ(size, 15);
}

int main(void) {
  printf("Running C API tests\n");
  test_types();
  test_zero_arguments();
  test_buffer_size_query();
  printf("C API tests passed\n");
}
