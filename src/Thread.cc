#include "Thread.hpp"

#include <semaphore>

#include "CurrentThread.hpp"

std::atomic_int32_t Thread::m_count = 0;

Thread::Thread(ThreadFunc func, const std::string& name):
    m_name(name),
    m_started(false),
    m_joined(false),
    m_tid(0),
    m_func(std::move(func)){
    if(m_name.empty()){
        setDefaultName();
    }
}

Thread::~Thread(){
    if(m_started && !m_joined){
        m_thread->detach();
    }
}

void Thread::setDefaultName(){
    int num = m_count++;
    char buf[32] = {0};
    snprintf(buf, 32, "Thread%d", num);
    m_name = buf;
}

void Thread::start(){
    m_started = true;
    sem_t sem;
    sem_init(&sem, false, 0);
    m_thread = std::make_shared<std::thread>([this, &sem]{
        m_tid = CurrentThread::tid();
        sem_post(&sem);
        m_func();
    });
    // 这里必须等待子线程获得tid后才能继续执行
    // 防止因为线程调度问题导致迟迟不能获得tid从而导致后续问题
    sem_wait(&sem);
}

void Thread::join(){
    if(m_started && !m_joined){
        m_joined = true;
        m_thread->join();
    }
}