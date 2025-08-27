#include "InetAddress.hpp"

#include <string.h>


InetAddress::InetAddress(const std::string& ip, uint16_t port){
    memset(&m_addr, 0, sizeof(m_addr));
    m_addr.sin_family = AF_INET;
    m_addr.sin_port = htons(port);
    m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
}


std::string InetAddress::toIp(){
    char buf[64] = {0};
    inet_ntop(AF_INET, &m_addr.sin_addr.s_addr, buf, 64);
    return buf;
}


uint16_t InetAddress::toPort(){
    return ntohs(m_addr.sin_port);
}


std::string InetAddress::toIpPort(){
    char buf[128];
    snprintf(buf, 128, "%s:%d", toIp().c_str(), toPort());
    return buf;
}

