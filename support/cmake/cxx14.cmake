# C++14 feature support detection

include(CheckCXXSourceCompiles)
include(CheckCXXCompilerFlag)

if (NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 11)
endif()
message(STATUS "CXX_STANDARD: ${CMAKE_CXX_STANDARD}")

if (CMAKE_CXX_STANDARD EQUAL 20)
  check_cxx_compiler_flag(-std=c++20 has_std_20_flag)
  check_cxx_compiler_flag(-std=c++2a has_std_2a_flag)

  if (has_std_20_flag)
    set(CXX_STANDARD_FLAG -std=c++20)
  elseif (has_std_2a_flag)
    set(CXX_STANDARD_FLAG -std=c++2a)
  endif ()
elseif (CMAKE_CXX_STANDARD EQUAL 17)
  check_cxx_compiler_flag(-std=c++17 has_std_17_flag)
  check_cxx_compiler_flag(-std=c++1z has_std_1z_flag)

  if (has_std_17_flag)
    set(CXX_STANDARD_FLAG -std=c++17)
  elseif (has_std_1z_flag)
    set(CXX_STANDARD_FLAG -std=c++1z)
  endif ()
elseif (CMAKE_CXX_STANDARD EQUAL 14)
  check_cxx_compiler_flag(-std=c++14 has_std_14_flag)
  check_cxx_compiler_flag(-std=c++1y has_std_1y_flag)

  if (has_std_14_flag)
    set(CXX_STANDARD_FLAG -std=c++14)
  elseif (has_std_1y_flag)
    set(CXX_STANDARD_FLAG -std=c++1y)
  endif ()
elseif (CMAKE_CXX_STANDARD EQUAL 11)
  check_cxx_compiler_flag(-std=c++11 has_std_11_flag)
  check_cxx_compiler_flag(-std=c++0x has_std_0x_flag)

  if (has_std_11_flag)
    set(CXX_STANDARD_FLAG -std=c++11)
  elseif (has_std_0x_flag)
    set(CXX_STANDARD_FLAG -std=c++0x)
  endif ()
endif ()

set(CMAKE_REQUIRED_FLAGS ${CXX_STANDARD_FLAG})

# Check if variadic templates are working and not affected by GCC bug 39653:
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=39653
# Can be removed once gcc 4.4 support is dropped.
check_cxx_source_compiles("
  template <class T, class ...Types>
  struct S { typedef typename S<Types...>::type type; };
  int main() {}" SUPPORTS_VARIADIC_TEMPLATES)
if (NOT SUPPORTS_VARIADIC_TEMPLATES)
  set (SUPPORTS_VARIADIC_TEMPLATES OFF)
endif ()

# Check if user-defined literals are available
check_cxx_source_compiles("
  void operator\"\" _udl(long double);
  int main() {}"
  SUPPORTS_USER_DEFINED_LITERALS)
if (NOT SUPPORTS_USER_DEFINED_LITERALS)
  set (SUPPORTS_USER_DEFINED_LITERALS OFF)
endif ()

# Check if <variant> is available
set(CMAKE_REQUIRED_FLAGS -std=c++1z)
check_cxx_source_compiles("
  #include <variant>
  int main() {}"
  FMT_HAS_VARIANT)
if (NOT FMT_HAS_VARIANT)
  set (FMT_HAS_VARIANT OFF)
endif ()

set(CMAKE_REQUIRED_FLAGS )
