#include "TimeQueue.hpp"

#include <sys/timerfd.h>

#include <spdlog/spdlog.h>

#include <EventLoop.hpp>
#include <future>

static EventLoop* checkNull(EventLoop* loop){
    if(loop == nullptr){
        spdlog::critical("loop is null");
    }
    return loop;
}

TimerQueue::TimerQueue(EventLoop* loop):
    m_loop(checkNull(loop)),
    m_channel(std::make_unique<Channel>(loop, timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC))),
    m_100MsCount(0),
    m_100MsPtr(0),
    m_secondPtr(0){

    if(m_channel->fd() <= 0){
        spdlog::critical("TimerQueue::TimeQueue timerfd creation fail");
    }
    // 设置timerfd
    struct itimerspec its;
    its.it_value.tv_sec = 0;
    its.it_value.tv_nsec = 100 * 1000000; // 100ms
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 100 * 1000000; // 100ms

    if (timerfd_settime(m_channel->fd(), 0, &its, NULL) == -1) {
        spdlog::critical("TimerQueue::TimeQueue timerfd setting fail");
    }

    m_channel->enableReading();
    m_channel->setReadCallback(std::bind(&TimerQueue::tick, this));
    m_100MsQueue.fill(nullptr);
    m_secondQueue.fill(nullptr);
}

TimerQueue::~TimerQueue(){
    m_channel->disableAll();
    m_channel->remove();
}

void TimerQueue::addTask(Functor func, size_t timeoutMS, bool isInterval, int repeatTimes, size_t timeInterval, bool isAsync){
    TimerTask* task = new TimerTask{.isAsync=isAsync, 
                .isInterval=isInterval,
                .task=func,  
                .timeoutMs=timeoutMS,
                .timeIntervalMs=timeInterval,
                .repeatTimes=repeatTimes, 
                .next=nullptr};
    if(m_loop->isInLoopThread()){
        addTaskToWheel(task);
    }else{
        {
            std::unique_lock<std::mutex> l(m_mtx);
            m_queue.emplace_back(task);
        }
    }
}

void TimerQueue::doAddTasksInLoop(){
    std::vector<TimerTask*> tasks;
    {
        std::unique_lock<std::mutex> l(m_mtx);
        tasks.swap(m_queue);
        m_queue.clear();
    }
    for(const auto& item: m_queue){
        addTaskToWheel(item);
    }
}

void TimerQueue::addTaskToWheel(TimerTask* task){
    // 首先看等待时间
    std::array<TimerTask*, 10>* wheelPtr = nullptr;
    int index = 0;
    if(task->timeoutMs < 1000){
        wheelPtr = &m_100MsQueue;
        index = (task->timeoutMs + m_100MsPtr * 100) / 100 % 10;
    }else{
        wheelPtr = &m_secondQueue;
        index = (task->timeoutMs + m_secondPtr * 1000) / 1000 % 10;
    }

    // 插入，保持剩余时间的降序
    TimerTask* currentPtr = (*wheelPtr)[index];
    TimerTask* prePtr = nullptr;
    while(currentPtr != nullptr && currentPtr->timeoutMs < task->timeoutMs){
        prePtr = currentPtr;
        currentPtr = currentPtr->next;
    }
    if(prePtr == nullptr){
        // 头部
        if(currentPtr == nullptr){
            // 第一个
            (*wheelPtr)[index] = task;
        }else{
            // 头插
            task->next = (*wheelPtr)[index];
            (*wheelPtr)[index] = task;
        }
    }else{
        if(currentPtr == nullptr){
            // 尾插
            prePtr->next = task;
        }else{
            // 中间插
            task->next = prePtr->next;
            prePtr->next = task;
        }
    }
}

void TimerQueue::tick(){

    uint64_t expirations;
    ssize_t s;
    s = read(m_channel->fd(), &expirations, sizeof(expirations));
    if (s != sizeof(expirations)) {
        spdlog::error("partial read");
    }else if(s < 0){
        spdlog::critical("timer error");
    }

    tickWheel(m_100MsQueue, m_100MsPtr++);
    spdlog::debug("100ms tick");
    m_100MsCount++;
    if(m_100MsPtr == 10)m_100MsPtr = 0;
    if(m_100MsCount == 10){
        m_100MsCount = 0;
        tickWheel(m_secondQueue, m_secondPtr++);
        spdlog::debug("1s tick");
        if(m_secondPtr == 10)m_secondPtr = 0;
    }
}

void TimerQueue::tickWheel(std::array<TimerTask*, 10>& wheel, uint8_t index){
    
    TimerTask* currentPtr = wheel[index];
    std::vector<TimerTask*> temp;
    while(currentPtr != nullptr){
        // 首先执行任务
        if(currentPtr->isAsync){
            auto t = std::async(std::launch::async, currentPtr->task);
        }else{
            try{
                currentPtr->task();
            } catch (const std::exception& e) {
                spdlog::error("exception occured: {}", e.what());
            } catch (...) {
                spdlog::error("unknown error occured");
            }
        }

        // 检查是否需要继续执行
        if((currentPtr->repeatTimes > 0 || currentPtr->repeatTimes == -1) && currentPtr->isInterval){
            if(currentPtr->repeatTimes > 0) currentPtr->repeatTimes--;
            currentPtr->timeoutMs = currentPtr->timeIntervalMs;
            temp.push_back(currentPtr);
            currentPtr = currentPtr->next;
        }else{
            TimerTask* temp = currentPtr;
            currentPtr = currentPtr->next;
            delete temp;
        }
    }
    wheel[index] = nullptr;
    doAddTasksInLoop();
    for(auto& item: temp){
        addTaskToWheel(item);
    }
}