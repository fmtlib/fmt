#------------------------------------------------------------------------------
# Build the google test library

# We compile Google Test ourselves instead of using pre-compiled libraries.
# See the Google Test FAQ "Why is it not recommended to install a
# pre-compiled copy of Google Test (for example, into /usr/local)?"
# at http://code.google.com/p/googletest/wiki/FAQ for more details.
add_library(gmock STATIC
  gmock-gtest-all.cc gmock/gmock.h gtest/gtest.h gtest/gtest-spi.h)
target_compile_definitions(gmock PUBLIC GTEST_HAS_STD_WSTRING=1)
target_include_directories(gmock SYSTEM PUBLIC . gmock gtest)

find_package(Threads)
if (Threads_FOUND)
  target_link_libraries(gmock ${CMAKE_THREAD_LIBS_INIT})
else ()
  target_compile_definitions(gmock PUBLIC GTEST_HAS_PTHREAD=0)
endif ()

target_compile_definitions(gmock PUBLIC GTEST_LANG_CXX11=0)

if (MSVC)
  # Workaround a bug in implementation of variadic templates in MSVC11.
  target_compile_definitions(gmock PUBLIC _VARIADIC_MAX=10)
  
  # Disable MSVC warnings of _CRT_INSECURE_DEPRECATE functions.
  target_compile_definitions(gmock PRIVATE _CRT_SECURE_NO_WARNINGS)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    # Disable MSVC warnings of POSIX functions.
    target_compile_options(gmock PUBLIC -Wno-deprecated-declarations)
  endif ()
endif ()

# GTest doesn't detect <tuple> with clang.
if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  target_compile_definitions(gmock PUBLIC GTEST_USE_OWN_TR1_TUPLE=1)
endif ()

# Silence MSVC tr1 deprecation warning in gmock.
target_compile_definitions(gmock
  PUBLIC _SILENCE_TR1_NAMESPACE_DEPRECATION_WARNING=1)

#------------------------------------------------------------------------------
# Build the actual library tests

set(TEST_MAIN_SRC test-main.cc gtest-extra.cc gtest-extra.h util.cc)
add_library(test-main STATIC ${TEST_MAIN_SRC})
target_include_directories(test-main SYSTEM PUBLIC gtest gmock)
target_link_libraries(test-main gmock fmt)

include(CheckCXXCompilerFlag)

# Workaround GTest bug https://github.com/google/googletest/issues/705.
check_cxx_compiler_flag(
  -fno-delete-null-pointer-checks HAVE_FNO_DELETE_NULL_POINTER_CHECKS)
if (HAVE_FNO_DELETE_NULL_POINTER_CHECKS)
  target_compile_options(test-main PUBLIC -fno-delete-null-pointer-checks)
endif ()

