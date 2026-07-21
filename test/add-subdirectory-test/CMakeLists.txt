cmake_minimum_required(VERSION 3.8...3.25)

project(fmt-test CXX)

add_subdirectory(../.. fmt)

add_executable(library-test main.cc)
target_include_directories(library-test PUBLIC SYSTEM .)
target_compile_options(library-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})
target_link_libraries(library-test fmt::fmt)

if (TARGET fmt::fmt-header-only)
  add_executable(header-only-test main.cc)
  target_include_directories(header-only-test PUBLIC SYSTEM .)
  target_compile_options(header-only-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})
  target_link_libraries(header-only-test fmt::fmt-header-only)
endif ()

# Test that a target publicly depending on fmt can itself be exported even
# though fmt is added via add_subdirectory and FMT_INSTALL is off (#4806).
add_library(export-test STATIC export-lib.cc)
target_link_libraries(export-test PUBLIC fmt::fmt)
export(
  TARGETS export-test
  NAMESPACE test::
  FILE ${CMAKE_CURRENT_BINARY_DIR}/export-test-targets.cmake)
