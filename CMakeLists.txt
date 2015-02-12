message(STATUS "CMake version: ${CMAKE_VERSION}")

if (POLICY CMP0054)
  # Only interpret `if` arguments as variables or keywords when unquoted.
  cmake_policy(SET CMP0054 NEW)
endif ()

cmake_minimum_required(VERSION 2.6)

# Set the default CMAKE_BUILD_TYPE to Release.
# This should be done before the project command since the latter can set
# CMAKE_BUILD_TYPE itself (it does so for nmake).
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
    "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
endif ()

option(FMT_EXTRA_TESTS "Enable extra tests." OFF)

project(FORMAT)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-std=c++11 HAVE_STD_CPP11_FLAG)
if (HAVE_STD_CPP11_FLAG)
  set(CPP11_FLAG -std=c++11)
else ()
  check_cxx_compiler_flag(-std=c++0x HAVE_STD_CPP0X_FLAG)
  if (HAVE_STD_CPP0X_FLAG)
    set(CPP11_FLAG -std=c++0x)
  endif ()
endif ()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

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
  include(biicode/cmake/biicode.cmake)
  return()
endif ()

add_library(format ${FMT_SOURCES})
if (CMAKE_COMPILER_IS_GNUCXX)
  set_target_properties(format PROPERTIES COMPILE_FLAGS
    "-Wall -Wextra -Wshadow -pedantic")
endif ()
if (CPP11_FLAG AND FMT_EXTRA_TESTS)
  set_target_properties(format PROPERTIES COMPILE_FLAGS ${CPP11_FLAG})
  # Test compilation with default flags.
  file(GLOB src RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cc test/*.h)
  add_library(testformat STATIC ${FMT_SOURCE_FILES} ${src})
endif ()

add_subdirectory(doc)

include_directories(. gmock)

# We compile Google Test ourselves instead of using pre-compiled libraries.
# See the Google Test FAQ "Why is it not recommended to install a
# pre-compiled copy of Google Test (for example, into /usr/local)?"
# at http://code.google.com/p/googletest/wiki/FAQ for more details.

add_library(gmock STATIC gmock/gmock-gtest-all.cc)
find_package(Threads)
target_link_libraries(gmock ${CMAKE_THREAD_LIBS_INIT})

# Check if variadic templates are working and not affected by GCC bug 39653:
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=39653
check_cxx_source_compiles("
  template <class T, class ...Types>
  struct S { typedef typename S<Types...>::type type; };
  int main() {}" FMT_VARIADIC_TEMPLATES)
if (NOT FMT_VARIADIC_TEMPLATES)
  add_definitions(-DGTEST_LANG_CXX11=0)
endif ()

# GTest doesn't detect <tuple> with clang.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  target_compile_definitions(gmock PUBLIC GTEST_USE_OWN_TR1_TUPLE=1)
endif ()

enable_testing()
add_subdirectory(test)

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
  set(CPACK_PACKAGE_VERSION_MAJOR 1)
  set(CPACK_PACKAGE_VERSION_MINOR 0)
  set(CPACK_PACKAGE_VERSION_PATCH 1)
  set(CPPFORMAT_VERSION
    ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
  set(CPACK_SOURCE_PACKAGE_FILE_NAME cppformat-${CPPFORMAT_VERSION})
  set(CPACK_RESOURCE_FILE_README ${FORMAT_SOURCE_DIR}/README.rst)
  include(CPack)
endif ()
