cmake_minimum_required(VERSION 3.28...4.5)

project(fmt-test CXX)

find_package(fmt REQUIRED)

# Check that the installed package doesn't refer to the build tree, e.g. to a
# BMI that is not installed.
file(GLOB_RECURSE package_files ${fmt_DIR}/*.cmake)
foreach (file ${package_files})
  file(READ ${file} content)
  string(FIND "${content}" "${FMT_BINARY_DIR}" pos)
  if (NOT pos EQUAL -1)
    message(FATAL_ERROR "${file} refers to ${FMT_BINARY_DIR}")
  endif ()
endforeach ()

add_executable(module-test main.cc)
target_link_libraries(module-test fmt::fmt-module)
