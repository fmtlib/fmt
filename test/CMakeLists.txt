# Test if compile errors are produced where necessary.

include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/..)

function (expect_compile_error code)
  check_cxx_source_compiles("
  #include \"format.cc\"
  int main() {
    ${code}
  }
  " compiles)
  if (compiles)
    error("No compile error for: ${code}")
  endif ()
  unset(compiles CACHE)
endfunction ()

# Writing a wide character to a character stream Writer is forbidden.
expect_compile_error("fmt::Writer() << L'a';")
expect_compile_error("fmt::Writer() << fmt::pad(\"abc\", 5, L' ');")
