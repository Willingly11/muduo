#pragma once
#include <string>
#include <map>
#include <memory>

#include "Callbacks.h"
#include "Thread.h"
#include "noncopyable.h"
#include "EventLoopThreadPool.h"

class EventLoop;
class Acceptor;
class TcpConnection;
class InetAddress;


class TcpServer : noncopyable
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    TcpServer(EventLoop* loop,
              const InetAddress& listenAddr,
              const std::string& nameArg,
              bool reusePort=false);
    ~TcpServer();

    void start();
    
    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
    void setThreadInitCallback(const ThreadInitCallback& cb) { threadInitCallback_ = cb; }

    void setThreadNum(int numThreads) { numThreads_ = numThreads; threadPool_->setThreadNum(numThreads); }

private:
    void newConnection(int sockfd, const InetAddress& peerAddr);
    void removeConnection(const TcpConnectionPtr& conn);
    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    EventLoop* loop_;
    std::string ipPort_;
    std::string name_;
    bool reusePort_;
    std::unique_ptr<Acceptor> acceptor_; //运行在主线程, 负责接受新连接
    std::map<std::string, TcpConnectionPtr> connections_;


    std::unique_ptr<EventLoopThreadPool> threadPool_; //EventLoop 线程池

    ConnectionCallback connectionCallback_; //有新连接时的回调
    MessageCallback messageCallback_; //有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_;

    ThreadInitCallback threadInitCallback_; //线程初始化回调
    int numThreads_; //EventLoop 线程数量
    int nextConnId_; //下一个连接的序号
    std::atomic<int> started_; //是否启动服务器
};