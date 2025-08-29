#include "Acceptor.hpp"

#include <sys/socket.h>

#include <spdlog/spdlog.h>

Acceptor::Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport):
    m_loop(loop),
    m_acceptSocket(::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP)),
    m_acceptChannel(loop, m_acceptSocket.fd()),
    m_listening(false),
    idleFd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)){
    if(m_acceptSocket.fd() <= 0){
        spdlog::critical("Acceptor::socket fail");
    }
    m_acceptSocket.setReuseAddr(true);
    m_acceptSocket.setReusePort(reuseport);
    m_acceptSocket.bindAddress(listenAddr);
    m_acceptChannel.setReadCallback(std::bind(&Acceptor::handleRead, this));
}

Acceptor::~Acceptor(){
    m_acceptChannel.disableAll();
    m_acceptChannel.remove();
    ::close(idleFd_);
}

void Acceptor::handleRead(){
    spdlog::debug("Acceptor::handleRead");
    InetAddress peeraddr;
    int sockfd = m_acceptSocket.accept(&peeraddr);
    if(sockfd >= 0){
        if(m_cb){
            m_cb(sockfd, peeraddr);
        }else{
            spdlog::debug("no newConnectionCallback");
            // 如果没有创立连接的回调，则直接关闭
            // 这合理吗？或者说这种情况会发生吗?
            ::close(sockfd);
        }
    }else{
        spdlog::error("Acceptor::handleRead {}", errno);
        // 处理文件描述符大于进程限制的情况
        if (errno == EMFILE)
        {
        spdlog::error("Acceptor::handleRead max file descriper exceeded");
        ::close(idleFd_);
        idleFd_ = ::accept(m_acceptSocket.fd(), NULL, NULL);
        ::close(idleFd_);
        idleFd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
        }
    }
}

void Acceptor::listen(){
    spdlog::debug("Acceptor::listen");
    m_listening = true;
    m_acceptSocket.listen();
    // 开始监听后再注册到poll上
    m_acceptChannel.enableReading();
}