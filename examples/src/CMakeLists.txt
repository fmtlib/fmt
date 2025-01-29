cmake_minimum_required(VERSION 3.2)

project(fmt_example LANGUAGE CXX)

add_executable(fmt_example main.cpp)

add_subdirectory(../../fmt)

target_link_libraries(fmt_example PRIVATE fmt::fmt)
