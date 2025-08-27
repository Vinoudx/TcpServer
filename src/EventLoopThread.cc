#include "EventLoopThread.hpp"

#include "EventLoop.hpp"

EventLoopThread::EventLoopThread(ThreadInitCallback cb, const std::string& name):
    m_loop(nullptr),
    m_exiting(false),
    m_thread(std::bind(&EventLoopThread::threadFunc, this), name),
    m_callback(std::move(cb))
{}

EventLoopThread::~EventLoopThread(){
    m_exiting = true;
    if(m_loop != nullptr){
        m_loop->quit();
        m_thread.join();
    }
}


// 所以下面这两个函数为什么不是
// 在startLoop中创建EventLoop
// 使用一个线程退出回调来关闭EventLoop
EventLoop* EventLoopThread::startLoop(){
    m_thread.start();
    EventLoop* loop = nullptr;
    {   // 这里必须等待，因为这个函数和下面那个函数不是在同一个线程中执行的
        std::unique_lock<std::mutex> l(m_mtx);
        m_cv.wait(l, [this]{return m_loop != nullptr;});
        loop = m_loop; 
    }
    return loop;
}

void EventLoopThread::threadFunc(){
    // 这就是one loop per thread的体现
    // 这个函数运行在子线程中
    EventLoop loop;
    if(m_callback){
        m_callback(&loop);
    }
    {   // 这里为了保证startLoop在返回前必须让m_loop被赋值
        std::unique_lock<std::mutex> l(m_mtx);
        m_loop = &loop;
        m_cv.notify_all();
    }
    loop.loop();
}