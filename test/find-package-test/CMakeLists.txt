cmake_minimum_required(VERSION 3.8...3.30)

project(fmt-test)

find_package(FMT 11.0.1 REQUIRED HINTS ${FMT_DIR})

add_executable(library-test main.cc)
target_link_libraries(library-test fmt::fmt)
target_compile_options(library-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})

if (TARGET fmt::fmt-header-only)
  add_executable(header-only-test main.cc)
  target_link_libraries(header-only-test fmt::fmt-header-only)
  target_compile_options(header-only-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})
endif ()

if (CMAKE_VERSION VERSION_GREATER_EQUAL 3.28.4 AND
  CMAKE_GENERATOR STREQUAL "Ninja")
  if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 17.0)
    set(FMT_MODULE TRUE)
  elseif (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 14.0)
    set(FMT_MODULE TRUE)
  endif ()
endif ()

if (FMT_MODULE)
  message(STATUS "FMT_MODULE: ${FMT_MODULE}")
  set(CMAKE_CXX_SCAN_FOR_MODULES ON)
  set(CMAKE_CXX_STANDARD 23)
  set(CMAKE_CXX_EXTENSIONS OFF)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  # TODO: not yet tested! add_compile_definitions(FMT_IMPORT_STD)
  # Always use libc++
  # add_compile_options(-stdlib=libc++)
  # add_link_options(-stdlib=libc++)
  # MYBE: generate new BMI with other comiler settings
  # XXX add_fmt_module(fmt-module)

  add_executable(module-test main.cc)
  target_link_libraries(module-test fmt::fmt)
  target_compile_definitions(module-test PUBLIC FMT_MODULE)
  target_compile_options(module-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})
endif ()
