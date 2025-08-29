#include "EventLoop.hpp"

#include <sys/eventfd.h>

#include <spdlog/spdlog.h>

#include "EpollPoller.hpp"
#include "Channel.hpp"

// 防止一个线程中创建多个EventLoop对象
thread_local EventLoop* t_loopInThisThread = nullptr;

const int KPollTimeoutMs = 10000;

// 创建eventfd作为wakeupFd
int createEventFd(){
    int evtFd = eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if(evtFd < 0){
        spdlog::critical("createEventFd()");
        exit(-1);
    }
    return evtFd;
}

EventLoop::EventLoop():m_looping(false),
                        m_quit(false),
                        m_threadId(CurrentThread::tid()),
                        m_poller(EpollPoller::getDefaultPoller(this)),
                        m_wakeupFd(createEventFd()),
                        m_wakeupChannel(std::make_unique<Channel>(this, m_wakeupFd)),
                        m_callingPendingFunc(false),
                        m_currentActiveChannel(nullptr){

    if(t_loopInThisThread){
        spdlog::critical("another eventloop in this thread");
    }else{
        t_loopInThisThread = this;
    }
    // 设置wakeupfd的事件类型以及发生事件后的处理函数
    m_wakeupChannel->setReadCallback(std::bind(&EventLoop::handleRead, this));
    m_wakeupChannel->enableReading();
}

EventLoop::~EventLoop(){
    m_wakeupChannel->disableAll();
    m_wakeupChannel->remove();
    close(m_wakeupFd);
    t_loopInThisThread = nullptr;
}

void EventLoop::handleRead(){
    uint64_t one = 1;
    ssize_t n = read(m_wakeupFd, &one, sizeof(uint64_t));
    if(n != sizeof(uint64_t)){
        spdlog::error("EventLoop::handleRead recv {} bytes", n);
    }
}

void EventLoop::loop(){
    assertInLoopThread();
    spdlog::debug("EventLoop::loop");
    m_looping = true;
    m_quit = false;
    while(!m_quit){
        m_activeChannels.clear();
        m_pollReturnTime = m_poller->poll(KPollTimeoutMs, &m_activeChannels);
        for(Channel* channel: m_activeChannels){
            m_currentActiveChannel = channel;
            m_currentActiveChannel->handleEvent(m_pollReturnTime);
        }
        m_currentActiveChannel = nullptr;
        doPendingFunctors();
    }
    m_looping = false;
}

void EventLoop::removeChannel(Channel* channel){
    assertInLoopThread();
    m_poller->removeChannel(channel);
}

void EventLoop::updateChannel(Channel* channel){
    assertInLoopThread();
    m_poller->updateChannel(channel);
}

bool EventLoop::hasChannel(Channel* channel){
    assertInLoopThread();
    return m_poller->hasChannel(channel);
}

void EventLoop::assertInLoopThread(){
    if(!isInLoopThread()){
        spdlog::critical("not in loop thread");
    }
}

void EventLoop::doPendingFunctors(){
    assertInLoopThread();
    m_callingPendingFunc = true;
    // 这个设计很妙
    // 将任务队列换出来，尽量减少上锁时间
    // 防止任务耗时太长从而因为争夺锁导致大量线程阻塞    
    std::vector<Functor> functors;
    {
        std::lock_guard<std::mutex> l(m_mtx);
        functors.swap(m_pendingFunctors);
    }
    for(const auto& func: functors){
        func();
    }
    m_callingPendingFunc = false;
}

void EventLoop::quit(){
    m_quit = true;
    if(!isInLoopThread()){
        // 如果在loop线程之外调用quit，loop阻塞在epoll_wait，需要进行唤醒
        // 如果时loop自己调用quit就不需要唤醒
        wakeup();
    }
}

void EventLoop::wakeup(){
    uint64_t one = 1;
    ssize_t n = write(m_wakeupFd, reinterpret_cast<char*>(&one), sizeof(uint64_t));
    if(n != sizeof(uint64_t)){
        spdlog::error("EventLoop::wakeup send {} bytes", n);
    }
}

void EventLoop::runInLoop(Functor cb){
    if(isInLoopThread()){
        cb();
    }else{
        queueInLoop(std::move(cb));
    }
}

void EventLoop::queueInLoop(Functor cb){
    {
        std::lock_guard<std::mutex> l(m_mtx);
        m_pendingFunctors.emplace_back(std::move(cb));
    }
    if(!isInLoopThread() || m_callingPendingFunc){
        wakeup();
    }
}