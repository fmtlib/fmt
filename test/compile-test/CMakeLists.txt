# Test if compile errors are produced where necessary.

cmake_minimum_required(VERSION 2.8)

include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/../..)
set(CMAKE_REQUIRED_FLAGS ${CPP11_FLAG})

function (generate_source result fragment)
  set(${result} "
  #define FMT_HEADER_ONLY 1
  #include \"fmt/posix.h\"
  int main() {
    ${fragment}
  }
  " PARENT_SCOPE)
endfunction ()

function (expect_compile code)
  generate_source(source "${code}")
  check_cxx_source_compiles("${source}" compiles)
  if (NOT compiles)
    set(error_msg "Compile error for: ${code}")
  endif ()
  # Unset the CMake cache variable compiles. Otherwise the compile test will
  # just use cached information next time it runs.
  unset(compiles CACHE)
  if (error_msg)
    message(FATAL_ERROR ${error_msg})
  endif ()
endfunction ()

function (expect_compile_error code)
  generate_source(source "${code}")
  check_cxx_source_compiles("${source}" compiles)
  if (compiles)
    set(error_msg "No compile error for: ${code}")
  endif ()
  # Unset the CMake cache variable compiles. Otherwise the compile test will
  # just use cached information next time it runs.
  unset(compiles CACHE)
  if (error_msg)
    message(FATAL_ERROR ${error_msg})
  endif ()
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

expect_compile("FMT_STATIC_ASSERT(true, \"this should never happen\");")
expect_compile_error("FMT_STATIC_ASSERT(0 > 1, \"oops\");")

# Make sure that compiler features detected in the header
# match the features detected in CMake.
if (SUPPORTS_USER_DEFINED_LITERALS)
  set(supports_udl 1)
else ()
  set(supports_udl 0)
endif ()
expect_compile("#if FMT_USE_USER_DEFINED_LITERALS != ${supports_udl}
                # error
                #endif")
