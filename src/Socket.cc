#include "Socket.hpp"

#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/tcp.h>

#include <spdlog/spdlog.h>

Socket::~Socket(){
    close(m_fd);
}

void Socket::bindAddress(const InetAddress& localAddr){
    if(bind(m_fd, (sockaddr*)(localAddr.getSockaddr()), sizeof(sockaddr)) < 0){
        spdlog::critical("Socket::bindAddress fail");
    }
}

void Socket::listen(){
    if(::listen(m_fd, 1024) < 0){
        spdlog::critical("Socket::listen fail");
    }
}

int Socket::accept(InetAddress* peerAddr){
    sockaddr_in addr;
    socklen_t len = sizeof addr;
    memset(&addr, 0, sizeof(sockaddr_in));
    int connfd = ::accept(m_fd, (sockaddr*)&addr, &len);
    if(connfd >= 0){
        peerAddr->setSockaddr(addr);
    }
    return connfd;
}

void Socket::shutDownWrite(){
    if(shutdown(m_fd, SHUT_WR) < 0){
        spdlog::error("Socket::shutDownWrite");
    }
}

void Socket::setTcpNoDelay(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(m_fd, IPPROTO_TCP, TCP_NODELAY, &optval, static_cast<socklen_t>(sizeof(int)));
}

void Socket::setReuseAddr(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &optval, static_cast<socklen_t>(sizeof(int)));
}

void Socket::setReusePort(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(m_fd, SOL_SOCKET, SO_REUSEPORT, &optval, static_cast<socklen_t>(sizeof(int)));
}

void Socket::setKeepAlive(bool on){
    int optval = on ? 1 : 0;
    ::setsockopt(m_fd, SOL_SOCKET, SO_KEEPALIVE, &optval, static_cast<socklen_t>(sizeof(int)));
}