cmake_minimum_required(VERSION 3.28)

project(fmt-test CXX)

find_package(fmt REQUIRED)

add_executable(module-test main.cc)
target_link_libraries(module-test fmt::fmt-module)
