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

# Options that control generation of various targets.
option(FMT_DOC "Generate the doc target." ON)
option(FMT_INSTALL "Generate the install target." ON)
option(FMT_TEST "Generate the test target." ON)

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
  set(FMT_TEST_DEFAULT_FLAGS TRUE)
endif ()

set_target_properties(cppformat
  PROPERTIES COMPILE_FLAGS "${FMT_EXTRA_COMPILE_FLAGS}")

if (FMT_DOC)
  add_subdirectory(doc)
endif ()

if (FMT_TEST)
  enable_testing()
  add_subdirectory(test)
endif ()

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
  set(ignored_files ${ignored_files}
    /.git /breathe /format-benchmark sphinx/ .buildinfo .doctrees)

  set(CPACK_SOURCE_GENERATOR ZIP)
  set(CPACK_SOURCE_IGNORE_FILES ${ignored_files})
  set(CPACK_SOURCE_PACKAGE_FILE_NAME cppformat-${CPPFORMAT_VERSION})
  set(CPACK_RESOURCE_FILE_README ${FORMAT_SOURCE_DIR}/README.rst)
  include(CPack)
endif ()

# Install targets.
if (FMT_INSTALL)
  set(FMT_LIB_DIR lib CACHE STRING
    "Installation directory for libraries, relative to ${CMAKE_INSTALL_PREFIX}.")
  install(TARGETS cppformat DESTINATION ${FMT_LIB_DIR})
  install(FILES format.h DESTINATION include/cppformat)
endif ()
