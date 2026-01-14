#include <sys/epoll.h>
#include <unistd.h>

#include "EpollPoller.h"
#include "Channel.h"
#include "Logger.h"



// Channel在epoll中的index状态常量定义
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2; //未添加到epoll中，但在channels_中

EpollPoller::EpollPoller(EventLoop* loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize)
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("EpollPoller::EpollPoller - epoll_create1 error");
    }
}

EpollPoller::~EpollPoller()
{
    ::close(epollfd_);
}

Timestamp EpollPoller::poll(int timeoutMs, ChannelList* activeChannels)
{
    int numEvents = ::epoll_wait(epollfd_,
                                 &*events_.begin(),
                                 static_cast<int>(events_.size()),
                                 timeoutMs);
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        fillActiveChannels(numEvents, activeChannels); // 填充活跃的channel通道
        if (static_cast<size_t>(numEvents) == events_.size()) //如果满了，扩容
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG("EpollPoller::poll - timeout");
    }
    else
    {
        LOG_ERROR("EpollPoller::poll - epoll_wait error");
    }
    return now;
}

void EpollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const
{
    for(int i = 0; i < numEvents; i++)
    {
        auto channnel = static_cast<Channel*>(events_[i].data.ptr);
        channnel->set_revents(events_[i].events);
        activeChannels->push_back(channnel);
    }
}

void EpollPoller::updateChannel(Channel* channel)
{
    const int index = channel->index();
    LOG_DEBUG("EpollPoller::updateChannel - fd=%d events=%d index=%d", channel->fd(), channel->events(), index);

    if(index == kNew || index == kDeleted)
    {
        int fd = channel->fd();
        if(index == kNew)
        {
            channels_[fd] = channel;
        }
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else //index == kAdded
    {
        if(channel->isNoneEvent())
        {
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EpollPoller::update(int operation, Channel* channel)
{
    struct epoll_event event;
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    if(::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if(operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("EpollPoller::update - epoll_ctl DEL error fd=%d", fd);
        }
        else
        {
            LOG_FATAL("EpollPoller::update - epoll_ctl ADD/MOD error fd=%d", fd);
        }
    }
}

void EpollPoller::removeChannel(Channel* channel)
{
    int fd = channel->fd();
    LOG_DEBUG("EpollPoller::removeChannel - fd=%d", fd);

    channels_.erase(fd);

    int index = channel->index();
    if(index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}