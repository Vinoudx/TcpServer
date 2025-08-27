#ifndef _INETADDRESS_
#define _INETADDRESS_

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string>


class InetAddress{
public:
    explicit InetAddress(const std::string& ip, uint16_t port);
    explicit InetAddress(struct sockaddr_in& addr):m_addr(addr){}

    std::string toIp();
    uint16_t toPort();
    std::string toIpPort();

    const sockaddr_in* getSockaddr() const {return &m_addr;}

private:
    struct sockaddr_in m_addr;
};

#endif