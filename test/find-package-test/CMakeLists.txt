cmake_minimum_required(VERSION 2.8.12)

project(fmt-test)

find_package(FMT REQUIRED)

add_executable(library-test main.cc)
target_link_libraries(library-test fmt::fmt)

if (TARGET fmt::fmt-header-only)
  add_executable(header-only-test main.cc)
  target_link_libraries(header-only-test fmt::fmt-header-only)
endif ()
