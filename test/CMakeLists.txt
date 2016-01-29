set(FMT_GMOCK_DIR ../gmock)

include_directories(.. ${FMT_GMOCK_DIR})

# Links target with cppformat and any libraries passed as extra arguments.
function (target_link_cppformat target)
  target_link_libraries(${target} cppformat ${ARGN})
  if (BUILD_SHARED_LIBS)
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS -DFMT_SHARED)
  endif ()
endfunction ()

# We compile Google Test ourselves instead of using pre-compiled libraries.
# See the Google Test FAQ "Why is it not recommended to install a
# pre-compiled copy of Google Test (for example, into /usr/local)?"
# at http://code.google.com/p/googletest/wiki/FAQ for more details.

add_library(gmock STATIC
  ${FMT_GMOCK_DIR}/gmock-gtest-all.cc ${FMT_GMOCK_DIR}/gmock/gmock.h
  ${FMT_GMOCK_DIR}/gtest/gtest.h ${FMT_GMOCK_DIR}/gtest/gtest-spi.h)
target_include_directories(gmock INTERFACE ${FMT_GMOCK_DIR})
find_package(Threads)
if (Threads_FOUND)
  target_link_libraries(gmock ${CMAKE_THREAD_LIBS_INIT})
else ()
  target_compile_definitions(gmock PUBLIC GTEST_HAS_PTHREAD=0)
endif ()

# Check if variadic templates are working and not affected by GCC bug 39653:
# https://gcc.gnu.org/bugzilla/show_bug.cgi?id=39653
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

set(TEST_MAIN_SRC test-main.cc gtest-extra.cc gtest-extra.h util.cc)
add_library(test-main STATIC ${TEST_MAIN_SRC})
target_link_cppformat(test-main gmock)

# Adds a test.
# Usage: add_fmt_test(name [CUSTOM_LINK] srcs...)
function(add_fmt_test name)
  cmake_parse_arguments(add_fmt_test CUSTOM_LINK "" "" ${ARGN})
  add_executable(${name} ${name}.cc ${add_fmt_test_UNPARSED_ARGUMENTS})
  target_link_libraries(${name} test-main)
  if (NOT add_fmt_test_CUSTOM_LINK)
    target_link_cppformat(${name})
  endif ()
  add_test(NAME ${name} COMMAND ${name})
endfunction()

add_fmt_test(assert-test)
add_fmt_test(gtest-extra-test)
add_fmt_test(format-test)
if (FMT_PEDANTIC AND MSVC)
  set_target_properties(format-test PROPERTIES COMPILE_FLAGS /W4)
endif ()

add_fmt_test(format-impl-test CUSTOM_LINK)
add_fmt_test(printf-test)
foreach (target format-test printf-test)
  if (FMT_PEDANTIC AND CMAKE_COMPILER_IS_GNUCXX)
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS
      "-Wall -Wextra -pedantic -Wno-long-long -Wno-variadic-macros")
  endif ()
  if (CPP11_FLAG)
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS ${CPP11_FLAG})
  endif ()
endforeach ()
add_fmt_test(util-test mock-allocator.h)
if (CPP11_FLAG)
  set_target_properties(util-test PROPERTIES COMPILE_FLAGS ${CPP11_FLAG})
endif ()

if (SUPPORTS_ENUM_BASE)
  set_target_properties(util-test
    PROPERTIES COMPILE_DEFINITIONS "FMT_USE_ENUM_BASE=1")
endif ()

foreach (src ${FMT_SOURCES})
  set(FMT_TEST_SOURCES ${FMT_TEST_SOURCES} ../${src})
endforeach ()

if (SUPPORTS_TYPE_TRAITS)
  foreach (target format-test util-test)
    set_target_properties(${target}
      PROPERTIES COMPILE_DEFINITIONS "FMT_USE_TYPE_TRAITS=1")
  endforeach ()
endif ()

add_executable(macro-test macro-test.cc ${FMT_TEST_SOURCES} ${TEST_MAIN_SRC})
target_link_libraries(macro-test gmock)

if (HAVE_OPEN)
  add_executable(posix-mock-test posix-mock-test.cc ../cppformat/format.cc ${TEST_MAIN_SRC})
  target_include_directories(posix-mock-test PRIVATE ${PROJECT_SOURCE_DIR})
  target_compile_definitions(posix-mock-test PRIVATE FMT_USE_FILE_DESCRIPTORS=1)
  target_link_libraries(posix-mock-test gmock)
  add_test(NAME posix-mock-test COMMAND posix-mock-test)
  add_fmt_test(posix-test)
endif ()

add_executable(header-only-test
  header-only-test.cc header-only-test2.cc test-main.cc)
target_link_libraries(header-only-test gmock)
if (TARGET cppformat-header-only)
  target_link_libraries(header-only-test cppformat-header-only)
else ()
  target_include_directories(header-only-test PRIVATE ${PROJECT_SOURCE_DIR})
  target_compile_definitions(header-only-test PRIVATE FMT_HEADER_ONLY=1)
endif ()

# Test that the library can be compiled with exceptions disabled.
check_cxx_compiler_flag(-fno-exceptions HAVE_FNO_EXCEPTIONS_FLAG)
if (HAVE_FNO_EXCEPTIONS_FLAG)
  add_library(noexception-test STATIC ../cppformat/format.cc)
  set_target_properties(noexception-test
    PROPERTIES COMPILE_FLAGS -fno-exceptions)
endif ()

if (FMT_PEDANTIC)
  add_test(compile-test ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMAKE_CURRENT_SOURCE_DIR}/compile-test"
    "${CMAKE_CURRENT_BINARY_DIR}/compile-test"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM})

  # Test that the library compiles without windows.h.
  add_library(no-windows-h-test ../cppformat/format.cc)
  set_target_properties(no-windows-h-test
    PROPERTIES COMPILE_DEFINITIONS "FMT_USE_WINDOWS_H=0")
endif ()
