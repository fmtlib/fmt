cmake_minimum_required(VERSION 3.10)
project(fmt_sample)

# C++のバージョンを指定
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# fmtライブラリを指定
find_package(fmt REQUIRED)

# 実行ファイルを生成
add_executable(fmt_sample main.cpp)

add_subdirectory(/home/yusuke/oss/fmt)

# fmtをリンク
target_link_libraries(fmt_sample PRIVATE fmt::fmt)
