cmake_minimum_required(VERSION 2.6)
project(FORMAT)

add_library(format format.cc)

# We compile Google Test ourselves instead of using pre-compiled libraries.
# See the Google Test FAQ "Why is it not recommended to install a
# pre-compiled copy of Google Test (for example, into /usr/local)?"
# at http://code.google.com/p/googletest/wiki/FAQ for more details.
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/gtest/CMakeLists.txt)
  add_subdirectory(gtest)
  include_directories(gtest/include)
  link_directories(${CMAKE_CURRENT_BINARY_DIR}/gtest)
  enable_testing()
  add_executable(format_test format_test.cc gtest/src/gtest_main.cc)
  target_link_libraries(format_test format gtest)
  add_test(format_test format_test)
endif ()
