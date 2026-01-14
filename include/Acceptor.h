#pragma once
#include "InetAddress.h"
#include "EventLoop.h"
#include "Socket.h"
#include "Logger.h"
#include "noncopyable.h"
#include "Channel.h"

class Acceptor : noncopyable
{
public:
    using NewConnectionCallback = std::function<void(int sockfd, const InetAddress&)>;

    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    void listen();
    bool listening() const { return listening_; }
    void setNewConnectionCallback(const NewConnectionCallback& cb) { newConnectionCallback_ = cb; }
    
private:
    void handleRead(); //处理新用户的连接事件

    EventLoop* loop_; //Acceptor属于某个EventLoop
    Socket acceptSocket_; //用于监听的socket
    std::unique_ptr<Channel> acceptChannel_; 
    bool listening_;
    NewConnectionCallback newConnectionCallback_; //有新连接到来时的回调函数
};

//如何理解这里的回调关系
//1. TcpServer中有一个Acceptor对象 acceptor_
//2. TcpServer::start()->Acceptor.listen()->acceptChannel_.enableReading() => 让acceptChannel_注册到Poller中监听listenfd的读事件
//3. 当有新用户连接时 mainloop监听到listenfd的读事件发生了 => 调用acceptChannel_.handleEvent => 调用ReadCallback

//acceptChannel_.setReadCallback->Acceptor::handleRead->acceptSocket_.accept->NewConnectionCallback_(connfd, peerAddr)

/* 回调函数：
我这里需要处理的任务我不知道，因为我将要处理的逻辑丢给上层来实现，只有上层才知道我接下来要去干什么 */
