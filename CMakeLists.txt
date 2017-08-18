message(STATUS "CMake version: ${CMAKE_VERSION}")

cmake_minimum_required(VERSION 2.8.12)

if (POLICY CMP0048) # Version variables
  cmake_policy(SET CMP0048 OLD)
endif ()

if (POLICY CMP0063) # Visibility
  cmake_policy(SET CMP0063 OLD)
endif (POLICY CMP0063)

# Determine if fmt is built as a subproject (using add_subdirectory)
# or if it is the master project.
set(MASTER_PROJECT OFF)
if (CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
  set(MASTER_PROJECT ON)
endif ()

# Joins arguments and places the results in ${result_var}.
function(join result_var)
  set(result )
  foreach (arg ${ARGN})
    set(result "${result}${arg}")
  endforeach ()
  set(${result_var} "${result}" PARENT_SCOPE)
endfunction()

# Set the default CMAKE_BUILD_TYPE to Release.
# This should be done before the project command since the latter can set
# CMAKE_BUILD_TYPE itself (it does so for nmake).
if (NOT CMAKE_BUILD_TYPE)
  join(doc "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or "
           "CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING ${doc})
endif ()

option(FMT_PEDANTIC "Enable extra warnings and expensive tests." OFF)

# Options that control generation of various targets.
option(FMT_DOC "Generate the doc target." ${MASTER_PROJECT})
option(FMT_INSTALL "Generate the install target." ${MASTER_PROJECT})
option(FMT_TEST "Generate the test target." ${MASTER_PROJECT})
option(FMT_USE_CPP11 "Enable the addition of C++11 compiler flags." ON)

project(FMT)

# Starting with cmake 3.0 VERSION is part of the project command.
file(READ fmt/format.h format_h)
if (NOT format_h MATCHES "FMT_VERSION ([0-9]+)([0-9][0-9])([0-9][0-9])")
  message(FATAL_ERROR "Cannot get FMT_VERSION from format.h.")
endif ()
# Use math to skip leading zeros if any.
math(EXPR CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1})
math(EXPR CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_2})
math(EXPR CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_3})
join(FMT_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.
                 ${CPACK_PACKAGE_VERSION_PATCH})
message(STATUS "Version: ${FMT_VERSION}")

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/support/cmake")

include(cxx11)

if (CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
  set(PEDANTIC_COMPILE_FLAGS -Wall -Wextra -Wshadow -pedantic)
endif ()

if (MASTER_PROJECT AND CMAKE_GENERATOR MATCHES "Visual Studio")
  # If Microsoft SDK is installed create script run-msbuild.bat that
  # calls SetEnv.cmd to set up build environment and runs msbuild.
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

include(CheckSymbolExists)
if (WIN32)
  check_symbol_exists(open io.h HAVE_OPEN)
else ()
  check_symbol_exists(open fcntl.h HAVE_OPEN)
endif ()

add_subdirectory(fmt)

if (FMT_DOC)
  add_subdirectory(doc)
endif ()

if (FMT_TEST)
  enable_testing()
  add_subdirectory(test)
endif ()

set(gitignore ${PROJECT_SOURCE_DIR}/.gitignore)
if (MASTER_PROJECT AND EXISTS ${gitignore})
  # Get the list of ignored files from .gitignore.
  file (STRINGS ${gitignore} lines)
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
  set(CPACK_SOURCE_PACKAGE_FILE_NAME fmt-${FMT_VERSION})
  set(CPACK_PACKAGE_NAME fmt)
  set(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/README.rst)
  include(CPack)
endif ()
