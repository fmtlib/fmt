# Test if compile errors are produced where necessary.

cmake_minimum_required(VERSION 2.8)

include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/../..)

function (generate_source result fragment)
  set(${result} "
  #define FMT_HEADER_ONLY 1
  #include \"cppformat/format.h\"
  int main() {
    ${fragment}
  }
  " PARENT_SCOPE)
endfunction ()

function (expect_compile code)
  generate_source(source "${code}")
  check_cxx_source_compiles("${source}" compiles)
  if (NOT compiles)
    message(FATAL_ERROR "Compile error for: ${code}")
  endif ()
  # Unset the CMake cache variable compiles. Otherwise the compile test will
  # just use cached information next time it runs.
  unset(compiles CACHE)
endfunction ()

function (expect_compile_error code)
  generate_source(source "${code}")
  check_cxx_source_compiles("${source}" compiles)
  if (compiles)
    message(FATAL_ERROR "No compile error for: ${code}")
  endif ()
  # Unset the CMake cache variable compiles. Otherwise the compile test will
  # just use cached information next time it runs.
  unset(compiles CACHE)
endfunction ()

# check if the source file skeleton compiles
expect_compile("")

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
