#pragma once
#include <string>
#include <memory>

#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"

class EventLoop;
class Socket;
class Channel;
class Buffer;
class Timestamp;


class TcpConnection : public std::enable_shared_from_this<TcpConnection>, public noncopyable
{
public:
    TcpConnection(EventLoop* loop,
                  int sockfd,
                  const std::string& name,
                  const InetAddress& localAddr,
                  const InetAddress& peerAddr);
    ~TcpConnection();
    
    void send(const std::string& message);
    void shutdown();

    void connectionEstablished();
    void connectionDestroyed();

    EventLoop* getLoop() const { return loop_; }
    const std::string& name() const { return name_; }
    const InetAddress& localAddress() const { return localAddr_; }
    const InetAddress& peerAddress() const { return peerAddr_; }

    bool connected() const { return state_ == StateE::kConnected; }


    void setConnectionCallback(const ConnectionCallback& cb) { connectionCallback_ = cb; }
    void setMessageCallback(const MessageCallback& cb) { messageCallback_ = cb; }
    void setCloseCallback(const CloseCallback& cb) { closeCallback_ = cb; }
    void setWriteCompleteCallback(const WriteCompleteCallback& cb) { writeCompleteCallback_ = cb; }
    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark) 
    { 
        highWaterMarkCallback_ = cb; 
        highWaterMark_ = highWaterMark;
    }

private:
    void sendInLoop(const std::string& message);
    void shutdownInLoop();

    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    enum class StateE 
    { 
        kDisconnected, 
        kConnecting, 
        kConnected, 
        kDisconnecting //连接正在断开, shutdown 后变为此状态
    };
    StateE state_;  //连接状态

    std::string name_; //连接名称
    InetAddress localAddr_;   //本地地址
    InetAddress peerAddr_;    //对端地址

    EventLoop* loop_; 
    std::unique_ptr<Socket> socket_; //连接对应的 socket
    std::unique_ptr<Channel> channel_; //连接对应的 channel

    std::unique_ptr<Buffer> inputBuffer_;  //读缓冲区
    std::unique_ptr<Buffer> outputBuffer_; //写缓冲区

    ConnectionCallback connectionCallback_; //连接建立或断开时的回调
    MessageCallback messageCallback_;      //有消息到达时的回调
    CloseCallback closeCallback_;          //连接关闭时的回调
    WriteCompleteCallback writeCompleteCallback_; //数据发送完成时的回调
    HighWaterMarkCallback highWaterMarkCallback_; //高水位回调
    size_t  highWaterMark_; //高水位值
};
