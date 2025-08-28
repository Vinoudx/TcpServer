#include "TcpServer.hpp"

#include <sys/socket.h>
#include <unistd.h>

#include "EventLoopThreadPool.hpp"

#include <spdlog/spdlog.h>

static EventLoop* checkNull(EventLoop* loop){
    if(loop == nullptr){
        spdlog::critical("loop is null");
    }
    return loop;
}

TcpServer::TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& name, Option option):
    m_loop(checkNull(loop)),
    m_ipPort(listenAddr.toIpPort()),
    m_name(name),
    m_acceptor(std::make_unique<Acceptor>(loop, listenAddr, option == KREUSEPORT)),
    m_threadPool(std::make_shared<EventLoopThreadPool>(loop, name)),
    m_connectionCallback(defaultConnectionCallback),
    m_messageCallback(defaultMessageCallback),
    m_nextFd(0){

    m_acceptor->setNewConnectionCallback(std::bind(&TcpServer::newConnection, this, std::placeholders::_1, std::placeholders::_2));

}

TcpServer::~TcpServer(){
    for(auto& item: m_map){
        TcpConnectionPtr conn(item.second);
        item.second.reset();
        conn->getLoop()->runInLoop(std::bind(&TcpConnection::connectDestoried, conn));
    }
}

void TcpServer::setThreadNum(int threadNum){
    m_threadPool->setThreadNum(threadNum);
}

void TcpServer::start(){
    // 保证只启动一次
    if(m_started++ == 0){
        m_threadPool->start(m_threadInitCallback);
        // 这个跟直接调用m_accptor->listen()有什么区别？
        m_loop->runInLoop(std::bind(&Acceptor::listen, m_acceptor.get()));
    }
}

void TcpServer::newConnection(int sockfd, const InetAddress& peerAddr){
    EventLoop* loop = m_threadPool->getNextLoop();
    char buf[64];
    snprintf(buf, sizeof buf, "-%s#%d", m_ipPort.c_str(), m_nextFd);
    m_nextFd++;
    std::string connName = m_name+buf;
    spdlog::info("TcpServer::newConnection [%s] - new connection [%s] from %s", m_name, connName, peerAddr.toIpPort());
    
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    if(::getsockname(sockfd, (sockaddr*)&local, &addrlen) < 0){
        spdlog::error("socket::getsockname");
    }
    InetAddress localAddr(local);
    TcpConnectionPtr conn(new TcpConnection(loop, connName, sockfd, localAddr, peerAddr));
    m_map[connName] = conn;
    conn->setConnectionCallback(m_connectionCallback);
    conn->setMessageCallback(m_messageCallback);
    conn->setWriteCompleteCallback(m_writeCallback);
    conn->setCloseCallback(std::bind(&TcpServer::removeConnection, this, std::placeholders::_1));
    loop->runInLoop(std::bind(&TcpConnection::connectEstablished, conn));
}

void TcpServer::removeConnection(const TcpConnectionPtr& conn){
    m_loop->runInLoop(std::bind(&TcpServer::removeConnectionInLoop, this, conn));
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr& conn){
    spdlog::info("TcpServer::removeConnection [%s] - connection", conn->name());
    m_map.erase(conn->name());
    EventLoop* ioloop = conn->getLoop();
    ioloop->runInLoop(std::bind(&TcpConnection::connectDestoried, conn));
}

