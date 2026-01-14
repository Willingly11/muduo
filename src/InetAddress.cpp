#include <arpa/inet.h>
#include <string>
#include <cstring>

#include "InetAddress.h"


InetAddress::InetAddress(uint16_t port, std::string ip)
{
    addr_.sin_family = AF_INET;
    addr_.sin_port = htons(port); //主机字节序转网络字节序
    if(ip == "0.0.0.0")
    {
        addr_.sin_addr.s_addr = INADDR_ANY; //绑定到所有网卡地址
    } else if(ip == "127.0.0.1")
    {
        addr_.sin_addr.s_addr = INADDR_LOOPBACK; //绑定到回环地址
    } else
    {
        inet_pton(AF_INET, ip.c_str(), &addr_.sin_addr); //点分十进制字符串转网络字节序
    }
}

std::string InetAddress::toIp() const
{
    char buf[INET_ADDRSTRLEN] = "";
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf)); //网络字节序转点分十进制字符串
    return buf;
}

std::string InetAddress::toIpPort() const
{
    char buf[INET_ADDRSTRLEN + 6] = ""; //+6是为了存放端口号
    inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof(buf));
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port); //网络字节序转主机字节序
    snprintf(buf + end, sizeof(buf) - end, ":%u", port);
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port); //网络字节序转主机字节序
}



