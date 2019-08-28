
//
//  Direct NVCC command line example:
//
//  nvcc.exe ./cuda-cpp14.cu -x cu -I"../include" -l"fmtd" -L"../build/Debug"  \
//          -std=c++14 -Xcompiler /std:c++14 -Xcompiler /Zc:__cplusplus
//

//
// Ensure that we are using the latest C++ standard for NVCC
// The version is C++14
//
// https://docs.nvidia.com/cuda/cuda-c-programming-guide/index.html#c-cplusplus-language-support
// https://en.cppreference.com/w/cpp/preprocessor/replace#Predefined_macros
//
static_assert(__cplusplus >= 201402L, "expect C++ 2014 for nvcc");

//
// https://docs.nvidia.com/cuda/cuda-compiler-driver-nvcc/index.html#nvcc-identification-macro
//
// __NVCC__ is for NVCC compiler
// __CUDACC__ is for CUDA(.cu) source code
//
// Since we don't know the actual case in this header, checking both macro
// will prevent possible pitfalls ...
//
#if defined(__NVCC__) || defined(__CUDACC__)
#  define FMT_DEPRECATED
#endif
#include <fmt/core.h>

#include <cuda.h>
#include <iostream>

using namespace std;

extern auto make_message_cpp() -> std::string;
extern auto make_message_cuda() -> std::string;

int main(int, char*[]) {
  cout << make_message_cuda() << endl;
  cout << make_message_cpp() << endl;
}

auto make_message_cuda() -> std::string {
  return fmt::format("nvcc compiler \t: __cplusplus == {}", __cplusplus);
}
