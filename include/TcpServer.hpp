#ifndef _TCPSERVER_
#define _TCPSERVER_

#include <functional>
#include <string>
#include <memory>
#include <atomic>
#include <unordered_map>

#include "noncopyable.hpp"
#include "EventLoop.hpp"
#include "Acceptor.hpp"
#include "InetAddress.hpp"
#include "TcpConnection.hpp"
#include "Callbacks.hpp"

class EventLoop;
class EventLoopThreadPool;
class Buffer;

/*
    完结
    这就是服务器类了
    里面包含了一个acceptor和一个eventloopthreadpool
*/
class TcpServer: public noncopyable{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    enum Option{
        KNOREUSEPORT,
        KREUSEPORT
    };

    TcpServer(EventLoop* loop, const InetAddress& listenAddr, const std::string& name, Option option = KNOREUSEPORT);
    ~TcpServer();

    void setThreadNum(int threadNum);
    void setThreadInitCallback(ThreadInitCallback cb){m_threadInitCallback = std::move(cb);}

    void start();

    void setConnectionCallback(ConnectionCallback cb){m_connectionCallback = std::move(cb);}
    void setWriteCompleteCallback(WriteCompleteCallback cb){m_writeCallback = std::move(cb);}
    void setMessageCallback(MessageCallback cb){m_messageCallback = std::move(cb);}

private:

    // 给Acceptor的新连接回调
    void newConnection(int sockfd, const InetAddress& peerAddr);

    void removeConnection(const TcpConnectionPtr& conn);

    void removeConnectionInLoop(const TcpConnectionPtr& conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    // 这是base loop，也就是acceptor中的loop
    EventLoop* m_loop;

    const std::string m_ipPort;
    const std::string m_name;
    std::unique_ptr<Acceptor> m_acceptor;
    std::shared_ptr<EventLoopThreadPool> m_threadPool;
    ConnectionCallback m_connectionCallback;
    WriteCompleteCallback m_writeCallback;
    MessageCallback m_messageCallback;
    ThreadInitCallback m_threadInitCallback;
    std::atomic_int32_t m_started;
    int m_nextFd;

    // 保存所有连接，按连接名字哈希
    ConnectionMap m_map;
};

#endif