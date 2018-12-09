#------------------------------------------------------------------------------
# Build the google test library

# We compile Google Test ourselves instead of using pre-compiled libraries.
# See the Google Test FAQ "Why is it not recommended to install a
# pre-compiled copy of Google Test (for example, into /usr/local)?"
# at http://code.google.com/p/googletest/wiki/FAQ for more details.
add_library(gmock STATIC
  gmock-gtest-all.cc gmock/gmock.h gtest/gtest.h gtest/gtest-spi.h)
target_compile_definitions(gmock PUBLIC GTEST_HAS_STD_WSTRING=1)
target_include_directories(gmock SYSTEM PUBLIC . gmock gtest)

find_package(Threads)
if (Threads_FOUND)
  target_link_libraries(gmock ${CMAKE_THREAD_LIBS_INIT})
else ()
  target_compile_definitions(gmock PUBLIC GTEST_HAS_PTHREAD=0)
endif ()

if (NOT SUPPORTS_VARIADIC_TEMPLATES OR NOT SUPPORTS_INITIALIZER_LIST)
  target_compile_definitions(gmock PUBLIC GTEST_LANG_CXX11=0)
endif ()

# Workaround a bug in implementation of variadic templates in MSVC11.
if (MSVC)
  target_compile_definitions(gmock PUBLIC _VARIADIC_MAX=10)
endif ()

# GTest doesn't detect <tuple> with clang.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_definitions(gmock PUBLIC GTEST_USE_OWN_TR1_TUPLE=1)
endif ()

# Silence MSVC tr1 deprecation warning in gmock.
target_compile_definitions(gmock
  PUBLIC _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING=0)

#------------------------------------------------------------------------------
# Build the actual library tests

set(TEST_MAIN_SRC test-main.cc gtest-extra.cc gtest-extra.h util.cc)
add_library(test-main STATIC ${TEST_MAIN_SRC})
target_compile_definitions(test-main PUBLIC
  FMT_USE_FILE_DESCRIPTORS=$<BOOL:${HAVE_OPEN}>)
target_include_directories(test-main SYSTEM PUBLIC gtest gmock)
target_link_libraries(test-main gmock fmt)

include(CheckCXXCompilerFlag)

# Workaround GTest bug https://github.com/google/googletest/issues/705.
check_cxx_compiler_flag(
  -fno-delete-null-pointer-checks HAVE_FNO_DELETE_NULL_POINTER_CHECKS)
if (HAVE_FNO_DELETE_NULL_POINTER_CHECKS)
  target_compile_options(test-main PUBLIC -fno-delete-null-pointer-checks)
endif ()

# Use less strict pedantic flags for the tests because GMock doesn't compile
# cleanly with -pedantic and -std=c++98.
if (CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    #set(PEDANTIC_COMPILE_FLAGS -Wall -Wextra -Wno-long-long -Wno-variadic-macros)
endif ()

function(add_fmt_executable name)
  add_executable(${name} ${ARGN})
  if (MINGW)
    target_link_libraries(${name} -static-libgcc -static-libstdc++)
  endif ()
endfunction()

# Adds a test.
# Usage: add_fmt_test(name srcs...)
function(add_fmt_test name)
  add_fmt_executable(${name} ${name}.cc ${ARGN})
  target_link_libraries(${name} test-main)

  # Define if certain C++ features can be used.
  target_compile_definitions(${name} PRIVATE
    FMT_USE_TYPE_TRAITS=$<BOOL:${SUPPORTS_TYPE_TRAITS}>
    FMT_USE_ENUM_BASE=$<BOOL:${SUPPORTS_ENUM_BASE}>)
  if (FMT_PEDANTIC)
    target_compile_options(${name} PRIVATE ${PEDANTIC_COMPILE_FLAGS})
  endif ()
  target_include_directories(${name} SYSTEM PUBLIC gtest gmock)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

add_fmt_test(assert-test)
add_fmt_test(chrono-test)
add_fmt_test(core-test)
add_fmt_test(gtest-extra-test)
add_fmt_test(format-test mock-allocator.h)
if (NOT (MSVC AND BUILD_SHARED_LIBS))
  add_fmt_test(format-impl-test)
endif ()
add_fmt_test(locale-test)
add_fmt_test(ostream-test)
add_fmt_test(printf-test)
add_fmt_test(time-test)
add_fmt_test(custom-formatter-test)
add_fmt_test(ranges-test)

if (HAVE_OPEN)
  add_fmt_executable(posix-mock-test
    posix-mock-test.cc ../src/format.cc ${TEST_MAIN_SRC})
  target_include_directories(
    posix-mock-test PRIVATE ${PROJECT_SOURCE_DIR}/include)
  target_compile_definitions(posix-mock-test PRIVATE FMT_USE_FILE_DESCRIPTORS=1)
  target_link_libraries(posix-mock-test gmock)
  target_include_directories(posix-mock-test SYSTEM PUBLIC gtest gmock)
  if (FMT_PEDANTIC)
    target_compile_options(posix-mock-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})
  endif ()
  add_test(NAME posix-mock-test COMMAND posix-mock-test)
  add_fmt_test(posix-test)
