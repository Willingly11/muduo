#include <sys/timerfd.h>
#include <strings.h>
#include <unistd.h>
#include <iostream>

// 引入你写的头文件
#include "EventLoop.h"
#include "Channel.h"
#include "Timestamp.h" // 如果你没写Timestamp，可以暂时把回调里的参数去掉

EventLoop* g_loop;

// 这是一个回调函数，当 timerfd 可读（时间到了）时，会执行这个函数
// void timeout(Timestamp receiveTime)
// {
//     printf("Timeout!\n");
    
//     // 读取 timerfd 数据，如果不读，它会一直处于“可读”状态，导致 epoll 忙轮询（Level Trigger）
//     uint64_t howmany;
//     ::read(g_loop->getEpollFd(), &howmany, sizeof howmany); // 注意：这里需要你有个办法拿到当前正在处理的fd，或者直接把fd传进来。
//     // 在标准 muduo 中，channel 内部处理 read，这里为了简化演示逻辑，
//     // 我们假设 Channel 的 ReadCallback 只是业务逻辑。
//     // 实际上，你需要在这个函数里 read 那个 timerfd，或者在外部把 timerfd 传进来 read。
    
//     // 为了演示简单，我们这里只是打印并退出循环
//     g_loop->quit(); 
// }

// 修正后的更符合 Muduo 接口的回调逻辑
// 真正的 Muduo 中，read 操作应该在 Channel::handleEvent 内部或者由用户负责
// 这里我们模拟一个简单的读取操作
void handleRead(int fd) {
    uint64_t one;
    ssize_t n = ::read(fd, &one, sizeof one);
    if (n != sizeof one) {
        std::cout << "reads " << n << " bytes instead of 8" << std::endl;
    }
    std::cout << "Timerfd triggered! EventLoop is working." << std::endl;
    g_loop->quit(); // 测试退出循环
}

int main()
{
    // 1. 创建 EventLoop
    // 这会初始化 Poller (EpollPoller)
    EventLoop loop;
    g_loop = &loop;

    // 2. 创建一个 timerfd (Linux 系统调用)
    // 这个 fd 在时间到的时候会变成“可读”状态
    int timerfd = ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    
    // 3. 创建 Channel，把 timerfd 包装起来
    Channel channel(&loop, timerfd);
    
    // 4. 设置回调函数
    // 当 Channel 感知到 timerfd 可读时，调用 handleRead
    // 使用 lambda 或者 std::bind
    channel.setReadCallback([timerfd](Timestamp receiveTime) {
        handleRead(timerfd);
    });
    
    // 5. 开启监听 (EnableReading)
    // 这一步至关重要，它会调用 loop -> updateChannel -> poller -> epoll_ctl(ADD)
    channel.enableReading();

    // 6. 设置 timerfd 的触发时间 (比如 2 秒后)
    struct itimerspec howlong;
    bzero(&howlong, sizeof howlong);
    howlong.it_value.tv_sec = 2; // 2秒
    ::timerfd_settime(timerfd, 0, &howlong, NULL);

    std::cout << "Loop starting... wait for 2 seconds..." << std::endl;

    // 7. 启动循环
    // 这会阻塞在 epoll_wait 上
    loop.loop();

    std::cout << "Loop exited." << std::endl;
    
    ::close(timerfd);
    return 0;
}

/*
既然你完成了 Phase 2（Channel, Poller, EventLoop），你现在手头已经有了一个**“事件驱动引擎”。
虽然还不能处理 TCP 连接（因为还没有 Acceptor 和 TcpConnection），
但你完全可以使用 Linux 的文件描述符（fd）来测试这个引擎是否能正常运转

最经典的测试方法是使用 timerfd（定时器文件描述符）。
因为它和 Socket 一样都是 fd，可以被 epoll 监听，而且不需要写客户端去连接它，它自己到了时间就会“可读”。 
 */

 /* 
 这个测试证明了什么？
如果你跑通了这个测试，说明你的 Reactor 核心三角 是完美的：

- EventLoop 能正常阻塞和唤醒。
- Poller (Epoll) 能正确添加 fd 并监听事件。
- Channel 能正确分发事件到回调函数。

*/
