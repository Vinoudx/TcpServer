#ifndef _TIMERQUEUE_
#define _TIMERQUEUE_

#include <functional>
#include <memory>
#include <array>
#include <string>
#include <mutex>

#include "noncopyable.hpp"
#include "Channel.hpp"
#include "CurrentThread.hpp"

class EventLoop;

/*
    基于时间轮的定时器组件
    最短间隔时间 100Ms
    最长间隔时间 10s
    含有100ms和1s两个时间轮
*/
class TimerQueue: public noncopyable{

    using Functor = std::function<void()>;
    struct TimerTask{
        bool isAsync;
        bool isInterval;
        Functor task;
        size_t timeoutMs;
        size_t timeIntervalMs;
        // -1为永远执行，0为执行第一次，>0为重复执行次数
        int repeatTimes;
        TimerTask* next;
    };

public:

    TimerQueue(EventLoop* loop);
    ~TimerQueue();

    void tick();

    // 需要检测是否是在当前线程中
    void addTask(Functor func, size_t timeoutMS, bool isInterval = false, int repeatTimes = 0, size_t timeInterval = 0, bool isAsync = false);

private:

    void tickWheel(std::array<TimerTask*, 10>& wheel, uint8_t index);
    void addTaskToWheel(TimerTask* task);
    void doAddTasksInLoop();

    // 在Accptor所在loop，也就是baseloop
    EventLoop* m_loop;
    std::unique_ptr<Channel> m_channel;
    size_t m_100MsCount;
    
    // 0-9
    uint8_t m_100MsPtr;
    // 0-1s
    std::array<TimerTask*, 10> m_100MsQueue;
    // 0-9
    uint8_t m_secondPtr;
    // 1s-10s 
    std::array<TimerTask*, 10> m_secondQueue;

    std::vector<TimerTask*> m_queue;
    std::mutex m_mtx;
};

#endif