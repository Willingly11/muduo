#pragma once

#include <functional>
#include <mutex>
#include <condition_variable>
#include <string>

#include "noncopyable.h"
#include "Thread.h"

class EventLoop;

class EventLoopThread : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
                    const std::string &name = std::string()); //ThreadInitCallback()为临时的空函数对象，可以用{}或者nullptr代替
    ~EventLoopThread();

    EventLoop *startLoop();

private:
    void threadFunc();

    EventLoop *loop_;
    bool exiting_; // 标志位 线程退出时置为true
    Thread thread_;
    std::mutex mutex_;             // 互斥锁
    std::condition_variable cond_; // 条件变量
    ThreadInitCallback callback_;
};