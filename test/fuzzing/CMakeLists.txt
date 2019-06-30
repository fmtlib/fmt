# Copyright (c) 2019, Paul Dreik
# License: see LICENSE.rst in the fmt root directory

# settings this links in a main. useful for reproducing,
# kcov, gdb, afl, valgrind.
# (note that libFuzzer can also reproduce, just pass it the files)
option(FMT_FUZZ_LINKMAIN "enables the reproduce mode, instead of libFuzzer" On)

# For oss-fuzz - insert $LIB_FUZZING_ENGINE into the link flags, but only for
# the fuzz targets, otherwise the cmake configuration step fails.
set(FMT_FUZZ_LDFLAGS "" CACHE STRING "LDFLAGS for the fuzz targets")

# Find all fuzzers.
set(SOURCES
  chrono_duration.cpp
  named_arg.cpp
  one_arg.cpp
  sprintf.cpp
  two_args.cpp
)

macro(implement_fuzzer sourcefile)
  get_filename_component(basename ${sourcefile} NAME_WE)
  set(name fuzzer_${basename})
  add_executable(${name} ${sourcefile} fuzzer_common.h)
  if (FMT_FUZZ_LINKMAIN)
      target_sources(${name} PRIVATE main.cpp)
  endif ()
  target_link_libraries(${name} PRIVATE fmt)
if (FMT_FUZZ_LDFLAGS)
  target_link_libraries(${name} PRIVATE ${FMT_FUZZ_LDFLAGS})
endif ()
  target_compile_features(${name} PRIVATE cxx_generic_lambdas)
endmacro ()

foreach (X IN ITEMS ${SOURCES})
  implement_fuzzer(${X})
endforeach ()
