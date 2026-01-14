#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop* loop);
    virtual ~Poller() = default;

    //纯虚函数
    virtual Timestamp poll(int timeoutMs, ChannelList* activeChannels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;

    bool hasChannel(Channel* channel) const; 

    // 在DefaultPoller.cpp中实现:为了避免循环依赖
    // 将该函数声明放在这里，实现放在DefaultPoller.cpp中，而不是Poller.cpp中
    static Poller* newDefaultPoller(EventLoop* loop);//获取默认的IO复用具体实现

protected:
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_; //key:sockfd value:sockfd所属的channel通道类型

private:
    EventLoop* ownerLoop_; // 定义Poller所属的事件循环EventLoop
};