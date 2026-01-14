#pragma once
#include "InetAddress.h"
#include "noncopyable.h"
class Socket: noncopyable
{
public:
    explicit Socket(int sockfd) : sockfd_(sockfd) {}
    ~Socket();
    int fd() const { return sockfd_; }

    void bindAddress(const InetAddress& localaddr); //绑定地址
    void listen(); //监听端口
    int accept(InetAddress& peeraddr); //接受连接
    void shutdownWrite(); //关闭写端

    void setTcpNoDelay(bool on); //设置TCP_NODELAY选项
    void setReuseAddr(bool on); //设置地址复用选项
    void setReusePort(bool on); //设置端口复用选项
    void setKeepAlive(bool on); //设置保持连接选项

private:
    const int sockfd_;
};