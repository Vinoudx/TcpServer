#ifndef _EVENT_LOOP_THREAD_POOL_
#define _EVENT_LOOP_THREAD_POOL_

#include <string>
#include <vector>
#include <memory>
#include <functional>

#include "noncopyable.hpp"
#include "EventLoopThread.hpp"
class EventLoop;
class EventLoopThread;
/*
    管理多个EventLoopThread的类
    对外提供一个getNextLoop()的方法
    共创建新连接后将对应channel分发到子反应堆（子线程）使用
*/
class EventLoopThreadPool: public noncopyable{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    explicit EventLoopThreadPool(EventLoop* baseLoop, const std::string& name = std::string());
    ~EventLoopThreadPool() = default;

    void setThreadNum(int num){m_numThreads = num;}
    void start(ThreadInitCallback cb = ThreadInitCallback());
    
    // 默认以轮询方式将channel分配给loop
    // 这里直接向loop添加，跳过Thread类中间商，因为不方便
    EventLoop* getNextLoop();

    std::vector<EventLoop*> getAllLoops();
    bool started(){return m_started;}

    const std::string& name() const{return m_name;}

private:
    // 这是默认情况下muduo使用一个线程时所使用的loop
    // 也就是在创建服务器对象时自己传进去的那个loop
    // 一个线程自然只有一个loop
    EventLoop* m_baseLoop;

    std::string m_name;
    bool m_started;
    int m_numThreads;
    int m_next;
    std::vector<std::unique_ptr<EventLoopThread>> m_threads;
    
    // 这里指向的EventLoop对象是栈上对象
    // 在线程函数中以局部变量的形式创建的
    // 在线程结束，出线程函数作用域时自动销毁
    std::vector<EventLoop*> m_loops;
};

#endif