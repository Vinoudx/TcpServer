#ifndef _EVENT_LOOP_
#define _EVENT_LOOP_

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

#include "noncopyable.hpp"
#include "Timestamp.hpp"
#include "CurrentThread.hpp"

class Channel;
class Poller;

/*
    eventLoop是什么
    EventLoop是对EpollPoller和channel的封装
    EventLoop不断调用EpollPoller中的epoll_wait获得发生事件的Channel
*/

class EventLoop: public noncopyable{
public:
    // 这是这里所有用到的回调的类型
    using Functor = std::function<void()>;
    EventLoop();
    ~EventLoop();

    void loop();
    void quit();

    TimeStamp pollReturnTime() const{return m_pollReturnTime;}

    // 若任务来自loop创建时所在线程，则直接执行
    void runInLoop(Functor cb);
    // 若不在，则加入队列
    void queueInLoop(Functor cb);
    // 唤醒loop所在线程
    void wakeup();
    // 对poller中的channel进行
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    bool hasChannel(Channel* channel);

    // 看当前线程是不是loop创建时所在的线程
    inline bool isInLoopThread(){return m_threadId == CurrentThread::tid();}



private:

    // 给wakeupfd用的， 其实没做什么事情，就是把fd里面的东西读出来
    void handleRead();
    void doPendingFunctors();

    void assertInLoopThread();

    using ChannelList = std::vector<Channel*>;
    std::atomic_bool m_looping;
    std::atomic_bool m_quit;

    // 记录当前loop所在线程id
    // 与当前线程id比较可以确定是不是在loop线程中
    const pid_t m_threadId;
    // poller返回发生事件的时间点
    TimeStamp m_pollReturnTime;

    std::unique_ptr<Poller> m_poller;

    // 当无事件发生时
    // poller线程会阻塞在epoll_wait
    // 使用wakeupFd向poller发送消息唤醒poller线程
    // 是一种统一事件源的思想
    int m_wakeupFd;
    std::unique_ptr<Channel> m_wakeupChannel;
    
    ChannelList m_activeChannels;
    Channel* m_currentActiveChannel;
    
    // 外部来的任务的队列和队列的保护锁
    // 为什么要这个队列
    // 因为loop所在线程阻塞在epoll_wait
    // 直接把任务交给loop的话它也执行不了
    mutable std::mutex m_mtx;
    std::vector<Functor> m_pendingFunctors;
    // 正在执行附加的任务
    std::atomic_bool m_callingPendingFunc;
};

#endif