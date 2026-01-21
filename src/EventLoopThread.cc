#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc, this), name)
    , mutex_()
    , cond_()
    , callback_(cb)
{
}
/* 
对于全局函数或静态成员函数，函数名可以隐式转换为函数指针
（就像数组名可以转换为数组首地址一样），所以写不写 & 都可以。
但非静态成员函数由于其特殊性（需要隐藏的 this 指针），
C++ 标准强制要求加上 & 以免产生歧义。
*/


EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        loop_->quit();
        thread_.join();
    }
}

//主线程 (Caller)：调用 startLoop() 的线程
//子线程 (Worker)：thread_.start() 启动的那个新线程，它运行 threadFunc()
EventLoop *EventLoopThread::startLoop()
{
    thread_.start(); // 启用底层线程Thread类对象thread_中通过start()创建的线程

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this](){return loop_ != nullptr;});
        loop = loop_;
    }
    return loop;
}

// 下面这个方法 是在单独的新线程里运行的
void EventLoopThread::threadFunc()
{
    //one loop per thread：由新创建的线程来创建EventLoop对象，构造函数默认会绑定创建该对象的线程TID
    EventLoop loop; // 创建一个独立的EventLoop对象 和上面的线程是一一对应的

    if (callback_)
    {
        callback_(&loop);
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }
    loop.loop();    // 执行EventLoop的loop() 开启了底层的Poller的poll()，子线程进入事件循环

    //执行到这里，说明loop已经退出了
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}

/**
Q1: 为什么要这么麻烦？直接在主线程 new 一个 EventLoop 传给子线程不行吗？
绝对不行！ Muduo 的核心原则是 One Loop Per Thread。 EventLoop 在构造的时候，
会记录下**“我是属于哪个线程的”**（通过 CurrentThread::tid()）。
如果你在主线程 new EventLoop，那这个 Loop 就认为自己属于主线程。
只有在子线程的函数 (threadFunc) 里创建 Loop，它才会正确地绑定到子线程的 ID 上。

Q2: loop_ = &loop; 这里取栈上变量的地址给别人，不会因为函数结束变量销毁而变成野指针吗？
这也是一个好问题。 在这里是安全的。 因为 loop.loop() 是一个死循环。
只要线程还在运行，threadFunc 函数就不会返回，栈上的 loop 对象就一直活着，地址就一直有效。 
当 loop.loop() 退出时（说明线程要结束了），代码后面马上把 loop_ = nullptr 了，所以主线程也不会拿到一个野指针。
 */