
#
# We can find some usecases which follows the guide of CMake.
# The way replaces 'find_package(CUDA)' to 'enable_language(CUDA)'.
# And let the CMake built-in functions to use NVCC.
#
# See: https://cmake.org/cmake/help/latest/module/FindCUDA.html#replacement
#
# However, such CMake versions are pretty high (3.10 or later).
# And we can't sure most of the CUDA projects are using those latest
# because the latest C++ standard for NVCC is C++ 14 at this moment.
#
# In conclusion, this test will use older version of CMake,
# and rely on 'find_package(CUDA)'. For its version, do follow the Root CMakeLists.txt.
#

#
# cmake_minimum_required(VERSION 3.10)
# project(fmt-cuda-test LANGUAGES CXX CUDA) # see 'enable_language(CUDA)'
cmake_minimum_required(VERSION 3.1) # for C++ 14
project(fmt-cuda-test LANGUAGES CXX)

# find from ${PROJECT_BINARY_DIR}
find_package(FMT    REQUIRED) 

# The environment variables(CUDA_BIN_PATH & CUDA_PATH) must be specified at this point
find_package(CUDA   REQUIRED)

#
# Update these when NVCC becomes ready for C++ 17 features
# https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#cpp14-language-features
#
set(CMAKE_CUDA_STANDARD 14)
set(CMAKE_CUDA_STANDARD_REQUIRED 14)

list(APPEND CUDA_NVCC_FLAGS "-std=c++14")
if(MSVC)
    # this is the solution of pytorch
    #   https://github.com/pytorch/pytorch/pull/7118
    list(APPEND CUDA_NVCC_FLAGS "-Xcompiler" "/std:c++14")
    list(APPEND CUDA_NVCC_FLAGS "-Xcompiler" "/Zc:__cplusplus")
    # for the reason of this -Xcompiler options, see below ...
endif()

#
# In this test, we will assume that 
# the user is going to pre-compile CUDA source codes
# with some libraries (including this 'fmt').
#
# In addition to that, 
# this test will invoke both C++ Host compiler and NVCC 
# by providing another (non-CUDA) C++ source code
#
cuda_add_executable(fmt-in-cuda-test
    cuda-cpp14.cu
    cpp14.cc
)

#
# https://docs.nvidia.com/cuda/cuda-compiler-driver-nvcc/index.html
#
set_target_properties(fmt-in-cuda-test
PROPERTIES
    CXX_STANDARD 14 # Notice this is for C++ code
    POSITION_INDEPENDENT_CODE ON
)
# target_compile_features(fmt-in-cuda-test
# PRIVATE
#     cxx_std_14 # just make sure of the property is available
# )

get_target_property(cuda_standard 
    fmt-in-cuda-test CUDA_STANDARD
)
message(STATUS "cuda_standard:          ${cuda_standard}")

get_target_property(cuda_standard_required
    fmt-in-cuda-test CUDA_STANDARD_REQUIRED
)
message(STATUS "cuda_standard_required: ${cuda_standard_required}")

#
# https://cmake.org/cmake/help/latest/module/FindCUDA.html
#
# From the document, you can see "The default is to use no keyword"
#
target_link_libraries(fmt-in-cuda-test
# PUBLIC
    fmt::fmt
)

if(MSVC)
    #
    # MSVC places incorrect '__cplusplus' macro. Follow the guideline.
    #
    # https://github.com/Microsoft/vscode-cpptools/issues/2595
    # https://devblogs.microsoft.com/cppblog/msvc-now-correctly-reports-__cplusplus/
    #
    target_compile_options(fmt-in-cuda-test
    PRIVATE
        /Zc:__cplusplus
        /permissive-
    )
endif()
