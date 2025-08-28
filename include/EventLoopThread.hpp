#ifndef _EVENT_LOOP_THREAD_
#define _EVENT_LOOP_THREAD_

#include <memory>
#include <functional>
#include <string>
#include <mutex>
#include <condition_variable>

#include "noncopyable.hpp"
#include "Thread.hpp"

class EventLoop;

/*
    EventLoopThread是对EventLoop在线程中的封装
    简单的说就是把EventLoop对象放到Thread类中运行
    实现one loop per thread
*/
class EventLoopThread: public noncopyable{
public:
    // 这是在线程初始化时调用的回调函数
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    
    explicit EventLoopThread(ThreadInitCallback cb = ThreadInitCallback(), const std::string& name = std::string());
    ~EventLoopThread();

    void threadFunc();
    // 开启内部的线程，执行EventLoop的loop()
    EventLoop* startLoop();

private:
    EventLoop* m_loop;
    bool m_exiting;
    Thread m_thread;
    std::mutex m_mtx;
    std::condition_variable m_cv;
    ThreadInitCallback m_callback;
};  

#endif