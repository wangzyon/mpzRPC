# 设置cmake的最低版本和项目名称
cmake_minimum_required(VERSION 3.0)
project(mpzrpc)

# 生成debug版本，可以进行gdb调试
set(CMAKE_BUILD_TYPE "Debug")

# 设置项目库文件输出的路径
set(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)

# 设置项目编译头文件搜索路径 -I
include_directories(${PROJECT_SOURCE_DIR}/include)
include_directories(${PROJECT_SOURCE_DIR}/example/protos)

# 设置项目库文件搜索路径 -L
link_directories(${PROJECT_SOURCE_DIR}/lib)

# src包含了mpzrpc框架所有的相关代码
add_subdirectory(src)
# example包含了mpzrpc框架使用的示例代码
add_subdirectory(example)