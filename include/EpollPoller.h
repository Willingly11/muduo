#pragma once

#include <vector>
#include <sys/epoll.h>

#include "Poller.h"
#include "Channel.h"
#include "Timestamp.h"


class EpollPoller : public Poller
{
public:
    EpollPoller(EventLoop* loop);
    ~EpollPoller() override;

    Timestamp poll(int timeoutMs, ChannelList* activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;
private:
    static const int kInitEventListSize = 16;
    
    void fillActiveChannels(int numEvents, ChannelList* activeChannels) const;
    void update(int operation, Channel* channel);

    int epollfd_;
    std::vector<epoll_event> events_;
    /*struct epoll_event {
        uint32_t events; // Epoll events
        epoll_data_t data; // User data variable
    };*/  
};