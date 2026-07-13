cmake_minimum_required(VERSION 3.8...4.4)

project(fmt-test)

find_package(FMT REQUIRED)

set(PEDANTIC_COMPILE_FLAGS)

add_executable(library-test main.cc)
target_link_libraries(library-test fmt::fmt)
target_compile_options(library-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})

if (TARGET fmt::fmt-header-only)
  add_executable(header-only-test main.cc)
  target_link_libraries(header-only-test fmt::fmt-header-only)
  target_compile_options(header-only-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})
endif ()

if (TARGET fmt::fmt-module)
  add_executable(module-test main.cc)
  target_link_libraries(module-test fmt::fmt-module)
  target_compile_options(module-test PRIVATE ${PEDANTIC_COMPILE_FLAGS})
  if (FMT_IMPORT_STD)
    target_compile_features(module-test PRIVATE cxx_std_23)
    set_target_properties(module-test PROPERTIES CXX_MODULE_STD 1)
  endif ()
endif ()
