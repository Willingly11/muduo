#include <cstring>

#include "TcpServer.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include "InetAddress.h"
#include "EventLoopThreadPool.h"
#include "Logger.h"
#include <netinet/in.h>

TcpServer::TcpServer(EventLoop* loop,
                     const InetAddress& listenAddr,
                     const std::string& nameArg,
                     bool reusePort)
    : loop_(loop),
      ipPort_(listenAddr.toIpPort()),
      name_(nameArg),
      acceptor_(new Acceptor(loop, listenAddr, reusePort)),
      threadPool_(new EventLoopThreadPool(loop, name_)),
      numThreads_(0),
      nextConnId_(1),
      started_(0)
{

    acceptor_->setNewConnectionCallback(
        [this](int sockfd, const InetAddress& peerAddr)
        { this->newConnection(sockfd, peerAddr); }
    );
}

TcpServer::~TcpServer()
{
    for(auto &item : connections_)
    {
        TcpConnectionPtr conn(item.second);
        item.second.reset();    // 把原始的智能指针复位 让栈空间的TcpConnectionPtr conn指向该对象 当conn出了其作用域 即可释放智能指针指向的对象
        
        conn->getLoop()->runInLoop(
            [conn]() { conn->connectionDestroyed(); }
        );
    }
}
/*
- 注意点：
这里依赖 EventLoopThreadPool 的析构顺序。threadPool_ 必须在 connections_ 清理之后才析构。
在 C++ 类中，成员变量按声明顺序构造，按相反顺序析构。
请检查 TcpServer.h 中的声明顺序，必须确保 threadPool_ 定义在 connections_ map 的下面
（或者你期望线程池最后死，那其实无所谓，关键是 Loop 不能在 Task 执行前就消失）。 
 */

void TcpServer::start()
{
    if(started_.fetch_add(1) == 0)
    {
        threadPool_->start(threadInitCallback_);
        loop_->runInLoop(
            [this]() { acceptor_->listen(); }
        );
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr)
{
    EventLoop* ioLoop = threadPool_->getNextLoop();
    char buf[32];
    snprintf(buf, sizeof buf, "-%s#%d", ipPort_.c_str(), nextConnId_);
    ++nextConnId_;
    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s\n",
             name_.c_str(),
             connName.c_str(),
             peerAddr.toIpPort().c_str()); 
    
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    if (::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0)
    {
        LOG_ERROR("TcpServer::newConnection getsockname error");
    }

    InetAddress localAddr(local);
    // 使用 make_shared 优化内存分配 (一次分配 vs 两次分配)
    TcpConnectionPtr conn = std::make_shared<TcpConnection>(
                                ioLoop,
                                sockfd,
                                connName,
                                localAddr,
                                peerAddr);

    connections_[connName] = conn;
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);
    conn->setCloseCallback(
        [this](const TcpConnectionPtr& conn) { this->removeConnection(conn); }
    );

    ioLoop->runInLoop(
        [conn]() { conn->connectionEstablished(); }
    );
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn)
{
    loop_->runInLoop(
        [this, conn]() { this->removeConnectionInLoop(conn); }
    );
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn)
{
    LOG_INFO("TcpServer::removeConnectionInLoop [%s] - connection %s\n",
             name_.c_str(),
             conn->name().c_str());

    connections_.erase(conn->name());

    EventLoop* ioLoop = conn->getLoop();
    ioLoop->queueInLoop(
        [conn]() { conn->connectionDestroyed(); }
    );
}
