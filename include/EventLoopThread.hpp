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
    EventLoopThread是对
*/
class EventLoopThread: public noncopyable{
public:
    // 这是在线程初始化时调用的回调函数
    using ThreadInitCallback = std::function<void(EventLoop*)>;
    
    explicit EventLoopThread(ThreadInitCallback cb = ThreadInitCallback(), const std::string& name = std::string());
    ~EventLoopThread();

    void threadFunc();
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