#include <unistd.h>
#include <netinet/tcp.h>

#include "Socket.h"
#include "Logger.h"
#


Socket::~Socket()
{
    ::close(sockfd_);
}

void Socket::bindAddress(const InetAddress& localaddr)
{
    int ret = ::bind(sockfd_, (sockaddr*)(&localaddr.getSockAddrInet()), sizeof(sockaddr_in));
    if(ret < 0)
    {
        LOG_FATAL("Socket::bindAddress error %d\n", errno);
    }
}

void Socket::listen()
{
    int ret = ::listen(sockfd_, SOMAXCONN);
    if(ret < 0)
    {
        LOG_FATAL("Socket::listen error %d\n", errno);
    }
}

int Socket::accept(InetAddress& peeraddr)
{
    sockaddr_in addr;
    socklen_t addrlen = sizeof(addr);
    int connfd = ::accept4(sockfd_, (sockaddr*)(&addr), &addrlen, SOCK_NONBLOCK | SOCK_CLOEXEC);
    if(connfd < 0)
    {
        LOG_ERROR("Socket::accept error %d\n", errno);
        return -1;
    }
    peeraddr.setSockAddrInet(addr);
    return connfd;
}

void Socket::shutdownWrite()
{
    if(::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR("Socket::shutdownWrite error %d\n", errno);
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof(optval));
}
void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
}
void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}
void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof(optval));
}
