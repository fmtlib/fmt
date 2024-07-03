cmake_minimum_required(VERSION 3.8...3.30)

project(fmt-test)

find_package(FMT 11.0.0 REQUIRED HINTS ${FMT_DIR})

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.28.4 AND
    CMAKE_GENERATOR STREQUAL "Ninja" AND
    CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  set(FMT_MODULE TRUE)
endif ()

add_executable(library-test main.cc)
target_link_libraries(library-test fmt::fmt)
target_compile_options(library-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})

if (TARGET fmt::fmt-header-only)
  add_executable(header-only-test main.cc)
  target_link_libraries(header-only-test fmt::fmt-header-only)
  target_compile_options(header-only-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})
endif ()

if (FMT_MODULE)
  message(STATUS "FMT_MODULE: ${FMT_MODULE}")
  set(CMAKE_CXX_SCAN_FOR_MODULES ON)
  # Always use libc++
  set(CMAKE_CXX_STANDARD 23)
  set(CMAKE_CXX_EXTENSIONS OFF)
  add_compile_options(-stdlib=libc++)
  add_link_options(-stdlib=libc++)
  add_fmt_module(fmt-module)

  add_executable(module-test main.cc)
  target_link_libraries(module-test fmt-module)
endif ()
