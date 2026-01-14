#include <mutex>
#include <stdexcept>
#include <sys/eventfd.h>

#include "CurrentThread.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Timestamp.h"

const int kPollTimeMs = 10000; //默认的Poller IO复用接口的超时时间：10s

static int createEventfd()
{
    // 创建一个eventfd 用于线程间唤醒
    int fd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(fd < 0)
    {
        LOG_FATAL("createEventfd - eventfd error"); 
    }
    return fd;
}

EventLoop::EventLoop()
    : threadId_(CurrentThread::tid()),
      poller_(Poller::newDefaultPoller(this)),
      wakeupFd_(createEventfd()),
      wakeupChannel_(new Channel(this, wakeupFd_)),
      looping_(false),
      quit_(false),
      callingPendingFunctors_(false)
{
    LOG_DEBUG("EventLoop created %p in thread %d\n", this, threadId_);

    wakeupChannel_->setReadCallback([this](Timestamp) { this->handleRead(); }); //handleRead用于从wakeupFd_读取数据
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll(); //从epoll中移除wakeupFd_
    wakeupChannel_->remove(); //从channelmap中移除wakeupChannel_
    ::close(wakeupFd_);
}

void EventLoop::loop()
{
    //事件循环的核心函数

    // LOG_DEBUG("EventLoop %p start looping\n", this);
    looping_ = true;
    quit_ = false;

    while(!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_); //阻塞等待IO事件发生

        for(Channel* channel : activeChannels_)
        {
            channel->handleEvent(pollReturnTime_); //调用channel的回调函数
        }

        doPendingFunctors(); //执行当前loop需要执行的回调操作
    }
    looping_ = false;
}

void EventLoop::quit()
{
    LOG_DEBUG("EventLoop %p stop looping\n", this);

    // 设置退出标志，结束事件循环
    quit_ = true;

    if(!isinLoopThread())
    {
        wakeup();
    }
}

void EventLoop::doPendingFunctors()
{
    // 执行回调函数
    std::vector<functor> functors;
    callingPendingFunctors_ = true; ////配合queueInLoop使用
    {
        std::lock_guard<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_); 
    }

    for(const auto& func : functors)
    {
        func();
    }

    callingPendingFunctors_ = false;
}

void EventLoop::runInLoop(functor cb)
{
    // 在当前loop线程执行回调函数cb
    if(isinLoopThread())
    {
        cb();
    }
    else
    {
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(functor cb)
{
    // 把回调函数添加到队列中，稍后执行
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(std::move(cb));
    }
    if(isinLoopThread() == false || callingPendingFunctors_)
    {
        wakeup();
    }
}



void EventLoop::wakeup()
{
    // 写一个64位的整数到wakeupFd_，唤醒阻塞在poller上的线程
    uint64_t one = 1;
    ssize_t n = ::write(wakeupFd_, &one, sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %ld bytes instead of 8", n);
    }
}

void EventLoop::handleRead()
{
    // 与wakeup对应，从wakeupFd_读取64位整数，清除事件
    uint64_t one = 1;
    ssize_t n = ::read(wakeupFd_, &one, sizeof one);
    if(n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() reads %ld bytes instead of 8", n);
    }
}

void EventLoop::updateChannel(Channel* channel)
{
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel* channel)
{
    poller_->removeChannel(channel);
}
bool EventLoop::hasChannel(Channel* channel)
{
    return poller_->hasChannel(channel);
}   