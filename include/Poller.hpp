#ifndef _POOLER_
#define _POOLER_

#include <vector>
#include <unordered_map>

#include "noncopyable.hpp"
#include "Timestamp.hpp"

class Channel;
class EventLoop;

class Poller: public noncopyable{
public:
    using ChannelList = std::vector<Channel*>;
    Poller(EventLoop* loop);
    virtual ~Poller() = default;

    // 实现epoll_wait的函数
    // 将发生事件的channel接到传入的ChannelList中
    // 返回发生时间点的TimeStamp
    virtual TimeStamp poll(int timeoutMs, ChannelList* channels) = 0;
    
    // 实现对epoll中的channel进行更改
    // 主要是感兴趣事件的更改以及channel的增加和删除
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;
    
    bool hasChannel(Channel* channel) const;

    // 为什么需要这个函数？
    // 因为在muduo的实现中支持poll和epoll，靠系统的环境变量选择
    // 所以需要一个统一的接口，将这个选择过程封装起来，得到一个默认的poll对象
    // 但在这里我们只支持epoll，所以这个函数直接返回一个EpollPoller
    static Poller* getDefaultPoller(EventLoop* loop);
protected:
    // 这是pool中所有channel, 根据channel中fd的值找到channel
    // 这个fd是来自epoll中监听有事件的fd
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap m_channels;
private:
    // 指向所属的eventloop
    EventLoop* m_ownerLoop;
};

#endif