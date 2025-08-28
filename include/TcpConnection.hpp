#ifndef _TCPCONNECTION_
#define _TCPCONNECTION_

#include <memory>
#include <string>
#include <atomic>

#include "noncopyable.hpp"
#include "InetAddress.hpp"
#include "Callbacks.hpp"
#include "Buffer.hpp"
#include "Timestamp.hpp"

class Channel;
class EventLoop;
class Socket;

/*
    代表已建立连接的客户端和服务器的通信链路
    所有用户的读写操作都在这里实现
*/
class TcpConnection: public noncopyable, public std::enable_shared_from_this<TcpConnection>{
    // disconnecting表示被调用关闭，但是还有数据在发送
    enum StateE{
        KDisconnectd,
        KDisconnecting,
        KConnected,
        KConnecting
    };
public:

    TcpConnection(EventLoop* loop, const std::string& name, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr);
    ~TcpConnection();

    EventLoop* getLoop() const{return m_loop;}
    const std::string& name() const{return m_name;}
    const InetAddress& localAddress() const{return m_localAddr;}
    const InetAddress& peerAddress() const{return m_peerAddr;}
    
    bool connected() const{return m_state == KConnected;}

    void send(const std::string& buf);
    void shutdown();

    void setState(StateE state){m_state = state;}

    void setConnectionCallback(const ConnectionCallback& cb)
    { m_connectionCallback = cb; }

    void setMessageCallback(const MessageCallback& cb)
    { m_messageCallback = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback& cb)
    { m_writeCallback = cb; }

    void setCloseCallback(const CloseCallback& cb)
    { m_closeCallback = cb; }

    void setHighWaterMarkCallback(const HighWaterMarkCallback& cb, size_t highWaterMark)
    { m_highWaterCallback = cb; m_highWaterMark = highWaterMark; }

    // 连接建立的时候
    void connectEstablished();
    // 连接断开的时候
    void connectDestoried();

private:

    void handleRead(TimeStamp recvTime);

    // channel可写事件的回调
    // 处理的是如果一次没写完，就需要让epoll去监听可写事件
    // 可写了再将事情写完
    // 避免阻塞调用线程
    void handleWrite();
    void handleClose();
    void handleError();

    // 将需要发送的消息写入buffer
    // 防止用户写的快，内核发的慢
    void sendInLoop(const void* message, size_t len);
    void shutdownInLoop();

    EventLoop* m_loop;
    const std::string m_name;
    std::atomic<StateE> m_state;
    bool m_reading;

    std::unique_ptr<Socket> m_socket;
    std::unique_ptr<Channel> m_channel;
    const InetAddress m_localAddr;
    const InetAddress m_peerAddr;
    ConnectionCallback m_connectionCallback;
    WriteCompleteCallback m_writeCallback;
    MessageCallback m_messageCallback;
    CloseCallback m_closeCallback;
    HighWaterMarkCallback m_highWaterCallback;

    size_t m_highWaterMark;
    Buffer m_inputBuffer;
    Buffer m_outputBuffer;
};

#endif