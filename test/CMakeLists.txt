# Test if compile errors are produced where necessary.

cmake_minimum_required(VERSION 2.8)

include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/..)

function (expect_compile_error code)
  check_cxx_source_compiles("
  #include \"format.cc\"
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

# Array is noncopyable.
expect_compile_error("fmt::internal::Array<char, 5> a, b(a);")
expect_compile_error("fmt::internal::Array<char, 5> a, b; b = a;")

# Writer is noncopyable.
expect_compile_error("fmt::Writer a, b(a);")
expect_compile_error("fmt::Writer a, b; b = a;")

# Writing a wide character to a character stream Writer is forbidden.
expect_compile_error("fmt::Writer() << L'a';")
expect_compile_error("fmt::Writer() << fmt::pad(\"abc\", 5, L' ');")
expect_compile_error("fmt::Writer() << fmt::pad(42, 5, L' ');")
