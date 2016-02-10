cmake_minimum_required(VERSION 2.8.12)

project(cppformat-test)

add_subdirectory(../.. cppformat)

add_executable(library-test "main.cpp")
target_link_libraries(library-test cppformat)

if (TARGET cppformat-header-only)
  add_executable(header-only-test "main.cpp")
  target_link_libraries(header-only-test cppformat-header-only)
endif ()