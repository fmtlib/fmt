message(STATUS "CMake version: ${CMAKE_VERSION}")

cmake_minimum_required(VERSION 2.8.12)

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

# starting with cmake 3.0 VERSION is part of the project command
set(CPPFORMAT_VERSION 2.1.0)
if (NOT CPPFORMAT_VERSION MATCHES "^([0-9]+).([0-9]+).([0-9]+)$")
  message(FATAL_ERROR "Invalid version format ${CPPFORMAT_VERSION}.")
endif ()
set(CPACK_PACKAGE_VERSION_MAJOR ${CMAKE_MATCH_1})
set(CPACK_PACKAGE_VERSION_MINOR ${CMAKE_MATCH_2})
set(CPACK_PACKAGE_VERSION_PATCH ${CMAKE_MATCH_3})

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
  "${CMAKE_CURRENT_SOURCE_DIR}/support/cmake")

include(testCxx11)

if (CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
  set(PEDANTIC_COMPILE_FLAGS -Wall -Wextra -Wshadow -pedantic)
elseif (MSVC)
  set(PEDANTIC_COMPILE_FLAGS /W4)
endif ()

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

include(CheckSymbolExists)
if (WIN32)
  check_symbol_exists(open io.h HAVE_OPEN)
else ()
  check_symbol_exists(open fcntl.h HAVE_OPEN)
endif ()

if (BIICODE)
  include(support/cmake/biicode.cmake)
  return()
endif ()

#------------------------------------------------------------------------------
# define the cppformat library, its includes and the needed defines
set(FMT_SOURCES cppformat/format.cc cppformat/format.h)
if (HAVE_OPEN)
  set(FMT_SOURCES ${FMT_SOURCES} cppformat/posix.cc cppformat/posix.h)
endif ()

add_library(cppformat ${FMT_SOURCES})

target_compile_options(cppformat PUBLIC ${CPP11_FLAG}) # starting with cmake 3.1 the CXX_STANDARD property can be used
if (FMT_PEDANTIC)
  target_compile_options(cppformat PRIVATE ${PEDANTIC_COMPILE_FLAGS})
endif ()

target_compile_definitions(cppformat INTERFACE
  FMT_USE_FILE_DESCRIPTORS=$<BOOL:${HAVE_OPEN}>)

target_include_directories(cppformat INTERFACE
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
  $<INSTALL_INTERFACE:include>)

set_target_properties(cppformat PROPERTIES
  VERSION ${CPPFORMAT_VERSION} SOVERSION ${CPACK_PACKAGE_VERSION_MAJOR})

if (BUILD_SHARED_LIBS)
  if (UNIX AND NOT APPLE)
    # Fix rpmlint warning:
    # unused-direct-shlib-dependency /usr/lib/libformat.so.1.1.0 /lib/libm.so.6.
    target_link_libraries(cppformat -Wl,--as-needed)
  endif ()
  target_compile_definitions(cppformat PRIVATE FMT_EXPORT INTERFACE FMT_SHARED)
endif ()

#------------------------------------------------------------------------------
# additionally define a header only library when cmake is new enough
if (CMAKE_VERSION VERSION_GREATER 3.1.0 OR CMAKE_VERSION VERSION_EQUAL 3.1.0)
  add_library(cppformat-header-only INTERFACE)

  target_compile_definitions(cppformat-header-only INTERFACE FMT_HEADER_ONLY=1)

  target_include_directories(cppformat-header-only INTERFACE
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>)
endif ()

if (FMT_DOC)
  add_subdirectory(doc)
endif ()

if (FMT_TEST)
  enable_testing()
  add_subdirectory(test)
endif ()



set(gitignore ${PROJECT_SOURCE_DIR}/.gitignore)
if (EXISTS ${gitignore})
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
  set(CPACK_SOURCE_PACKAGE_FILE_NAME cppformat-${CPPFORMAT_VERSION})
  set(CPACK_PACKAGE_NAME cppformat)
  set(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/README.rst)
  include(CPack)
endif ()

# Install targets.
if (FMT_INSTALL)
  include(CMakePackageConfigHelpers)
  set(config_install_dir lib/cmake/cppformat)
  set(version_config ${CMAKE_CURRENT_BINARY_DIR}/cppformat-config-version.cmake)
  set(project_config ${CMAKE_CURRENT_BINARY_DIR}/cppformat-config.cmake)
  set(targets_export_name cppformat-targets)

  set(FMT_LIB_DIR lib CACHE STRING
    "Installation directory for libraries, relative to ${CMAKE_INSTALL_PREFIX}.")

  # Generate the version, config and target files into the build directory.
  write_basic_package_version_file(
    ${version_config}
    VERSION ${CPPFORMAT_VERSION}
    COMPATIBILITY AnyNewerVersion)
  configure_package_config_file(
    support/cmake/cppformat-config.cmake.in
    ${project_config}
    INSTALL_DESTINATION ${config_install_dir})
  set(CPPFORMAT_LIBRARY_TARGETS cppformat)
  if (TARGET cppformat-header-only)
    set(CPPFORMAT_LIBRARY_TARGETS ${CPPFORMAT_LIBRARY_TARGETS} cppformat-header-only)
  endif ()
  export(TARGETS ${CPPFORMAT_LIBRARY_TARGETS} FILE ${targets_export_name}.cmake)

  # Install version, config and target files.
  install(
    FILES ${project_config} ${version_config}
    DESTINATION ${config_install_dir})
  install(EXPORT ${targets_export_name} DESTINATION ${config_install_dir})

  # Install the library and the include file.
  install(TARGETS cppformat EXPORT ${targets_export_name} DESTINATION ${FMT_LIB_DIR})
  install(FILES cppformat/format.h DESTINATION include/cppformat)
endif ()
