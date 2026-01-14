#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include <sys/epoll.h>


Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1),
      tied_(false)
{
}

Channel::~Channel()
{
}

void Channel::handleEvent(Timestamp receiveTime)
{
    if (tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if(guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
    
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents:%d\n", revents_);

    // 读
    if ((revents_ & kReadEvent) && readCallback_)
    {
        readCallback_(receiveTime);
    }
    // 写
    if ((revents_ & kWriteEvent) && writeCallback_)
    {
        writeCallback_();
    }
    // 关闭
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN) && closeCallback_)
    {
        closeCallback_();
    }
    // 错误
    if ((revents_ & EPOLLERR) && errorCallback_)
    {
        LOG_ERROR("Channel::handleEvent() EPOLLERR fd:%d", fd_);
        errorCallback_();
    }
}

/* 调用者：TcpConnection::connectEstablished
智能指针类型为 void：一方面是为了泛型指针，另一方面是为了解耦，避免底层 Channel 依赖上层 TcpConnection 类型 */
void Channel::tie(const std::shared_ptr<void>& obj)
{
    tie_ = obj;
    tied_ = true;
}

// Channel -> EventLoop -> Poller
void Channel::update()
{
    // 通过channel所属的eventloop，调用poller的相应方法，注册fd的events事件
    loop_->updateChannel(this); //一个 loop对应多个 channel
}

void Channel::remove()
{
    // 在channel所属的EventLoop中把当前的channel删除掉
    loop_->removeChannel(this);
}

/* 
- Channel (意图): 
管理单个文件描述符（fd）及其感兴趣的事件（events）。
当逻辑层（如 TcpConnection）希望改变监听状态（比如“我要开始读数据”或“我不再关心写事件”）时，
它会修改 Channel 的内部状态，然后调用 Channel::update()。
- EventLoop (中转): 
每个线程一个 EventLoop，它拥有一个 Poller。
Channel 不能直接操作 Poller，必须通过它的 owner（即 loop_）来转发请求。
- Poller (实现): 
它是对 I/O 复用机制（如 epoll 或 poll）的封装。
最终由它调用 epoll_ctl 来真正向操作系统注册或修改事件
 */