endif ()

add_fmt_executable(header-only-test
  header-only-test.cc header-only-test2.cc test-main.cc)
target_link_libraries(header-only-test gmock)
target_include_directories(header-only-test SYSTEM PUBLIC gtest gmock)
if (TARGET fmt-header-only)
  target_link_libraries(header-only-test fmt-header-only)
else ()
  target_include_directories(
    header-only-test PRIVATE ${PROJECT_SOURCE_DIR}/include)
  target_compile_definitions(header-only-test PRIVATE FMT_HEADER_ONLY=1)
endif ()

# Test that the library can be compiled with exceptions disabled.
# -fno-exception is broken in icc: https://github.com/fmtlib/fmt/issues/822.
if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  check_cxx_compiler_flag(-fno-exceptions HAVE_FNO_EXCEPTIONS_FLAG)
endif ()
if (HAVE_FNO_EXCEPTIONS_FLAG)
  add_library(noexception-test ../src/format.cc)
  target_include_directories(
    noexception-test PRIVATE ${PROJECT_SOURCE_DIR}/include)
  target_compile_options(noexception-test PRIVATE -fno-exceptions)
  if (FMT_PEDANTIC)
    target_compile_options(noexception-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})
  endif ()
  target_include_directories(noexception-test SYSTEM PUBLIC gtest gmock)
endif ()

message(STATUS "FMT_PEDANTIC: ${FMT_PEDANTIC}")

if (FMT_PEDANTIC)
  # Test that the library compiles without windows.h.
  if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    add_library(no-windows-h-test ../src/format.cc)
    target_include_directories(
      no-windows-h-test PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_compile_definitions(no-windows-h-test PRIVATE FMT_USE_WINDOWS_H=0)
    if (FMT_PEDANTIC)
      target_compile_options(no-windows-h-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})
    endif ()
    target_include_directories(no-windows-h-test SYSTEM PUBLIC gtest gmock)
  endif ()

  add_test(compile-test ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMAKE_CURRENT_SOURCE_DIR}/compile-test"
    "${CMAKE_CURRENT_BINARY_DIR}/compile-test"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-options
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}"
    "-DCXX_STANDARD_FLAG=${CXX_STANDARD_FLAG}"
    "-DPEDANTIC_COMPILE_FLAGS=${PEDANTIC_COMPILE_FLAGS}"
    "-DSUPPORTS_USER_DEFINED_LITERALS=${SUPPORTS_USER_DEFINED_LITERALS}")
endif ()

# These tests are disabled on Windows because they take too long.
if (FMT_PEDANTIC AND NOT WIN32)
  # Test if the targets are found from the build directory.
  add_test(find-package-test ${CMAKE_CTEST_COMMAND}
    -C ${CMAKE_BUILD_TYPE}
    --build-and-test
    "${CMAKE_CURRENT_SOURCE_DIR}/find-package-test"
    "${CMAKE_CURRENT_BINARY_DIR}/find-package-test"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-options
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}"
    "-DFMT_DIR=${PROJECT_BINARY_DIR}"
    "-DPEDANTIC_COMPILE_FLAGS=${PEDANTIC_COMPILE_FLAGS}"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

  # Test if the targets are found when add_subdirectory is used.
  add_test(add-subdirectory-test ${CMAKE_CTEST_COMMAND}
    -C ${CMAKE_BUILD_TYPE}
    --build-and-test
    "${CMAKE_CURRENT_SOURCE_DIR}/add-subdirectory-test"
    "${CMAKE_CURRENT_BINARY_DIR}/add-subdirectory-test"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-options
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}"
    "-DPEDANTIC_COMPILE_FLAGS=${PEDANTIC_COMPILE_FLAGS}"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
endif ()
