#ifndef _THREAD_
#define _THREAD_

#include <memory>
#include <thread>
#include <functional>
#include <string>
#include <unistd.h>
#include <atomic>

#include "noncopyable.hpp"

/*
    Thread是对std::thread的封装
    个人认为在原版muduo库的实现中采用的是pthread，所以需要这种封装
    而这里采用std::thread就没必要这样封装了
*/

class Thread: public noncopyable{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc func, const std::string& name = std::string());
    ~Thread();

    void start();
    void join();

    bool started(){return m_started;}
    pid_t tid(){return m_tid;}

    const std::string& name() const {return m_name;}
    static int numCreated(){return m_count;}

private:

    void setDefaultName();

    std::shared_ptr<std::thread> m_thread;
    bool m_started;
    bool m_joined;
    std::string m_name;
    ThreadFunc m_func;
    pid_t m_tid;
    // 记录一共产生了多少线程
    static std::atomic_int32_t m_count;
};

#endif
