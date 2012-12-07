cmake_minimum_required(VERSION 2.6)

# Set the default CMAKE_BUILD_TYPE to Release.
# This should be done before the project command since the latter can set
# CMAKE_BUILD_TYPE itself (it does so for nmake).
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
    "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
endif ()

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

if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/tinyformat/tinyformat_test.cpp)
  add_executable(tinyformat_speed_test tinyformat/tinyformat_test.cpp)
  target_link_libraries(tinyformat_speed_test format)
  set_target_properties(tinyformat_speed_test
    PROPERTIES COMPILE_DEFINITIONS "SPEED_TEST;HAVE_FORMAT")

  add_custom_target(speed_test
    COMMAND @echo running speed tests...
    COMMAND @echo printf timings:
    COMMAND @time -p ./tinyformat_speed_test printf > /dev/null
    COMMAND @echo iostreams timings:
    COMMAND @time -p ./tinyformat_speed_test iostreams > /dev/null
    COMMAND @echo format timings:
    COMMAND @time -p ./tinyformat_speed_test format > /dev/null
    COMMAND @echo tinyformat timings:
    COMMAND @time -p ./tinyformat_speed_test tinyformat > /dev/null
    COMMAND @echo boost timings:
    COMMAND @time -p ./tinyformat_speed_test boost > /dev/null
    DEPENDS tinyformat_speed_test)

  add_custom_target(bloat_test
    COMMAND echo running bloat tests...
    COMMAND echo printf results:
    COMMAND ./bloat_test.sh
    COMMAND echo iostreams results:
    COMMAND ./bloat_test.sh -DUSE_IOSTREAMS
    COMMAND echo format results:
    COMMAND ./bloat_test.sh -DUSE_FORMAT -L.. -lformat
    COMMAND echo tinyformat results:
    COMMAND ./bloat_test.sh -DUSE_TINYFORMAT
    COMMAND echo boost results:
    COMMAND ./bloat_test.sh -DUSE_BOOST
    WORKING_DIRECTORY tinyformat
    DEPENDS format)
endif ()
