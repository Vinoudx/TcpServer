#ifndef _CHANNEL_
#define _CHANNEL_

#include <functional>
#include <memory>

#include "noncopyable.hpp"
#include "Timestamp.hpp"

class EventLoop;

/*
    什么是Channel
    Channel封装了sockfd，其感兴趣的事件，和真实发生的事件
    其被eventloop包含，使用一个指针与loop通信
    也被tcpconnection包含
*/

class Channel: public noncopyable{
public:
    // 发生事件时的回调函数
    using EventCallback = std::function<void()>;
    // 发生读事件的回调函数
    using ReadEventCallbcak = std::function<void(TimeStamp)>;

    Channel(EventLoop* eventloop, int fd);
    ~Channel() = default;

    // 事件处理的入口
    void handleEvent(TimeStamp recvTime);

    // 设置回调操作
    void setReadCallback(ReadEventCallbcak cb){m_readCallback = std::move(cb);}
    void setWriteCallback(EventCallback cb){m_writeCallback = std::move(cb);}
    void setCloseCallback(EventCallback cb){m_closeCallback = std::move(cb);}
    void setErrorCallback(EventCallback cb){m_errorCallback = std::move(cb);}

    // 防止channel在执行回调函数时，所属TcpConnection被析构掉
    void tie(const std::shared_ptr<void>& ptr);

    int fd() const {return m_fd;}
    int events() const {return m_events;}
    // 设置真实发生的事件
    void setRevents(int events) {m_revents = events;}

    // 以下是对感兴趣事件的setter和getter
    void enableReading(){m_events |= KReadEvent; update();}
    void disableReading(){m_events &= ~KReadEvent; update();}
    void enableWriting(){m_events |= KWriteEvent; update();}
    void disableWriting(){m_events &= ~KWriteEvent; update();} 
    void disableAll(){m_events = KNoneEvent; update();}
    bool isWriting() const {return m_events & KWriteEvent;}
    bool isReading() const {return m_events & KReadEvent;}
    bool isNoneEvent() const {return m_events == KNoneEvent;}

    int index(){return m_index;}
    void setIndex(int index){m_index = index;}
    EventLoop* ownerLoop() {return m_loop;}

    // 将自己从所属loop中移除
    void remove();
private:
    // 将自己感兴趣事件的改变通知到pool，使其调整epoll的监听
    void update();
    // 真正处理事件的函数
    void handleEventWithGuard(TimeStamp recvTime);

    // 三个是对channel状态的描述
    static const int KNoneEvent;
    static const int KReadEvent;
    static const int KWriteEvent;

    // 这个channel所属的eventloop
    EventLoop* m_loop;
    // 封装的fd，eventloop所监听的对象
    const int m_fd;
    // 感兴趣的事件
    int m_events;
    // 具体发生的事件
    int m_revents;
    // 表示channel在epoll中的状态，详见EpollPoller中的三个常量
    int m_index;

    // 防止在执行回调时所属TcpConnection被析构掉
    // 在调用事件处理函数时延长TcpConnection的生命周期
    // 这个指针指向的是TcpConnection
    // 使用弱指针是为了防止干扰TcpConnection的正常析构
    std::weak_ptr<void> m_tie;
    bool m_tied;

    // 对各种事件的回调函数，按照revent调用
    // channel负责调用各种事件最终的回调函数
    ReadEventCallbcak m_readCallback;
    EventCallback m_writeCallback;
    EventCallback m_closeCallback;
    EventCallback m_errorCallback;
};  

#endif