# Test if compile errors are produced where necessary.

cmake_minimum_required(VERSION 2.8)

include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/../..)

function (expect_compile_error code)
  check_cxx_source_compiles("
  #include \"format.cc\"
  #include \"posix.h\"
  int main() {
    ${code}
  }
  " compiles)
  set (does_compile ${compiles})
  # Unset the CMake cache variable compiles. Otherwise the compile test will
  # just use cached information next time it runs.
  unset(compiles CACHE)
  if (does_compile)
    message(FATAL_ERROR "No compile error for: ${code}")
  endif ()
endfunction ()

# MakeArg doesn't accept [const] volatile char *.
expect_compile_error("volatile char s[] = \"test\"; (fmt::internal::MakeArg<char>)(s);")
expect_compile_error("const volatile char s[] = \"test\"; (fmt::internal::MakeArg<char>)(s);")

# MakeArg<char> doesn't accept wchar_t.
expect_compile_error("fmt::internal::MakeValue<char>(L'a');")
expect_compile_error("fmt::internal::MakeValue<char>(L\"test\");")

# Writing a wide character to a character stream Writer is forbidden.
expect_compile_error("fmt::MemoryWriter() << L'a';")
expect_compile_error("fmt::MemoryWriter() << fmt::pad(\"abc\", 5, L' ');")
expect_compile_error("fmt::MemoryWriter() << fmt::pad(42, 5, L' ');")

# Formatting a wide character with a narrow format string is forbidden.
expect_compile_error("fmt::format(\"{}\", L'a';")

expect_compile_error("FMT_STATIC_ASSERT(0 > 1, \"oops\");")
