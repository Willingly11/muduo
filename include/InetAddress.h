#pragma once

#include <string>
#include <netinet/in.h>



class InetAddress {
public:
    explicit InetAddress(uint16_t port = 0, std::string ip = "0.0.0.0");
    explicit InetAddress(const sockaddr_in& addr)
        : addr_(addr)
    {}
    std::string toIp() const;
    std::string toIpPort() const;
    uint16_t toPort() const;
    const struct sockaddr_in& getSockAddrInet() const { return addr_; }
    void setSockAddrInet(const struct sockaddr_in& addr) { addr_ = addr; }
private:
    struct sockaddr_in addr_;
};

//默认IP = ip = "0.0.0.0"
/*
- 含义: 
表示绑定到本机所有可用的网络接口（网卡）。
- 效果: 
你的服务将监听来自本机所有 IP 地址的连接。
外部机器可以通过你服务器的公网 IP 或局域网 IP 访问你的服务，
本机客户端也可以通过 localhost 访问。
- 用途: 
通常作为服务器程序的默认值，以便对外提供服务。
 */

 //默认IP = ip = "127.0.0.1"
/*
- 含义:
表示绑定到本机的回环接口（Loopback Interface）。
- 效果:
你的服务只监听来自本机的连接请求。
外部机器无法通过网络访问你的服务。
本机客户端可以通过 localhost 访问。
- 用途:
通常用于开发和测试环境，确保服务仅在本机可见，避免外部访问。
 */