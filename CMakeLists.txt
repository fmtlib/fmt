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
  file(WRITE run-msbuild.bat "
    ${MSBUILD_SETUP}
    ${CMAKE_MAKE_PROGRAM} -p:FrameworkPathOverride=\"C:\\Program Files\\Reference Assemblies\\Microsoft\\Framework\\.NETFramework\\v4.0\" %*")
endif ()

option(FMT_SHARED "Build shared library instead of static one" OFF)
if (FMT_SHARED)
  set(shared SHARED)
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

add_library(format ${shared} ${FMT_SOURCES})
if (CMAKE_COMPILER_IS_GNUCXX)
  set_target_properties(format PROPERTIES COMPILE_FLAGS
    "-Wall -Wextra -pedantic")
endif ()
if (CPP11_FLAG AND FMT_EXTRA_TESTS)
  set_target_properties(format PROPERTIES COMPILE_FLAGS ${CPP11_FLAG})
  # Test compilation with default flags.
  file(GLOB src RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} test/*.cc test/*.h)
  add_library(testformat ${FMT_SOURCES} ${src})
endif ()

add_subdirectory(doc)

# We compile Google Test ourselves instead of using pre-compiled libraries.
# See the Google Test FAQ "Why is it not recommended to install a
# pre-compiled copy of Google Test (for example, into /usr/local)?"
# at http://code.google.com/p/googletest/wiki/FAQ for more details.

# GTest doesn't detect <tuple> with clang.
if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_definitions(-DGTEST_USE_OWN_TR1_TUPLE=1)
endif ()

include_directories(gtest/include)
add_subdirectory(gtest)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
  add_definitions(-DGTEST_USE_OWN_TR1_TUPLE=1)
endif ()

enable_testing()

include_directories(.)

set(TEST_MAIN_SRC
  test/test-main.cc test/gtest-extra.cc test/gtest-extra.h test/util.cc)
add_library(test-main ${TEST_MAIN_SRC})
target_link_libraries(test-main gtest format)

cxx_test(gtest-extra-test test-main)
cxx_test(format-test test-main)
cxx_test(printf-test test-main)
foreach (target format-test printf-test)
  if (CMAKE_COMPILER_IS_GNUCXX)
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS
      "-Wall -Wextra -pedantic -Wno-long-long -Wno-variadic-macros")
  endif ()
  if (CPP11_FLAG)
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS ${CPP11_FLAG})
  endif ()
endforeach ()
cxx_test(util-test test-main)

add_executable(macro-test test/macro-test.cc ${FMT_SOURCES} ${TEST_MAIN_SRC})
set_target_properties(macro-test
  PROPERTIES COMPILE_DEFINITIONS "FMT_USE_VARIADIC_TEMPLATES=0")
target_link_libraries(macro-test gtest)

if (HAVE_OPEN)
  add_executable(posix-test test/posix-test.cc ${FMT_SOURCES} ${TEST_MAIN_SRC})
  set_target_properties(posix-test
    PROPERTIES COMPILE_DEFINITIONS "FMT_INCLUDE_POSIX_TEST=1")
  target_link_libraries(posix-test gtest)
  add_test(posix-test posix-test)
endif ()

add_test(compile-test ${CMAKE_CTEST_COMMAND}
  --build-and-test
  "${CMAKE_CURRENT_SOURCE_DIR}/test"
  "${CMAKE_CURRENT_BINARY_DIR}/test"
  --build-generator ${CMAKE_GENERATOR}
  --build-makeprogram ${CMAKE_MAKE_PROGRAM})

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
  set(CPACK_PACKAGE_VERSION_MAJOR 0)
  set(CPACK_PACKAGE_VERSION_MINOR 10)
  set(CPACK_PACKAGE_VERSION_PATCH 0)
  set(CPPFORMAT_VERSION
    ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
  set(CPACK_SOURCE_PACKAGE_FILE_NAME cppformat-${CPPFORMAT_VERSION})
  set(CPACK_RESOURCE_FILE_README ${FORMAT_SOURCE_DIR}/README.rst)
  include(CPack)
endif ()

configure_file(doc/conf.py.in doc/conf.py @ONLY)
