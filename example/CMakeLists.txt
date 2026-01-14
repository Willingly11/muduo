# 1. 获取当前目录下的源文件
file(GLOB EXAMPLE_SRCS *.cpp)

# 2. 生成可执行文件
#    注意：这里我们做一个自动化遍历，或者手动指定 testserver
#    如果你只想编译 testserver.cc：
add_executable(test_Channel_Epoll_EventLoop test_Channel_Epoll_EventLoop.cpp)

# 3. 链接库
target_link_libraries(test_Channel_Epoll_EventLoop mymuduo Threads::Threads)