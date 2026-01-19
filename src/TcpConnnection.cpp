#include "Callbacks.h"
#include "TcpConnection.h"
#include "Timestamp.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "Buffer.h"
#include "EventLoop.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>

TcpConnection::TcpConnection(EventLoop* loop,
                             int sockfd,
                             const std::string& name,
                             const InetAddress& localAddr,
                             const InetAddress& peerAddr)
    : state_(StateE::kConnecting),
      name_(name),
      localAddr_(localAddr),
      peerAddr_(peerAddr),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      inputBuffer_(new Buffer()),
      outputBuffer_(new Buffer()),
      highWaterMark_(64 * 1024 * 1024) //默认高水位 64MB
{
    channel_->setReadCallback(
        [this](Timestamp receiveTime) { this->handleRead(receiveTime); }
    );
    channel_->setWriteCallback(
        [this]() { this->handleWrite(); }
    );
    channel_->setCloseCallback(
        [this]() { this->handleClose(); }
    );
    channel_->setErrorCallback(
        [this]() { this->handleError(); }
    ); 

    socket_->setKeepAlive(true);
}

TcpConnection::~TcpConnection()
{
    LOG_INFO("TcpConnection::~TcpConnection [%s] destructed", name_.c_str());
}

void TcpConnection::send(const std::string& message)
{
    if(state_ == StateE::kConnected)
    {
        if(loop_->isinLoopThread())
        {
            sendInLoop(message);
        }
        else
        {
            loop_->runInLoop(
                [this, message]() { this->sendInLoop(message); }
            );
        }
    }
}

void TcpConnection::sendInLoop(const std::string& message)
{
    ssize_t nwrote = 0;
    size_t remaining = message.size();
    bool faultError = false;

    if(state_ == StateE::kDisconnected)
    {
        LOG_ERROR("TcpConnection::sendInLoop() - Disconnected, give up writing");
        return;
    }

    //如果没有正在写且输出缓冲区为空, 则尝试直接写入
    if(!channel_->isWriting() && outputBuffer_->readableBytes() == 0)
    {
        nwrote = ::write(channel_->fd(), message.data(), message.size());
        if(nwrote >= 0)
        {
            remaining = message.size() - nwrote;
            if(remaining == 0 && writeCompleteCallback_)
            {
                loop_->queueInLoop(
                    [this]() { writeCompleteCallback_(shared_from_this()); }
                );
            }
        }
        else 
        {
            nwrote = 0;
            if(errno != EWOULDBLOCK)
            {
                LOG_ERROR("TcpConnection::sendInLoop() - write error");
                if(errno == EPIPE || errno == ECONNRESET) //SIGPIPE or ECONNRESET
                {
                    faultError = true;
                }
            }
        }

        if(!faultError && remaining > 0)
        {
            size_t oldlen = outputBuffer_->readableBytes();
            if(oldlen + remaining >= highWaterMark_ && 
               oldlen < highWaterMark_ &&
               highWaterMarkCallback_)
            {
                int len = oldlen + remaining;
                loop_->queueInLoop(
                    [this, len]() 
                    { highWaterMarkCallback_(shared_from_this(), len); }
                );
            }
            outputBuffer_->append(message.data() + nwrote, remaining);
            if(!channel_->isWriting())
            {
                channel_->enableWriting();
            }
        }
    }

}

void TcpConnection::shutdown()
{
    if(state_ == StateE::kConnected)
    {
        state_ = StateE::kConnected;
        loop_->runInLoop(
            [this]() { this->shutdownInLoop(); }
        );
    }
}

void TcpConnection::shutdownInLoop()
{
    if(!channel_->isWriting()) //如果没有正在写, 直接关闭写端
    {
        socket_->shutdownWrite();
    }
}

void TcpConnection::connectionEstablished()
{
    state_ = StateE::kConnected;
    channel_->enableReading(); //关注读事件

    connectionCallback_(shared_from_this());
}

void TcpConnection::connectionDestroyed()
{
    if(state_ == StateE::kConnected)
    {
        state_ = StateE::kDisconnected;
        channel_->disableAll(); //取消所有事件的关注

        connectionCallback_(shared_from_this());
    }
    channel_->remove(); //从 Poller 中删除该 channel
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedError = 0;
    ssize_t n = inputBuffer_->readFd(channel_->fd(), &savedError);
    if(n > 0)
    {
        messageCallback_(shared_from_this(), inputBuffer_.get(), receiveTime);
    }
    else if(n == 0)
    {
        handleClose();
    }
    else
    {
        errno = savedError;
        LOG_ERROR("TcpConnection::handleRead() - read error");
        handleError();
    }
}

void TcpConnection::handleWrite()
{
    if(channel_->isWriting())
    {
        ssize_t n = ::write(channel_->fd(),outputBuffer_->peek(), outputBuffer_->readableBytes());
        if(n > 0)
        {
            outputBuffer_->retrieve(n);
            if(outputBuffer_->readableBytes() == 0)
            {
                channel_->disableWriting(); //写完了, 取消关注写事件,否则会一直触发写事件
                if(writeCompleteCallback_)
                {
                    loop_->queueInLoop(
                        [this]() { writeCompleteCallback_(shared_from_this()); }
                    );
                }
                if(state_ == StateE::kDisconnecting)
                {
                    shutdownInLoop(); //如果正在断开连接, 则调用 shutdownInLoop, 配合之前的 shutdown 调用
                }

            }
        }
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("TcpConnection::handleClose() - connection [%s] closed", name_.c_str());
    // state_ = StateE::kDisconnected; //这里是否要修改状态?
    channel_->disableAll();

    TcpConnectionPtr guardThis(shared_from_this()); //
    closeCallback_(guardThis);
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if (::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }
    else
    {
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d", name_.c_str(), err);
}






