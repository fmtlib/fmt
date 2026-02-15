/* Test suite for fmt C API */
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

void test_basic_integer(void) {
  char buf[100];
  int ret = fmt_format(buf, sizeof(buf), "Number: {}", 42);
  ASSERT_STR_EQ(buf, "Number: 42");
  ASSERT_INT_EQ(ret, 10);
}

void test_multiple_integers(void) {
  char buf[100];
  fmt_format(buf, sizeof(buf), "{} + {} = {}", 1, 2, 3);
  ASSERT_STR_EQ(buf, "1 + 2 = 3");
}

void test_unsigned_integers(void) {
  char buf[100];
  unsigned int x = 4294967295U;
  fmt_format(buf, sizeof(buf), "{}", x);
  ASSERT_STR_EQ(buf, "4294967295");
}

void test_floating_point(void) {
  char buf[100];
  fmt_format(buf, sizeof(buf), "Pi = {}", 3.14159);
  ASSERT_TRUE(strncmp(buf, "Pi = 3.14159", 12) == 0);
}

void test_float_type(void) {
  char buf[100];
  float f = 1.234f;
  fmt_format(buf, sizeof(buf), "Float: {:.3f}", f);
  ASSERT_STR_EQ(buf, "Float: 1.234");
}

void test_long_double_type(void) {
  char buf[100];
  long double ld = 12345.6789L;
  fmt_format(buf, sizeof(buf), "{:.4f}", ld);
  ASSERT_STR_EQ(buf, "12345.6789");
}

void test_mixed_floating_types(void) {
  char buf[200];
  float f = 1.5f;
  double d = 2.5;
  long double ld = 3.5L;

  fmt_format(buf, sizeof(buf), "{} {} {}", f, d, ld);
  ASSERT_STR_EQ(buf, "1.5 2.5 3.5");
}

void test_strings(void) {
  char buf[100];
  fmt_format(buf, sizeof(buf), "Hello, {}!", "from fmt!");
  ASSERT_STR_EQ(buf, "Hello, from fmt!!");
}

void test_null_string(void) {
  char buf[100];
  const char* null_str = NULL;
  fmt_format(buf, sizeof(buf), "{}", null_str);
  ASSERT_STR_EQ(buf, "(null)");
}

void test_pointers(void) {
  char buf[100];
  void* ptr = (void*)0x12345678;
  fmt_format(buf, sizeof(buf), "{}", ptr);
  ASSERT_TRUE(strstr(buf, "12345678") != NULL);
}

void test_booleans(void) {
  char buf[100];
  fmt_format(buf, sizeof(buf), "{} {}", (bool)true, (bool)false);
  ASSERT_STR_EQ(buf, "true false");
}

void test_characters(void) {
  char buf[100];
  fmt_format(buf, sizeof(buf), "Char: {}", (char)'A');
  ASSERT_STR_EQ(buf, "Char: A");
}

void test_mixed_types(void) {
  char buf[100];
  fmt_format(buf, sizeof(buf), "{} {} {} {}", 42, 3.14, "text", (bool)true);
  ASSERT_TRUE(strstr(buf, "42") != NULL);
  ASSERT_TRUE(strstr(buf, "3.14") != NULL);
  ASSERT_TRUE(strstr(buf, "text") != NULL);
  ASSERT_TRUE(strstr(buf, "true") != NULL);
}
void test_zero_arguments(void) {
  char buf[100];
  // fmt_format(buf, sizeof(buf), "No arguments");
  fmt_vformat(buf, sizeof(buf), "No arguments", NULL,
              0);  // strict compiler check bypass
  ASSERT_STR_EQ(buf, "No arguments");
}

void test_buffer_size_query(void) {
  int size = fmt_format(NULL, 0, "Test string: {}", 42);
  ASSERT_INT_EQ(size, 15);
}

void test_error_invalid_format(void) {
  char buf[100];
  int ret = fmt_format(buf, sizeof(buf), "{:invalid}", 1);
  ASSERT_TRUE(ret < 0);
}

void test_long_strings(void) {
  char buf[1000];
  const char* long_str =
      "This is a very long string that contains a lot of text "
      "to test the buffer handling capabilities of the formatter";
  fmt_format(buf, sizeof(buf), "Message: {}", long_str);
  ASSERT_TRUE(strstr(buf, long_str) != NULL);
}

void test_multiple_calls(void) {
  char buf[100];

  fmt_format(buf, sizeof(buf), "{} {}", 1, 2);
  ASSERT_STR_EQ(buf, "1 2");

  fmt_format(buf, sizeof(buf), "{} {}", "hello", 3.14);
  ASSERT_TRUE(strstr(buf, "hello") != NULL);

  fmt_format(buf, sizeof(buf), "{}", (bool)true);
  ASSERT_STR_EQ(buf, "true");
}

void test_all_integer_types(void) {
  char buf[200];
  short s = 100;
  int i = 200;
  long l = 300L;
  long long ll = 400LL;
  unsigned short us = 500;
  unsigned int ui = 600;
  unsigned long ul = 700UL;
  unsigned long long ull = 800ULL;

  fmt_format(buf, sizeof(buf), "{} {} {} {} {} {} {} {}", s, i, l, ll, us, ui,
             ul, ull);
  ASSERT_TRUE(strstr(buf, "100") != NULL);
  ASSERT_TRUE(strstr(buf, "800") != NULL);
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
  test_null_string();
  test_pointers();
  test_booleans();
  test_characters();
  test_mixed_types();
  test_zero_arguments();
  test_buffer_size_query();
  test_error_invalid_format();
  test_long_strings();
  test_multiple_calls();
  test_all_integer_types();

  printf("\n=== All tests passed! ===\n");
  return 0;
}
