cmake_minimum_required(VERSION 2.8.12)

project(cppformat-test)

find_package(cppformat REQUIRED)

add_executable(library-test "main.cpp")
target_link_libraries(library-test cppformat)

if (TARGET cppformat-header-only)
  add_executable(header-only-test "main.cpp")
  target_link_libraries(header-only-test cppformat-header-only)
endif ()
