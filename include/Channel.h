#pragma once

#include <functional>
#include <memory>
#include <sys/epoll.h>
#include "Timestamp.h"
#include "noncopyable.h"

class EventLoop;

class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop* loop, int fd);
    ~Channel();

    void handleEvent(Timestamp receiveTime); // 处理事件


    EventLoop* ownerLoop() { return loop_; }
    int fd() const { return fd_; }
    int events() const { return events_; }
    void set_revents(int revt) { revents_ = revt; }

    void enableReading() { events_ |= kReadEvent; update(); }
    void disableReading() { events_ &= ~kReadEvent; update(); }
    void enableWriting() { events_ |= kWriteEvent; update(); }
    void disableWriting() { events_ &= ~kWriteEvent; update(); }
    void disableAll() { events_ = kNoneEvent; update(); }  

    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    void tie(const std::shared_ptr<void>& obj);
    
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }
    
    void remove(); //作用：将 Channel 从 EventLoop 中彻底移除（不再监听该 fd）

private:
    void handleEventWithGuard(Timestamp receiveTime);
    void update();

    static constexpr int kNoneEvent = 0;
    static constexpr int kReadEvent = EPOLLIN | EPOLLPRI; //EPOLLRPI：带外数据
    static constexpr int kWriteEvent = EPOLLOUT;

    EventLoop *loop_; //所属的EventLoop
    int fd_;
    int events_; //注册感兴趣的事件
    int revents_; //poller返回的具体发生的事件
    int index_; //kNew(-1), kAdded(1), kDeleted(2) 用于标识channel在Poller中的状态

    bool tied_;
    std::weak_ptr<void> tie_; //指向拥有这个 Channel 的上层对象（即 TcpConnection）,避免循环引用

    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;


};

/* TcpServer
├── loop_ (主 Reactor)
├── acceptor_ (负责监听)
│   └── acceptChannel_ (Channel 对象，负责 listenfd)
│
└── connectionMap_ (存活的连接列表)
    ├── TcpConnection A
    │   └── channel_ (Channel 对象，负责 connfd A) ——> 绑定到 subLoop 1
    ├── TcpConnection B
    │   └── channel_ (Channel 对象，负责 connfd B) ——> 绑定到 subLoop 2
    └── ...

总结：Channel 是 TcpConnection 的私有财产，但它被派遣到 EventLoop 里去干活
channel由TcpConnection创建和拥有 但channel要被注册到某个EventLoop中去监听它所表示的fd的事件 */



//tied_ 与 tie_ 的作用（重点）
/**这两个成员变量是 Muduo 网络库中非常经典的设计细节，
用于解决 对象生命周期管理 中的竞态问题（Race Condition）。

简单来说：这两个变量是为了防止 Channel 在执行回调函数时，
它所属的上层对象（通常是 TcpConnection）被意外销毁，导致程序崩溃。

    1. 变量含义
    - std::weak_ptr<void> tie_
    这是一个弱智能指针。
    它通常指向拥有这个 Channel 的上层对象（即 TcpConnection）。
    使用 weak_ptr 的原因是：Channel 是 TcpConnection 的成员，TcpConnection 拥有 Channel（强引用）。
    如果 Channel 反过来也持有 TcpConnection 的 shared_ptr，就会造成循环引用，
    导致内存永远无法释放。所以这里必须用弱引用“观察”上层对象。

    - bool tied_
    标志位。用来表示 tie_ 是否绑定了对象。
    默认为 false，当调用 Channel::tie() 后变为 true。

    2. 它们解决什么问题？（核心场景）
    假设没有这个保护机制，会发生以下恐怖故事：

    场景：TcpConnection 包含一个 Channel。
    触发：网络上有消息来了，Poller 通知 Channel。
    执行：Channel::handleEvent() 开始执行。
    回调：Channel 调用用户注册的回调函数（比如 ReadCallback）。
    意外：在用户的回调函数代码中，可能因为解析出错或者收到某些特定的关闭指令，
        用户决定调用 connection->handleClose()，进而导致 TcpConnection 被析构。
        崩溃：TcpConnection 析构时，连带着销毁了成员变量 Channel。
        回马枪：但是！此时代码还停留在第 4 步的函数栈中。当回调函数结束，
        返回到 Channel::handleEvent() 继续往下执行时，Channel 对象本身已经被销毁了。
        此时再访问 Channel 的成员变量（如 writeCallback_ 或 revents_），
        就是非法访问（Use-after-free），程序直接 Segfault 崩溃。 
*/

