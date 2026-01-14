#pragma once

#include <functional>
#include <vector>
#include <mutex>

#include "Channel.h"
#include "Poller.h"
#include "Timestamp.h"
#include "noncopyable.h"
#include "CurrentThread.h"

class EventLoop : noncopyable
{
public:
    using functor = std::function<void()>;

    EventLoop();
    ~EventLoop();   

    void loop();
    void quit();

    void runInLoop(functor cb);
    void queueInLoop(functor cb);

    void wakeup();
    void handleRead();
    

    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    bool isinLoopThread() const { return threadId_ == CurrentThread::tid(); }

private:
    void doPendingFunctors();

    const pid_t threadId_; // 记录创建该EventLoop的线程id

    std::unique_ptr<Poller> poller_;

    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;

    bool looping_;
    bool quit_;

    bool callingPendingFunctors_; // 标志当前是否有回调函数正在被调用
    std::vector<functor> pendingFunctors_; // 存储待执行的回调函数
    std::mutex mutex_; // 保护pendingFunctors_

    using ChannelList = std::vector<Channel*>;
    ChannelList activeChannels_; 

    
    
    

    Timestamp pollReturnTime_;
};
