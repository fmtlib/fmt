cmake_minimum_required(VERSION 2.6)

# Set the default CMAKE_BUILD_TYPE to Release.
# This should be done before the project command since the latter can set
# CMAKE_BUILD_TYPE itself (it does so for nmake).
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
    "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")
endif ()

project(FORMAT)

include(CheckCXXCompilerFlag)
check_cxx_compiler_flag(-std=c++11 HAVE_STD_CPP11_FLAG)

add_library(format format.cc)
if (CMAKE_COMPILER_IS_GNUCXX)
  set_target_properties(format PROPERTIES COMPILE_FLAGS
    "-Wall -Wextra -pedantic")
endif ()
if (HAVE_STD_CPP11_FLAG)
  set_target_properties(format PROPERTIES COMPILE_FLAGS "-std=c++11")
  # Test compilation with default flags.
  add_library(testformat format.cc)
endif ()

add_subdirectory(doc)

enable_testing()

add_test(compile-test ${CMAKE_CTEST_COMMAND}
  --build-and-test
  "${CMAKE_CURRENT_SOURCE_DIR}/test"
  "${CMAKE_CURRENT_BINARY_DIR}/test"
  --build-generator ${CMAKE_GENERATOR}
  --build-makeprogram ${CMAKE_MAKE_PROGRAM})

# We compile Google Test ourselves instead of using pre-compiled libraries.
# See the Google Test FAQ "Why is it not recommended to install a
# pre-compiled copy of Google Test (for example, into /usr/local)?"
# at http://code.google.com/p/googletest/wiki/FAQ for more details.
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/gtest/CMakeLists.txt)
  option(
    gtest_force_shared_crt
    "Use shared (DLL) run-time lib even when Google Test is built as static lib."
    ON)
  if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
    add_definitions(-DGTEST_USE_OWN_TR1_TUPLE=1)
  endif ()
  
  add_subdirectory(gtest)
  include_directories(gtest/include)
  link_directories(${CMAKE_CURRENT_BINARY_DIR}/gtest)

  add_executable(format-test format-test.cc)
  target_link_libraries(format-test format gtest)
  if (CMAKE_COMPILER_IS_GNUCXX)
    set_target_properties(format-test PROPERTIES COMPILE_FLAGS
      "-Wall -Wextra -pedantic -Wno-long-long -Wno-variadic-macros")
  endif ()
  add_test(format-test format-test)
  if (HAVE_STD_CPP11_FLAG)
    set_target_properties(format-test PROPERTIES COMPILE_FLAGS "-std=c++11")
  endif ()
endif ()

find_library(PROFILER_LIB profiler)
find_path(PROFILER_INCLUDE_DIR gperftools/profiler.h)
if (PROFILER_LIB AND PROFILER_INCLUDE_DIR)
  include_directories(${PROFILER_INCLUDE_DIR})
  set(HAVE_PROFILER TRUE)
endif ()

find_package(Boost)
if (Boost_FOUND)
  add_definitions(-DHAVE_BOOST)
  include_directories(${Boost_INCLUDE_DIRS})
endif ()
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/format-benchmark/tinyformat_test.cpp)
  add_subdirectory(format-benchmark)

  add_executable(tinyformat_speed_test format-benchmark/tinyformat_test.cpp)
  target_link_libraries(tinyformat_speed_test format)
  if (HAVE_PROFILER)
    target_link_libraries(tinyformat_speed_test ${PROFILER_LIB})
    set(PROFILE_DEFS ";FMT_PROFILE")
  endif ()
  set_target_properties(tinyformat_speed_test PROPERTIES COMPILE_DEFINITIONS
    "SPEED_TEST;HAVE_FORMAT;_SCL_SECURE_NO_WARNINGS;${PROFILE_DEFS}")
  if (HAVE_STD_CPP11_FLAG)
    set_target_properties(tinyformat_speed_test PROPERTIES COMPILE_FLAGS "-std=c++11")
  endif ()

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

  add_custom_target(bloat_test_debug
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

  add_custom_target(bloat_test_optimized
    COMMAND echo running bloat tests...
    COMMAND echo printf results:
    COMMAND ./bloat_test.sh -O3
    COMMAND echo iostreams results:
    COMMAND ./bloat_test.sh -O3 -DUSE_IOSTREAMS
    COMMAND echo format results:
    COMMAND ./bloat_test.sh -O3 -DUSE_FORMAT -L.. -lformat
    COMMAND echo tinyformat results:
    COMMAND ./bloat_test.sh -O3 -DUSE_TINYFORMAT
    COMMAND echo boost results:
    COMMAND ./bloat_test.sh -O3 -DUSE_BOOST
    WORKING_DIRECTORY tinyformat
    DEPENDS format)
endif ()
