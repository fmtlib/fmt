/* Test suite for fmt C API */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fmt/fmt-c.h"

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

#define ASSERT_TRUE(cond)                                 \
  do {                                                    \
    if (!(cond)) {                                        \
      fprintf(stderr, "\nAssertion failed: %s\n", #cond); \
      exit(1);                                            \
    }                                                     \
  } while (0)

// Helper to manually null-terminate buffer after formatting
void terminate(char* buf, int size, size_t capacity) {
  if (size >= 0 && (size_t)size < capacity) {
    buf[size] = '\0';
  } else if (capacity > 0) {
    buf[capacity - 1] = '\0';
  }
}

void test_basic_integer(void) {
  char buf[100];
  int ret = fmt_format(buf, sizeof(buf), "Number: {}", 42);
  terminate(buf, ret, sizeof(buf));
  ASSERT_STR_EQ(buf, "Number: 42");
  ASSERT_INT_EQ(ret, 10);
}

void test_multiple_integers(void) {
  char buf[100];
  int ret = fmt_format(buf, sizeof(buf), "{} + {} = {}", 1, 2, 3);
  terminate(buf, ret, sizeof(buf));
  ASSERT_STR_EQ(buf, "1 + 2 = 3");
}

void test_unsigned_integers(void) {
  char buf[100];
  unsigned int x = 4294967295U;
  int ret = fmt_format(buf, sizeof(buf), "{}", x);
  terminate(buf, ret, sizeof(buf));
  ASSERT_STR_EQ(buf, "4294967295");
}

void test_floating_point(void) {
  char buf[100];
  int ret = fmt_format(buf, sizeof(buf), "Pi = {}", 3.14159);
  terminate(buf, ret, sizeof(buf));
  ASSERT_TRUE(strncmp(buf, "Pi = 3.14159", 12) == 0);
}

void test_float_type(void) {
  char buf[100];
  float f = 1.234f;
  int ret = fmt_format(buf, sizeof(buf), "Float: {:.3f}", f);
  terminate(buf, ret, sizeof(buf));
  ASSERT_STR_EQ(buf, "Float: 1.234");
}

void test_long_double_type(void) {
  char buf[100];
  long double ld = 12345.6789L;
  int ret = fmt_format(buf, sizeof(buf), "{:.4f}", ld);
  terminate(buf, ret, sizeof(buf));
  ASSERT_STR_EQ(buf, "12345.6789");
}

void test_mixed_floating_types(void) {
  char buf[200];
  float f = 1.5f;
  double d = 2.5;
  long double ld = 3.5L;

  int ret = fmt_format(buf, sizeof(buf), "{} {} {}", f, d, ld);
  terminate(buf, ret, sizeof(buf));
  ASSERT_STR_EQ(buf, "1.5 2.5 3.5");
}

void test_strings(void) {
  char buf[100];
  int ret = fmt_format(buf, sizeof(buf), "Hello, {}!", "from fmt!");
  terminate(buf, ret, sizeof(buf));
  ASSERT_STR_EQ(buf, "Hello, from fmt!!");
}

void test_pointers(void) {
  char buf[100];
  void* ptr = (void*)0x12345678;
  int ret = fmt_format(buf, sizeof(buf), "{}", ptr);
  terminate(buf, ret, sizeof(buf));
  ASSERT_TRUE(strstr(buf, "12345678") != NULL);
}

void test_booleans(void) {
  char buf[100];
  int ret = fmt_format(buf, sizeof(buf), "{} {}", (bool)true, (bool)false);
  terminate(buf, ret, sizeof(buf));
  ASSERT_STR_EQ(buf, "true false");
}

void test_characters(void) {
  char buf[100];
  int ret = fmt_format(buf, sizeof(buf), "Char: {}", (char)'A');
  terminate(buf, ret, sizeof(buf));
  ASSERT_STR_EQ(buf, "Char: A");
}

void test_mixed_types(void) {
  char buf[100];
  int ret =
      fmt_format(buf, sizeof(buf), "{} {} {} {}", 42, 3.14, "text", (bool)true);
  terminate(buf, ret, sizeof(buf));
  ASSERT_TRUE(strstr(buf, "42") != NULL);
  ASSERT_TRUE(strstr(buf, "3.14") != NULL);
  ASSERT_TRUE(strstr(buf, "text") != NULL);
  ASSERT_TRUE(strstr(buf, "true") != NULL);
}

void test_zero_arguments(void) {
  char buf[100];
  int ret = fmt_vformat(buf, sizeof(buf), "No arguments", NULL, 0);
  terminate(buf, ret, sizeof(buf));
  ASSERT_STR_EQ(buf, "No arguments");
}

void test_buffer_size_query(void) {
  int size = fmt_format(NULL, 0, "Test string: {}", 42);
  ASSERT_INT_EQ(size, 15);
}

void test_long_strings(void) {
  char buf[1000];
  const char* long_str =
      "This is a very long string that contains a lot of text "
      "to test the buffer handling capabilities of the formatter";
  int ret = fmt_format(buf, sizeof(buf), "Message: {}", long_str);
  terminate(buf, ret, sizeof(buf));
  ASSERT_TRUE(strstr(buf, long_str) != NULL);
}

void test_multiple_calls(void) {
  char buf[100];

  int ret = fmt_format(buf, sizeof(buf), "{} {}", 1, 2);
  terminate(buf, ret, sizeof(buf));
  ASSERT_STR_EQ(buf, "1 2");

  ret = fmt_format(buf, sizeof(buf), "{} {}", "hello", 3.14);
  terminate(buf, ret, sizeof(buf));
  ASSERT_TRUE(strstr(buf, "hello") != NULL);

  ret = fmt_format(buf, sizeof(buf), "{}", (bool)true);
  terminate(buf, ret, sizeof(buf));
  ASSERT_STR_EQ(buf, "true");
}

int main(void) {
  printf("=== Running fmt C API Tests ===\n\n");

  test_basic_integer();
  test_multiple_integers();
  test_unsigned_integers();
  test_floating_point();
  test_float_type();
  test_long_double_type();
  test_mixed_floating_types();
  test_strings();
  test_pointers();
  test_booleans();
  test_characters();
  test_mixed_types();
  test_zero_arguments();
  test_buffer_size_query();
  test_long_strings();
  test_multiple_calls();

  printf("\n=== All tests passed! ===\n");
  return 0;
}
