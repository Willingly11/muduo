# 1. 查找当前目录下的所有源文件 (.cc)
aux_source_directory(. SRC_LIST)

# 2. 生成静态库 (注意：这里用 add_library，不是 add_executable)
#    库名我们统一叫 mymuduo (对应根目录定义的项目名)
add_library(mymuduo STATIC ${SRC_LIST})

# 3. 链接线程库
#    因为我们在根目录 find_package(Threads) 了，这里直接用
target_link_libraries(mymuduo Threads::Threads)