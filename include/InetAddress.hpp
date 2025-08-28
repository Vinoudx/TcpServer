#ifndef _INETADDRESS_
#define _INETADDRESS_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>


class InetAddress{
public:
    InetAddress() = default;
    explicit InetAddress(const std::string& ip, uint16_t port);
    explicit InetAddress(struct sockaddr_in& addr):m_addr(addr){}

    std::string toIp() const;
    uint16_t toPort() const;
    std::string toIpPort() const;

    const sockaddr_in* getSockaddr() const {return &m_addr;}
    void setSockaddr(const sockaddr_in& addr){m_addr = addr;}

private:
    struct sockaddr_in m_addr;
};

#endif