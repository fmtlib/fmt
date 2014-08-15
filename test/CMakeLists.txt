set(TEST_MAIN_SRC test-main.cc gtest-extra.cc gtest-extra.h util.cc)
add_library(test-main ${TEST_MAIN_SRC})
target_link_libraries(test-main gtest)

# Adds a test.
# Usage: add_fmt_test(name libs srcs...)
function(add_fmt_test name libs)
  cmake_parse_arguments(add_fmt_test CUSTOM_LINK "" "" ${ARGN})
  add_executable(${name} ${name}.cc ${add_fmt_test_UNPARSED_ARGUMENTS})
  target_link_libraries(${name} ${libs})
  if (NOT add_fmt_test_CUSTOM_LINK)
    target_link_libraries(${name} format)
  endif ()
  add_test(${name} ${name})
endfunction()

add_fmt_test(gtest-extra-test test-main)
add_fmt_test(format-test test-main CUSTOM_LINK ../posix.cc ../posix.h)
add_fmt_test(printf-test test-main)
foreach (target format-test printf-test)
  if (CMAKE_COMPILER_IS_GNUCXX)
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS
      "-Wall -Wextra -pedantic -Wno-long-long -Wno-variadic-macros")
  endif ()
  if (CPP11_FLAG)
    set_target_properties(${target} PROPERTIES COMPILE_FLAGS ${CPP11_FLAG})
  endif ()
endforeach ()
add_fmt_test(util-test test-main)

foreach (src ${FMT_SOURCES})
  set(FMT_TEST_SOURCES ${FMT_TEST_SOURCES} ../${src})
endforeach ()

add_executable(macro-test macro-test.cc ${FMT_TEST_SOURCES} ${TEST_MAIN_SRC})
set_target_properties(macro-test
  PROPERTIES COMPILE_DEFINITIONS "FMT_USE_VARIADIC_TEMPLATES=0")
target_link_libraries(macro-test gtest)

if (HAVE_OPEN)
  add_executable(posix-test posix-test.cc ${FMT_TEST_SOURCES} ${TEST_MAIN_SRC})
  set_target_properties(posix-test
    PROPERTIES COMPILE_DEFINITIONS "FMT_INCLUDE_POSIX_TEST=1")
  target_link_libraries(posix-test gtest)
  add_test(posix-test posix-test)
endif ()

add_test(compile-test ${CMAKE_CTEST_COMMAND}
  --build-and-test
  "${CMAKE_CURRENT_SOURCE_DIR}/compile-test"
  "${CMAKE_CURRENT_BINARY_DIR}/compile-test"
  --build-generator ${CMAKE_GENERATOR}
  --build-makeprogram ${CMAKE_MAKE_PROGRAM})
