#ifndef _SOCKET_
#define _SOCKET_

#include <unistd.h>

#include "noncopyable.hpp"
#include "InetAddress.hpp"

/*
    对socket的简单封装
    将c对socket的多步操作封装到一个函数中
*/

class Socket: public noncopyable{
public:
    explicit Socket(int fd):m_fd(fd){}
    ~Socket();

    int fd(){return m_fd;}
    void bindAddress(const InetAddress& localAddr);

    void listen();
    int accept(InetAddress* peerAddr);
    void shutDownWrite();

    void setTcpNoDelay(bool on);
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on);

private:
    const int m_fd;
};

#endif