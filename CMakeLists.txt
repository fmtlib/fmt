message(STATUS "CMake version: ${CMAKE_VERSION}")

cmake_minimum_required(VERSION 2.6)

# Set the default CMAKE_BUILD_TYPE to Release.
# This should be done before the project command since the latter can set
# CMAKE_BUILD_TYPE itself (it does so for nmake).
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
    "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
endif ()

option(FMT_PEDANTIC "Enable extra warnings and expensive tests." OFF)

project(FORMAT)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-std=c++11 HAVE_STD_CPP11_FLAG)
if (HAVE_STD_CPP11_FLAG)
  # Check if including cmath works with -std=c++11 and -O3.
  # It may not in MinGW due to bug http://ehc.ac/p/mingw/bugs/2250/.
  set(CMAKE_REQUIRED_FLAGS "-std=c++11 -O3")
  check_cxx_source_compiles("
    #include <cmath>
    int main() {}" FMT_CPP11_CMATH)
  # Check if including <unistd.h> works with -std=c++11.
  # It may not in MinGW due to bug http://sourceforge.net/p/mingw/bugs/2024/.
  check_cxx_source_compiles("
    #include <unistd.h>
    int main() {}" FMT_CPP11_UNISTD_H)
  if (FMT_CPP11_CMATH AND FMT_CPP11_UNISTD_H)
    set(CPP11_FLAG -std=c++11)
  else ()
    check_cxx_compiler_flag(-std=gnu++11 HAVE_STD_GNUPP11_FLAG)
    if (HAVE_STD_CPP11_FLAG)
      set(CPP11_FLAG -std=gnu++11)
    endif ()
  endif ()
  set(CMAKE_REQUIRED_FLAGS )
else ()
  check_cxx_compiler_flag(-std=c++0x HAVE_STD_CPP0X_FLAG)
  if (HAVE_STD_CPP0X_FLAG)
    set(CPP11_FLAG -std=c++0x)
  endif ()
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/support/cmake")

if (CMAKE_GENERATOR MATCHES "Visual Studio")
  # If Microsoft SDK is installed create script run-msbuild.bat that
  # calls SetEnv.cmd to to set up build environment and runs msbuild.
  # It is useful when building Visual Studio projects with the SDK
  # toolchain rather than Visual Studio.
  include(FindSetEnv)
  if (WINSDK_SETENV)
    set(MSBUILD_SETUP "call \"${WINSDK_SETENV}\"")
  endif ()
  # Set FrameworkPathOverride to get rid of MSB3644 warnings.
  set(netfxpath "C:\\Program Files\\Reference Assemblies\\Microsoft\\Framework\\.NETFramework\\v4.0")
  file(WRITE run-msbuild.bat "
    ${MSBUILD_SETUP}
    ${CMAKE_MAKE_PROGRAM} -p:FrameworkPathOverride=\"${netfxpath}\" %*")
endif ()

set(FMT_SOURCES format.cc format.h)

include(CheckSymbolExists)
if (WIN32)
  check_symbol_exists(open io.h HAVE_OPEN)
else ()
  check_symbol_exists(open fcntl.h HAVE_OPEN)
endif ()
if (HAVE_OPEN)
  add_definitions(-DFMT_USE_FILE_DESCRIPTORS=1)
  set(FMT_SOURCES ${FMT_SOURCES} posix.cc posix.h)
endif ()

if (CPP11_FLAG)
  set(CMAKE_REQUIRED_FLAGS ${CPP11_FLAG})
endif ()

if (BIICODE)
  include(support/cmake/biicode.cmake)
  return()
endif ()

add_library(cppformat ${FMT_SOURCES})
if (BUILD_SHARED_LIBS)
  # Fix rpmlint warning:
  # unused-direct-shlib-dependency /usr/lib/libformat.so.1.1.0 /lib/libm.so.6.
  target_link_libraries(cppformat -Wl,--as-needed)
endif ()
if (FMT_PEDANTIC AND
    (CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang")))
  set(FMT_EXTRA_COMPILE_FLAGS "-Wall -Wextra -Wshadow -pedantic")
endif ()

# If FMT_PEDANTIC is TRUE, then test compilation with both -std=c++11
# and the default flags. Otherwise use only the default flags.
# The library is distributed in the source form and users have full control
# over compile options, so the options used here only matter for testing.
if (CPP11_FLAG AND FMT_PEDANTIC)
  set(FMT_EXTRA_COMPILE_FLAGS "${FMT_EXTRA_COMPILE_FLAGS} ${CPP11_FLAG}")
  # Test compilation with default flags.
  file(GLOB src RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cc test/*.h)
  add_library(testformat STATIC ${FMT_SOURCE_FILES} ${src})
endif ()

set_target_properties(cppformat
  PROPERTIES COMPILE_FLAGS "${FMT_EXTRA_COMPILE_FLAGS}")

add_subdirectory(doc)

include_directories(. gmock)

# We compile Google Test ourselves instead of using pre-compiled libraries.
# See the Google Test FAQ "Why is it not recommended to install a
# pre-compiled copy of Google Test (for example, into /usr/local)?"
# at http://code.google.com/p/googletest/wiki/FAQ for more details.

add_library(gmock STATIC
  gmock/gmock-gtest-all.cc gmock/gmock/gmock.h
  gmock/gtest/gtest.h gmock/gtest/gtest-spi.h)
find_package(Threads)
if (Threads_FOUND)
  target_link_libraries(gmock ${CMAKE_THREAD_LIBS_INIT})
else ()
  target_compile_definitions(gmock PUBLIC GTEST_HAS_PTHREAD=0)
endif ()

# Check if variadic templates are working and not affected by GCC bug 39653:
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=39653
check_cxx_source_compiles("
  template <class T, class ...Types>
  struct S { typedef typename S<Types...>::type type; };
  int main() {}" FMT_VARIADIC_TEMPLATES)

# Check if initializer lists are supported.
check_cxx_source_compiles("
  #include <initializer_list>
  int main() {}" FMT_INITIALIZER_LIST)
  
if (NOT FMT_VARIADIC_TEMPLATES OR NOT FMT_INITIALIZER_LIST)
  add_definitions(-DGTEST_LANG_CXX11=0)
endif ()

# This is disabled at the moment because format is compiled without -std=c++11
# by default.
#check_cxx_source_compiles("
#  void f() noexcept {}
#  int main(){ f(); }" FMT_BASIC_NOEXCEPT_SUPPORT)
#if (FMT_BASIC_NOEXCEPT_SUPPORT)
#  add_definitions(-DFMT_USE_NOEXCEPT=1)
#endif ()

#check_cxx_source_compiles("
#  struct C{
#    C()=delete;
#    C(const C&)=delete;
#    C& operator=(const C&)=delete;
#  };
#  int main(){}" FMT_DELETED_FUNCTIONS)
#if (FMT_DELETED_FUNCTIONS)
#  add_definitions(-DFMT_USE_DELETED_FUNCTIONS=1)
#endif ()

#check_cxx_source_compiles("
#  static_assert(true, \"\");
#  int main(){}" FMT_STATIC_ASSERT)
#if (FMT_STATIC_ASSERT)
#  add_definitions(-DFMT_USE_STATIC_ASSERT=1)
#endif ()

# Workaround a bug in implementation of variadic templates in MSVC11.
if (MSVC)
  target_compile_definitions(gmock PUBLIC _VARIADIC_MAX=10)
endif ()

# GTest doesn't detect <tuple> with clang.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_definitions(gmock PUBLIC GTEST_USE_OWN_TR1_TUPLE=1)
endif ()

enable_testing()
add_subdirectory(test)

set(CPACK_PACKAGE_VERSION_MAJOR 1)
set(CPACK_PACKAGE_VERSION_MINOR 2)
set(CPACK_PACKAGE_VERSION_PATCH 0)
set(CPPFORMAT_VERSION
  ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
set_target_properties(cppformat PROPERTIES
  VERSION ${CPPFORMAT_VERSION} SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR})

if (EXISTS .gitignore)
  # Get the list of ignored files from .gitignore.
  file (STRINGS ".gitignore" lines)
  LIST(REMOVE_ITEM lines /doc/html)
  foreach (line ${lines})
    string(REPLACE "." "[.]" line "${line}")
    string(REPLACE "*" ".*" line "${line}")
    set(ignored_files ${ignored_files} "${line}$" "${line}/")
  endforeach ()
  set(ignored_files ${ignored_files} /.git /breathe /format-benchmark sphinx/)

  set(CPACK_SOURCE_GENERATOR ZIP)
  set(CPACK_SOURCE_IGNORE_FILES ${ignored_files})
  set(CPACK_SOURCE_PACKAGE_FILE_NAME cppformat-${CPPFORMAT_VERSION})
  set(CPACK_RESOURCE_FILE_README ${FORMAT_SOURCE_DIR}/README.rst)
  include(CPack)
endif ()

# Install targets.
set(FMT_LIB_DIR lib CACHE STRING
  "Installation directory for libraries, relative to ${CMAKE_INSTALL_PREFIX}.")
install(TARGETS cppformat DESTINATION ${FMT_LIB_DIR})
install(FILES format.h DESTINATION include/cppformat)
