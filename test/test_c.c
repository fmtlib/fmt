/* Test suite for fmt C API */
#include "fmt/c.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define TEST(name) \
    static void test_##name(void); \
    static void run_test_##name(void) { \
        printf("Running test: %s ... ", #name); \
        test_##name(); \
        printf("PASSED\n"); \
    } \
    static void test_##name(void)

#define ASSERT_STR_EQ(actual, expected) \
    do { \
        if (strcmp(actual, expected) != 0) { \
            fprintf(stderr, "\nAssertion failed:\n  Expected: \"%s\"\n  Got: \"%s\"\n", expected, actual); \
            exit(1); \
        } \
    } while(0)

#define ASSERT_INT_EQ(actual, expected) \
    do { \
        if ((actual) != (expected)) { \
            fprintf(stderr, "\nAssertion failed:\n  Expected: %d\n  Got: %d\n", expected, actual); \
            exit(1); \
        } \
    } while(0)

#define ASSERT_TRUE(cond) \
    do { \
        if (!(cond)) { \
            fprintf(stderr, "\nAssertion failed: %s\n", #cond); \
            exit(1); \
        } \
    } while(0)

TEST(basic_integer) {
    char buf[100];
    int ret = fmt_snprintf(buf, sizeof(buf), "Number: {}", 42);
    ASSERT_STR_EQ(buf, "Number: 42");
    ASSERT_INT_EQ(ret, 10);
}

TEST(multiple_integers) {
    char buf[100];
    fmt_snprintf(buf, sizeof(buf), "{} + {} = {}", 1, 2, 3);
    ASSERT_STR_EQ(buf, "1 + 2 = 3");
}

TEST(unsigned_integers) {
    char buf[100];
    unsigned int x = 4294967295U;
    fmt_snprintf(buf, sizeof(buf), "{}", x);
    ASSERT_STR_EQ(buf, "4294967295");
}

TEST(floating_point) {
    char buf[100];
    fmt_snprintf(buf, sizeof(buf), "Pi = {}", 3.14159);
    ASSERT_TRUE(strncmp(buf, "Pi = 3.14159", 12) == 0);
}

TEST(float_type) {
    char buf[100];
    float f = 1.234f;
    fmt_snprintf(buf, sizeof(buf), "Float: {:.3f}", f);
    ASSERT_STR_EQ(buf, "Float: 1.234");
}

TEST(long_double_type) {
    char buf[100];
    long double ld = 12345.6789L;
    fmt_snprintf(buf, sizeof(buf), "{:.4f}", ld);
    ASSERT_STR_EQ(buf, "12345.6789");
}

TEST(mixed_floating_types) {
    char buf[200];
    float f = 1.5f;
    double d = 2.5;
    long double ld = 3.5L;

    fmt_snprintf(buf, sizeof(buf), "{} {} {}", f, d, ld);
    ASSERT_STR_EQ(buf, "1.5 2.5 3.5");
}

TEST(strings) {
    char buf[100];
    fmt_snprintf(buf, sizeof(buf), "Hello, {}!", "from fmt!");
    ASSERT_STR_EQ(buf, "Hello, from fmt!!");
}

TEST(null_string) {
    char buf[100];
    const char* null_str = NULL;
    fmt_snprintf(buf, sizeof(buf), "{}", null_str);
    ASSERT_STR_EQ(buf, "(null)");
}

TEST(pointers) {
    char buf[100];
    void* ptr = (void*)0x12345678;
    fmt_snprintf(buf, sizeof(buf), "{}", ptr);
    ASSERT_TRUE(strstr(buf, "12345678") != NULL);
}

TEST(booleans) {
    char buf[100];
    fmt_snprintf(buf, sizeof(buf), "{} {}", (bool)true, (bool)false);
    ASSERT_STR_EQ(buf, "true false");
}

TEST(characters) {
    char buf[100];
    fmt_snprintf(buf, sizeof(buf), "Char: {}", (char)'A');
    ASSERT_STR_EQ(buf, "Char: A");
}

TEST(mixed_types) {
    char buf[100];
    fmt_snprintf(buf, sizeof(buf), "{} {} {} {}", 42, 3.14, "text", (bool)true);
    ASSERT_TRUE(strstr(buf, "42") != NULL);
    ASSERT_TRUE(strstr(buf, "3.14") != NULL);
    ASSERT_TRUE(strstr(buf, "text") != NULL);
    ASSERT_TRUE(strstr(buf, "true") != NULL);
}

TEST(format_zero_padding) {
    char buf[100];
    fmt_snprintf(buf, sizeof(buf), "{:05d}", 42);
    ASSERT_STR_EQ(buf, "00042");
}

TEST(format_precision) {
    char buf[100];
    fmt_snprintf(buf, sizeof(buf), "{:.2f}", 3.14159);
    ASSERT_STR_EQ(buf, "3.14");
}

TEST(format_hex) {
    char buf[100];
    fmt_snprintf(buf, sizeof(buf), "{:x}", 255);
    ASSERT_STR_EQ(buf, "ff");
}

TEST(format_hex_upper) {
    char buf[100];
    fmt_snprintf(buf, sizeof(buf), "{:X}", 255);
    ASSERT_STR_EQ(buf, "FF");
}

TEST(positional_arguments) {
    char buf[100];
    fmt_snprintf(buf, sizeof(buf), "{1} {0}", "from fmt!", "Hello");
    ASSERT_STR_EQ(buf, "Hello from fmt!");
}

TEST(zero_arguments) {
    char buf[100];
    // fmt_snprintf(buf, sizeof(buf), "No arguments");
    fmt_c_format(buf, sizeof(buf), "No arguments", NULL, 0); // strict compiler check bypass - either turn on cextension in cmake or this
    ASSERT_STR_EQ(buf, "No arguments");
}

TEST(buffer_size_query) {
    int size = fmt_snprintf(NULL, 0, "Test string: {}", 42);
    ASSERT_INT_EQ(size, 15);
}

TEST(buffer_overflow) {
    char buf[10];
    int ret = fmt_snprintf(buf, sizeof(buf), "Very long string: {}", 12345);
    ASSERT_INT_EQ(buf[9], '\0');
    ASSERT_TRUE(ret > 9);
}

static int custom_point_formatter(char* buf, size_t cap, const void* data) {
    const int* point = (const int*)data;
    if (!buf || cap == 0) {
        return snprintf(NULL, 0, "Point(%d, %d)", point[0], point[1]);
    }
    return snprintf(buf, cap, "Point(%d, %d)", point[0], point[1]);
}

TEST(custom_formatter) {
    char buf[100];
    int point[2] = {10, 20};
    FmtArg args[] = {
        FMT_MAKE_CUSTOM(point, custom_point_formatter)
    };
    fmt_c_format(buf, sizeof(buf), "Location: {}", args, 1);
    ASSERT_STR_EQ(buf, "Location: Point(10, 20)");
}

TEST(custom_formatter_null_function) {
    char buf[100];
    int data = 42;
    FmtArg args[] = {
        fmt_from_custom(&data, NULL)
    };
    int ret = fmt_c_format(buf, sizeof(buf), "Value: {}", args, 1);
    ASSERT_TRUE(ret < 0);
    const char* err = fmt_c_get_error();
    ASSERT_TRUE(strstr(err, "NULL") != NULL);
}

TEST(custom_formatter_null_data) {
    char buf[100];
    FmtArg args[] = {
        fmt_from_custom(NULL, custom_point_formatter)
    };
    int ret = fmt_c_format(buf, sizeof(buf), "Value: {}", args, 1);
    ASSERT_TRUE(ret < 0);
    const char* err = fmt_c_get_error();
    ASSERT_TRUE(strstr(err, "NULL") != NULL);
}

static int failing_formatter(char* buf, size_t cap, const void* data) {
    (void)buf; (void)cap; (void)data;
    return -1;
}

TEST(custom_formatter_error_return) {
    char buf[100];
    int data = 42;
    FmtArg args[] = {
        FMT_MAKE_CUSTOM(&data, failing_formatter)
    };
    int ret = fmt_c_format(buf, sizeof(buf), "Value: {}", args, 1);
    ASSERT_TRUE(ret < 0);
    const char* err = fmt_c_get_error();
    ASSERT_TRUE(strstr(err, "error") != NULL);
}

TEST(error_null_format) {
    char buf[100];
    int ret = fmt_c_format(buf, sizeof(buf), NULL, NULL, 0);
    ASSERT_TRUE(ret < 0);
    const char* err = fmt_c_get_error();
    ASSERT_TRUE(strlen(err) > 0);
}

TEST(error_too_many_args) {
    char buf[100];
    FmtArg args[20];  // More than MAX_PACKED_ARGS (16)
    for (int i = 0; i < 20; i++) {
        args[i] = fmt_from_int(i);
    }
    int ret = fmt_c_format(buf, sizeof(buf), "{}", args, 20);
    ASSERT_TRUE(ret < 0);
    const char* err = fmt_c_get_error();
    ASSERT_TRUE(strstr(err, "maximum") != NULL || strstr(err, "many") != NULL);
}

// NEW: Test NULL args with non-zero count
TEST(error_null_args_nonzero_count) {
    char buf[100];
    int ret = fmt_c_format(buf, sizeof(buf), "{}", NULL, 1);
    ASSERT_TRUE(ret < 0);
    const char* err = fmt_c_get_error();
    ASSERT_TRUE(strlen(err) > 0);
}

TEST(error_invalid_format) {
    char buf[100];
    int ret = fmt_snprintf(buf, sizeof(buf), "{:invalid}", 1);
    ASSERT_TRUE(ret < 0);
    const char* err = fmt_c_get_error();
    ASSERT_TRUE(strlen(err) > 0);
}

TEST(printf_to_stdout) {
    printf("\n  Output from fmt_printf: ");
    fmt_printf("Test {} {} {}", 1, 2.5, "string");
    printf("\n");
}
TEST(print_null_file) {
    FmtArg args[] = { fmt_from_int(42) };
    fmt_c_print(NULL, "{}", args, 1);
    const char* err = fmt_c_get_error();
    ASSERT_TRUE(strstr(err, "NULL") != NULL);
}

TEST(print_null_format) {
    fmt_c_print(stdout, NULL, NULL, 0);
    const char* err = fmt_c_get_error();
    ASSERT_TRUE(strstr(err, "NULL") != NULL);
}

TEST(long_strings) {
    char buf[1000];
    const char* long_str = "This is a very long string that contains a lot of text "
                          "to test the buffer handling capabilities of the formatter";
    fmt_snprintf(buf, sizeof(buf), "Message: {}", long_str);
    ASSERT_TRUE(strstr(buf, long_str) != NULL);
}

TEST(multiple_calls) {
    char buf[100];

    fmt_snprintf(buf, sizeof(buf), "{} {}", 1, 2);
    ASSERT_STR_EQ(buf, "1 2");

    fmt_snprintf(buf, sizeof(buf), "{} {}", "hello", 3.14);
    ASSERT_TRUE(strstr(buf, "hello") != NULL);

    fmt_snprintf(buf, sizeof(buf), "{}", (bool)true);
    ASSERT_STR_EQ(buf, "true");
}

TEST(escaped_braces) {
    char buf[100];
    fmt_snprintf(buf, sizeof(buf), "{{}} {}", 42);
    ASSERT_STR_EQ(buf, "{} 42");
}

TEST(all_integer_types) {
    char buf[200];
    short s = 100;
    int i = 200;
    long l = 300L;
    long long ll = 400LL;
    unsigned short us = 500;
    unsigned int ui = 600;
    unsigned long ul = 700UL;
    unsigned long long ull = 800ULL;

    fmt_snprintf(buf, sizeof(buf), "{} {} {} {} {} {} {} {}", s, i, l, ll, us, ui, ul, ull);
    ASSERT_TRUE(strstr(buf, "100") != NULL);
    ASSERT_TRUE(strstr(buf, "800") != NULL);
}

TEST(version_check) {
    int version = fmt_c_get_version();
    ASSERT_INT_EQ(version, FMT_C_ABI_VERSION);
}

TEST(alignment) {
    char buf[100];
    fmt_snprintf(buf, sizeof(buf), "{:>10}", 42);
    ASSERT_STR_EQ(buf, "        42");
}

TEST(center_alignment) {
    char buf[100];
    fmt_snprintf(buf, sizeof(buf), "{:^10}", "Hi");
    ASSERT_STR_EQ(buf, "    Hi    ");
}

TEST(struct_size_and_alignment) {
    // Verify that FmtArg has expected size with explicit padding
    // On 64-bit: 4 (type) + 4 (padding) + 16 (union) + 8 (ptr) = 32 bytes .... 24 bytes in MSVC becuz of it's internal optimization
    // This may vary on 32-bit or with different compilers
    printf("\n  FmtArg size: %zu bytes (alignment: %zu)\n",
           sizeof(FmtArg), _Alignof(FmtArg));

    FmtArg arg = fmt_from_int(42);
    ASSERT_INT_EQ(arg._padding, 0);
}

int main(void) {
    printf("=== Running fmt C API Tests ===\n\n");

    run_test_basic_integer();
    run_test_multiple_integers();
    run_test_unsigned_integers();
    run_test_floating_point();
    run_test_float_type();
    run_test_long_double_type();
    run_test_mixed_floating_types();
    run_test_strings();
    run_test_null_string();
    run_test_pointers();
    run_test_booleans();
    run_test_characters();
    run_test_mixed_types();
    run_test_format_zero_padding();
    run_test_format_precision();
    run_test_format_hex();
    run_test_format_hex_upper();
    run_test_positional_arguments();
    run_test_zero_arguments();
    run_test_buffer_size_query();
    run_test_buffer_overflow();
    run_test_custom_formatter();
    run_test_custom_formatter_null_function();
    run_test_custom_formatter_null_data();
    run_test_custom_formatter_error_return();
    run_test_error_null_args_nonzero_count();
    run_test_error_null_format();
    run_test_error_too_many_args();
    run_test_error_invalid_format();
    run_test_printf_to_stdout();
    run_test_print_null_file();
    run_test_print_null_format();
    run_test_long_strings();
    run_test_multiple_calls();
    run_test_escaped_braces();
    run_test_all_integer_types();
    run_test_version_check();
    run_test_alignment();
    run_test_center_alignment();
    run_test_struct_size_and_alignment();

    printf("\n=== All tests passed! ===\n");
    return 0;
}