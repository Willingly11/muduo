#设置最低版本和项目名称
cmake_minimum_required(VERSION 3.0)
project(my_muduo)

set (CMAKE_CXX_STANDARD 11) 
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 开启调试模式
set(CMAKE_BUILD_TYPE Debug)
add_compile_options(-Wall -g)

# 输出路径设置
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin) # 可执行文件放 bin

include_directories(${PROJECT_SOURCE_DIR}/include)

find_package(Threads REQUIRED)

#添加子目录
add_subdirectory(src)
add_subdirectory(example)