#include "Acceptor.h"

static int createNonblocking()
{
    int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
    if (sockfd < 0)
    {
        LOG_FATAL("%s:%s:%d socket create err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
    }
    return sockfd;
}

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport)
    : loop_(loop),
      acceptSocket_(createNonblocking()),
      acceptChannel_(new Channel(loop, acceptSocket_.fd())),
      listening_(false)
{
    acceptSocket_.setReuseAddr(true);
    if(reuseport)
    {
        acceptSocket_.setReusePort(true);
    }
    acceptSocket_.bindAddress(listenAddr);

    // TcpServer::start() => Acceptor.listen() 如果有新用户连接 要执行一个回调(accept => connfd => 打包成Channel => 唤醒subloop)
    // mainLoop => acceptChannel_ => handleRead => newConnectionCallback_(connfd, peeraddr)
    acceptChannel_->setReadCallback([this](Timestamp) { this->handleRead(); } //设置有新连接到来时的回调函数
    );
}

Acceptor::~Acceptor()
{
    acceptChannel_->disableAll(); //从epoll中移除acceptChannel_
    acceptChannel_->remove(); //从channelmap中移除acceptChannel_
}

void Acceptor::listen()
{
    listening_ = true;
    acceptSocket_.listen(); //开始监听端口
    acceptChannel_->enableReading(); //注册acceptChannel_的读事件到epoll中 监听新用户连接事件
}

void Acceptor::handleRead()
{
    InetAddress peeraddr(0);
    int connfd = acceptSocket_.accept(peeraddr); //接受新连接,返回connfd和peeraddr
    if(connfd >= 0)
    {
        if(newConnectionCallback_) //有新连接到来时的回调函数
        {
            newConnectionCallback_(connfd, peeraddr); //轮询分配一个subloop，唤醒subloop处理新用户连接
        }
        else
        {
            ::close(connfd); //没有设置回调函数 关闭连接
        }
    }
    else
    {
        LOG_ERROR("%s:%s:%d accept err:%d\n", __FILE__, __FUNCTION__, __LINE__, errno);
        if (errno == EMFILE)
        {
            LOG_ERROR("%s:%s:%d sockfd reached limit\n", __FILE__, __FUNCTION__, __LINE__);
        }
    }
}