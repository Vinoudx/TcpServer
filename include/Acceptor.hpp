#ifndef _ACCEPTOR_
#define _ACCEPTOR_

#include <functional>

#include "noncopyable.hpp"
#include "Socket.hpp"
#include "Channel.hpp"
#include "InetAddress.hpp"

class EventLoop;

/*
    服务器中负责监听监听socket的封装
    负责在独立的eventloop中只负责和新用户的连接事件
    注意这个eventloop运行在主线程上
*/
class Acceptor: public noncopyable{
public:
    // 在新连接建立时调用的函数
    // 主要就是从EventLoopThreadPool中获得下一个EventLoop
    // 将新连接放到那个loop中
    // 由tcpserver设置，也就是用户自己设置的那个
    using NewConnectionCallback = std::function<void(int, const InetAddress&)>;

    Acceptor(EventLoop* loop, const InetAddress& listenAddr, bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(NewConnectionCallback cb){m_cb = std::move(cb);}

    bool listening(){return m_listening;}

    void listen();

private:
    // 在新连接创建时所调用的回调
    // 在这里对NewConnectionCallback的调用
    void handleRead();

    // 这就是创建服务器时用户传入的那个loop
    // 也就是eventpoolThreadPool中的base loop
    EventLoop* m_loop;
    Socket m_acceptSocket;
    // 需要事件循环监听，所以需要channel
    Channel m_acceptChannel;
    NewConnectionCallback m_cb;
    bool m_listening;
    int idleFd_;
};

#endif