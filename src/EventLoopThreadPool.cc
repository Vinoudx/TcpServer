#include "EventLoopThreadPool.hpp"

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const std::string& name):
    m_baseLoop(baseLoop),
    m_name(name),
    m_started(false),
    m_numThreads(0),
    m_next(0){}

void EventLoopThreadPool::start(ThreadInitCallback cb){
    m_started = true;
    for(int i=0;i<m_numThreads;i++){
        char buf[m_name.size() + 32];
        snprintf(buf, sizeof buf, "%s%d", m_name.c_str(), i);
        EventLoopThread* t = new EventLoopThread(std::move(cb), buf);
        m_threads.emplace_back(t);
        m_loops.emplace_back(t->startLoop());
    }
    if(m_numThreads == 0 && cb){
        // 只有m_baseLoop在运行
        cb(m_baseLoop);
    }
}

std::vector<EventLoop*> EventLoopThreadPool::getAllLoops(){
    if(m_threads.empty()){
        return std::vector<EventLoop*>(1, m_baseLoop);
    }else{
        return m_loops;
    }
}

EventLoop* EventLoopThreadPool::getNextLoop(){
    EventLoop* loop = m_baseLoop;
    if(!m_loops.empty()){
        loop = m_loops[m_next++];
        if(m_next >= m_loops.size())m_next = 0;
    }
    return loop;
}