# Use less strict pedantic flags for the tests because GMock doesn't compile
# cleanly with -pedantic and -std=c++98.
if (CMAKE_COMPILER_IS_GNUCXX OR (CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
    #set(PEDANTIC_COMPILE_FLAGS -Wall -Wextra -Wno-long-long -Wno-variadic-macros)
endif ()

function(add_fmt_executable name)
  add_executable(${name} ${ARGN})
  if (MINGW)
    target_link_libraries(${name} -static-libgcc -static-libstdc++)
  endif ()
endfunction()

# Adds a test.
# Usage: add_fmt_test(name srcs...)
function(add_fmt_test name)
  add_fmt_executable(${name} ${name}.cc ${ARGN})
  target_link_libraries(${name} test-main)

  # Define if certain C++ features can be used.
  if (FMT_PEDANTIC)
    target_compile_options(${name} PRIVATE ${PEDANTIC_COMPILE_FLAGS})
  endif ()
  if (FMT_WERROR)
    target_compile_options(${name} PRIVATE ${WERROR_FLAG})
  endif ()
  target_include_directories(${name} SYSTEM PUBLIC gtest gmock)
  add_test(NAME ${name} COMMAND ${name})
endfunction()

add_fmt_test(assert-test)
add_fmt_test(chrono-test)
add_fmt_test(color-test)
add_fmt_test(core-test)
add_fmt_test(gtest-extra-test)
add_fmt_test(format-test mock-allocator.h)
if (MSVC)
  target_compile_options(format-test PRIVATE /bigobj)
endif ()
if (NOT (MSVC AND BUILD_SHARED_LIBS))
  add_fmt_test(format-impl-test)
endif ()
add_fmt_test(locale-test)
add_fmt_test(ostream-test)
add_fmt_test(compile-test)
add_fmt_test(printf-test)
add_fmt_test(ranges-test)
add_fmt_test(scan-test)

if (NOT DEFINED MSVC_STATIC_RUNTIME AND MSVC)
  foreach (flag_var
			 CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
			 CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
	if (${flag_var} MATCHES "^(/|-)(MT|MTd)")
	  set(MSVC_STATIC_RUNTIME ON)
	  break()
	endif()
  endforeach()
endif()

if (NOT MSVC_STATIC_RUNTIME)
  add_fmt_executable(posix-mock-test
    posix-mock-test.cc ../src/format.cc ${TEST_MAIN_SRC})
  target_include_directories(
    posix-mock-test PRIVATE ${PROJECT_SOURCE_DIR}/include)
  target_link_libraries(posix-mock-test gmock)
  target_include_directories(posix-mock-test SYSTEM PUBLIC gtest gmock)
  if (FMT_PEDANTIC)
    target_compile_options(posix-mock-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})
  endif ()
  if (HAVE_STRTOD_L)
    target_compile_definitions(posix-mock-test PRIVATE FMT_LOCALE)
  endif ()
  add_test(NAME posix-mock-test COMMAND posix-mock-test)
  add_fmt_test(os-test)
endif ()

add_fmt_executable(header-only-test
  header-only-test.cc header-only-test2.cc test-main.cc)
target_link_libraries(header-only-test gmock)
target_include_directories(header-only-test SYSTEM PUBLIC gtest gmock)
if (TARGET fmt-header-only)
  target_link_libraries(header-only-test fmt-header-only)
else ()
  target_include_directories(
    header-only-test PRIVATE ${PROJECT_SOURCE_DIR}/include)
  target_compile_definitions(header-only-test PRIVATE FMT_HEADER_ONLY=1)
endif ()

message(STATUS "FMT_PEDANTIC: ${FMT_PEDANTIC}")

if (FMT_PEDANTIC)
  # MSVC fails to compile GMock when C++17 is enabled.
  if (FMT_HAS_VARIANT AND NOT MSVC)
    add_fmt_test(std-format-test)
    set_property(TARGET std-format-test PROPERTY CXX_STANDARD 17)
  endif ()

  # Test that the library can be compiled with exceptions disabled.
  # -fno-exception is broken in icc: https://github.com/fmtlib/fmt/issues/822.
  if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
    check_cxx_compiler_flag(-fno-exceptions HAVE_FNO_EXCEPTIONS_FLAG)
  endif ()
  if (HAVE_FNO_EXCEPTIONS_FLAG)
    add_library(noexception-test ../src/format.cc)
    target_include_directories(
      noexception-test PRIVATE ${PROJECT_SOURCE_DIR}/include)
    target_compile_options(noexception-test PRIVATE -fno-exceptions)
    if (FMT_PEDANTIC)
      target_compile_options(noexception-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})
    endif ()
  endif ()

  # Test that the library compiles without locale.
  add_library(nolocale-test ../src/format.cc)
  target_include_directories(
    nolocale-test PRIVATE ${PROJECT_SOURCE_DIR}/include)
  target_compile_definitions(
    nolocale-test PRIVATE FMT_STATIC_THOUSANDS_SEPARATOR=1)

  add_test(compile-error-test ${CMAKE_CTEST_COMMAND}
    --build-and-test
    "${CMAKE_CURRENT_SOURCE_DIR}/compile-error-test"
    "${CMAKE_CURRENT_BINARY_DIR}/compile-error-test"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-options
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}"
    "-DCXX_STANDARD_FLAG=${CXX_STANDARD_FLAG}"
    "-DPEDANTIC_COMPILE_FLAGS=${PEDANTIC_COMPILE_FLAGS}"
    "-DSUPPORTS_USER_DEFINED_LITERALS=${SUPPORTS_USER_DEFINED_LITERALS}")
endif ()

# These tests are disabled on Windows because they take too long.
if (FMT_PEDANTIC AND NOT WIN32)
  # Test if the targets are found from the build directory.
  add_test(find-package-test ${CMAKE_CTEST_COMMAND}
    -C ${CMAKE_BUILD_TYPE}
    --build-and-test
    "${CMAKE_CURRENT_SOURCE_DIR}/find-package-test"
    "${CMAKE_CURRENT_BINARY_DIR}/find-package-test"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-options
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}"
    "-DFMT_DIR=${PROJECT_BINARY_DIR}"
    "-DPEDANTIC_COMPILE_FLAGS=${PEDANTIC_COMPILE_FLAGS}"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")

  # Test if the targets are found when add_subdirectory is used.
  add_test(add-subdirectory-test ${CMAKE_CTEST_COMMAND}
    -C ${CMAKE_BUILD_TYPE}
    --build-and-test
    "${CMAKE_CURRENT_SOURCE_DIR}/add-subdirectory-test"
    "${CMAKE_CURRENT_BINARY_DIR}/add-subdirectory-test"
    --build-generator ${CMAKE_GENERATOR}
    --build-makeprogram ${CMAKE_MAKE_PROGRAM}
    --build-options
    "-DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}"
    "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}"
    "-DPEDANTIC_COMPILE_FLAGS=${PEDANTIC_COMPILE_FLAGS}"
    "-DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}")
endif ()

# Activate optional CUDA tests if CUDA is found. For version selection see
# https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#cpp14-language-features
if (FMT_CUDA_TEST)
  if (${CMAKE_VERSION} VERSION_LESS 3.15)
    find_package(CUDA 9.0)
  else ()
    include(CheckLanguage)
    check_language(CUDA)
    if (CMAKE_CUDA_COMPILER)
      enable_language(CUDA OPTIONAL)
      set(CUDA_FOUND TRUE)
    endif ()
  endif ()

  if (CUDA_FOUND)
    add_subdirectory(cuda-test)
    add_test(NAME cuda-test COMMAND fmt-in-cuda-test)
  endif ()
endif ()
