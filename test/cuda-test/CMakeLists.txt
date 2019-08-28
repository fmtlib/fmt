# We can find some usecases which follow the guide of CMake which uses
# `enable_language(CUDA)` instead of `find_package(CUDA)` and let the CMake
# built-in functions use NVCC.

# See: https://cmake.org/cmake/help/latest/module/FindCUDA.html#replacement
#
# However, this requires CMake version 3.10 or higher and we can't be sure most
# of the CUDA projects are using those.
#
# This test relies on `find_package(CUDA)` in the parent CMake config.

# These can be updated when NVCC becomes ready for C++ 17 features
# https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#cpp14-language-features
set(CMAKE_CUDA_STANDARD 14)
set(CMAKE_CUDA_STANDARD_REQUIRED 14)

# https://docs.nvidia.com/cuda/cuda-compiler-driver-nvcc/index.html
list(APPEND CUDA_NVCC_FLAGS "-std=c++14")
if (MSVC)
  # This is the solution of pytorch:
  # https://github.com/pytorch/pytorch/pull/7118
  list(APPEND CUDA_NVCC_FLAGS "-Xcompiler" "/std:c++14")
  list(APPEND CUDA_NVCC_FLAGS "-Xcompiler" "/Zc:__cplusplus")
  # for the reason of this -Xcompiler options, see below.
endif ()

# In this test, we assume that the user is going to compile CUDA source code
# with some libraries (fmt in this case).
#
# In addition to that, this test invokes both the C++ host compiler and NVCC
# by providing another (non-CUDA) C++ source code.
cuda_add_executable(fmt-in-cuda-test cuda-cpp14.cu cpp14.cc)
target_compile_features(fmt-in-cuda-test PRIVATE cxx_std_14)

get_target_property(IN_USE_CUDA_STANDARD fmt-in-cuda-test CUDA_STANDARD)
message(STATUS "cuda_standard:          ${IN_USE_CUDA_STANDARD}")

get_target_property(IN_USE_CUDA_STANDARD_REQUIRED
    fmt-in-cuda-test CUDA_STANDARD_REQUIRED)
message(STATUS "cuda_standard_required: ${IN_USE_CUDA_STANDARD_REQUIRED}")

# We don't use PUBLIC or other keyword for reasons explained in the
# CUDA_LINK_LIBRARIES_KEYWORD section in
# https://cmake.org/cmake/help/latest/module/FindCUDA.html
target_link_libraries(fmt-in-cuda-test fmt::fmt)

if (MSVC)
  # This part is for (non-CUDA) C++ code. MSVC can define incorrect
  # `__cplusplus` macro. Fix for the issue is to use additional compiler flag. 
  #
  # See Also:
  # https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
  # https://github.com/Microsoft/vscode-cpptools/issues/2595
  target_compile_options(fmt-in-cuda-test PRIVATE /Zc:__cplusplus /permissive-)
endif ()
