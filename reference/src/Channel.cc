#include <sys/epoll.h>

#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"

const int Channel::kNoneEvent = 0; //空事件
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI; //读事件、优先级读事件（带外数据）
const int Channel::kWriteEvent = EPOLLOUT; //写事件

// EventLoop: ChannelList Poller
Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1)
    , tied_(false)
{
}

Channel::~Channel()
{
}

// channel的tie方法什么时候调用过?  TcpConnection => channel
/**
 * TcpConnection中注册了Channel对应的回调函数，传入的回调函数均为TcpConnection
 * 对象的成员方法，因此可以说明一点就是：Channel的结束一定晚于TcpConnection对象！
 * 此处用tie去解决TcpConnection和Channel的生命周期时长问题，从而保证了Channel对象能够在
 * TcpConnection销毁前销毁。
 **/
//  为什么要设计tied_这个标志位？
// 因为在 muduo 中，并不是所有的 Channel 都面临“突然暴毙”的风险。
// 情况 A：必须开开关 (tied_ = true)
// 对象：TcpConnection 拥有的 Channel。
// 原因：TcpConnection 是动态创建和销毁的。客户端随时可能断开连接，导致 TcpConnection 被析构。如果不加保护，回调执行到一半对象没了，程序就崩了。
// 操作：所以在 TcpConnection::connectEstablished 里，会主动调用 channel_->tie(shared_from_this())，把 tied_ 设为 true。
// 情况 B：不需要开开关 (tied_ = false)
// 对象：Acceptor 的 Channel (监听连接) 或 EventLoop 的 wakeupChannel_ (唤醒)。
// 原因：
// Acceptor 通常是作为 TcpServer 的成员变量，它的生命周期跟 Server 一样长，不会在运行中突然消失。
// wakeupChannel_ 是 EventLoop 的成员，跟 Loop 共存亡。
// 对于这些长寿对象，去执行 weak_ptr::lock() 这种操作是多余的，虽然开销不大，但能省则省。
// 操作：这些类在使用 Channel 时，不调用 tie() 函数。tied_ 默认为 false
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}
//update 和remove => EpollPoller 更新channel在poller中的状态
/**
 * 当改变channel所表示的fd的events事件后，update负责再poller里面更改fd相应的事件epoll_ctl
 **/
void Channel::update()
{
    // 通过channel所属的eventloop，调用poller的相应方法，注册fd的events事件
    loop_->updateChannel(this);
}

// 在channel所属的EventLoop中把当前的channel删除掉
void Channel::remove()
{
    loop_->removeChannel(this);
}

void Channel::handleEvent(Timestamp receiveTime)
{
    if (tied_)
    {
        std::shared_ptr<void> guard = tie_.lock();
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
        // 如果提升失败了 就不做任何处理 说明Channel的TcpConnection对象已经不存在了
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents:%d\n", revents_);
    // 关闭：// 当TcpConnection对应Channel 通过shutdown 关闭写端 epoll触发EPOLLHUP，同时防止丢失数据
    // 优先级：读数据 (Read) > 处理关闭 (Close)
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)) 
    {
        if (closeCallback_)
        {
            closeCallback_();
        }
    }
    // 错误
    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
    }
    // 读
    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCallback_)
        {
            readCallback_(receiveTime);
        }
    }
    // 写
    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
}