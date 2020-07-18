# Test if compile errors are produced where necessary.

cmake_minimum_required(VERSION 3.1...3.18)

include(CheckCXXSourceCompiles)
include(CheckCXXCompilerFlag)

set(CMAKE_REQUIRED_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/../../include)
set(CMAKE_REQUIRED_FLAGS ${CXX_STANDARD_FLAG} ${PEDANTIC_COMPILE_FLAGS})

function (generate_source result fragment)
  set(${result} "
  #define FMT_HEADER_ONLY 1
  #include \"fmt/format.h\"
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

# Formatting a wide character with a narrow format string is forbidden.
expect_compile_error("fmt::format(\"{}\", L'a');")

# Formatting a wide string with a narrow format string is forbidden.
expect_compile_error("fmt::format(\"{}\", L\"foo\");")

# Formatting a narrow string with a wide format string is forbidden because
# mixing UTF-8 with UTF-16/32 can result in an invalid output.
expect_compile_error("fmt::format(L\"{}\", \"foo\");")

# Formatting a wide string with a narrow format string is forbidden.
expect_compile_error("
  struct S {
    operator std::string() const { return std::string(); }
  };
  fmt::format(\"{}\", S());
")

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
