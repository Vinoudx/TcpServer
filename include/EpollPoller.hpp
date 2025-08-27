#ifndef _EPOLLPOLLER_
#define _EPOLLPOLLER_

#include <memory>
#include <vector>
#include <sys/epoll.h>

#include "noncopyable.hpp"
#include "Poller.hpp"
#include "Timestamp.hpp"

class EventLoop;

/*
    EpollPoller是什么
    是对epoll的封装，包含了一个epoll，所有注册在epoll中的channel（本质上是fd）
    并提供了对这些channel进行增删改查的接口
    本质上是一个epoll的封装
*/
class EpollPoller: public Poller{
public:
    EpollPoller(EventLoop* loop);
    ~EpollPoller() override;

    TimeStamp poll(int timeoutMs, ChannelList* activeChannel) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:

    // 真正将发生事件的channel填好的函数
    void fillActiveChannels(int numEvents, ChannelList* activeChannel) const;
    // 对channel进行修改
    void update(int operation, Channel* channel);
    static const int initialEventsSize = 16;

    // 为什么用vector不用数组
    // 需要考虑到事件太多需要扩容的情况
    // 防止在一个循环中无法将所有事件读出
    using EventList = std::vector<struct epoll_event>;

    int m_epollfd;
    EventList m_events;
};

#endif