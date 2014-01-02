# Test if compile errors are produced where necessary.

include(CheckCXXSourceCompiles)
set(CMAKE_REQUIRED_INCLUDES ${CMAKE_CURRENT_SOURCE_DIR}/..)

check_cxx_source_compiles("
  #include \"format.cc\"
  int main() { fmt::Writer() << L'a'; }
  " WRITE_WCHAR_COMPILES)
if (WRITE_WCHAR_COMPILES)
  error("No compile error for: fmt::Writer() << L'a'")
endif ()
