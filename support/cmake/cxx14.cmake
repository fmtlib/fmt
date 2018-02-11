# C++14 feature support detection

if (NOT FMT_USE_CPP14)
  return()
endif ()

include(CheckCXXCompilerFlag)

if (FMT_USE_CPP14)
  check_cxx_compiler_flag(-std=c++14 HAVE_STD_CPP14_FLAG)
  if (HAVE_STD_CPP14_FLAG)
    # Check if including cmath works with -std=c++14 and -O3.
    # It may not in MinGW due to bug http://ehc.ac/p/mingw/bugs/2250/.
    set(CMAKE_REQUIRED_FLAGS "-std=c++14 -O3")
    check_cxx_source_compiles("
      #include <cmath>
      int main() {}" FMT_CPP14_CMATH)
    # Check if including <unistd.h> works with -std=c++14.
    # It may not in MinGW due to bug http://sourceforge.net/p/mingw/bugs/2024/.
    check_cxx_source_compiles("
      #include <unistd.h>
      int main() {}" FMT_CPP14_UNISTD_H)
    # Check if snprintf works with -std=c++14. It may not in MinGW.
    check_cxx_source_compiles("
      #include <stdio.h>
      int main() {
        char buffer[10];
        snprintf(buffer, 10, \"foo\");
      }" FMT_CPP14_SNPRINTF)
    if (FMT_CPP14_CMATH AND FMT_CPP14_UNISTD_H AND FMT_CPP14_SNPRINTF)
      set(CPP14_FLAG -std=c++14)
    else ()
      check_cxx_compiler_flag(-std=gnu++14 HAVE_STD_GNUPP14_FLAG)
      if (HAVE_STD_CPP14_FLAG)
        set(CPP14_FLAG -std=gnu++14)
      endif ()
    endif ()
    set(CMAKE_REQUIRED_FLAGS )
  else ()
    check_cxx_compiler_flag(-std=c++1y HAVE_STD_CPP1Y_FLAG)
    if (HAVE_STD_CPP1Y_FLAG)
      set(CPP14_FLAG -std=c++1y)
    else ()
      # Fallback on c++11 if c++14 is not available.
      check_cxx_compiler_flag(-std=c++11 HAVE_STD_CPP11_FLAG)
      if (HAVE_STD_CPP11_FLAG)
        set(CPP14_FLAG -std=c++11)
      else ()
        check_cxx_compiler_flag(-std=c++0x HAVE_STD_CPP0X_FLAG)
        if (HAVE_STD_CPP0X_FLAG)
          set(CPP14_FLAG -std=c++0x)
        endif ()
      endif ()
    endif ()
  endif ()
endif ()

if (CMAKE_CXX_STANDARD)
  # Don't use -std compiler flag if CMAKE_CXX_STANDARD is specified.
  set(CPP14_FLAG )
endif ()

message(STATUS "CPP14_FLAG: ${CPP14_FLAG}")
set(CMAKE_REQUIRED_FLAGS ${CPP14_FLAG})

# Check if variadic templates are working and not affected by GCC bug 39653:
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=39653
check_cxx_source_compiles("
  template <class T, class ...Types>
  struct S { typedef typename S<Types...>::type type; };
  int main() {}" SUPPORTS_VARIADIC_TEMPLATES)

# Check if initializer lists are supported.
check_cxx_source_compiles("
  #include <initializer_list>
  int main() {}" SUPPORTS_INITIALIZER_LIST)

# Check if enum bases are available
check_cxx_source_compiles("
  enum C : char {A};
  int main() {}"
  SUPPORTS_ENUM_BASE)

# Check if type traits are available
check_cxx_source_compiles("
  #include <type_traits>
  class C { void operator=(const C&); };
  int main() { static_assert(!std::is_copy_assignable<C>::value, \"\"); }"
  SUPPORTS_TYPE_TRAITS)

# Check if user-defined literals are available
check_cxx_source_compiles("
  void operator\"\" _udl(long double);
  int main() {}"
  SUPPORTS_USER_DEFINED_LITERALS)

set(CMAKE_REQUIRED_FLAGS )
