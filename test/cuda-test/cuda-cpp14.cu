
//
//  nvcc -x cu -std=c++14 ..\test\cuda-test\include_test.cu -I"../include" -Xcompiler /Zc:__cplusplus  -l"fmtd" -L"../build/Debug"
//

//
// Ensure that we are using the expected standard
// https://en.cppreference.com/w/cpp/preprocessor/replace#Predefined_macros
//
static_assert(__cplusplus >= 201402L, "expect C++ 2014 for nvcc");

#if defined(__CUDACC__)
#   define FMT_DEPRECATED
#endif
#include <fmt/core.h>

#include <cuda.h>
#include <iostream>

using namespace std;

extern auto make_message_cpp() -> std::string;
extern auto make_message_cuda() -> std::string;

int main(int, char*[]){
    cout << make_message_cuda() << endl;
    cout << make_message_cpp() << endl;
}

auto make_message_cuda() -> std::string{
    return fmt::format("nvcc         \t: __cplusplus == {}", __cplusplus);
}
