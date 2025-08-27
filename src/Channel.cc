#include "Channel.hpp"

#include <sys/epoll.h>

#include "EventLoop.hpp"

const int Channel::KNoneEvent = 0;
 // 这个是EPOLLRPRI是为了带外信息吗
const int Channel::KReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::KWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop* loop, int fd):m_loop(loop),
                                          m_fd(fd),
                                          m_events(KNoneEvent),
                                          m_revents(KNoneEvent),
                                          m_index(-1),
                                          m_tied(false){}

void Channel::tie(const std::shared_ptr<void>& ptr){
    m_tie = ptr;
    m_tied = true;
}

void Channel::update(){
    // m_loop->update(this);
}

void Channel::remove(){
    // m_loop->removeChannel(this);
}

void Channel::handleEvent(TimeStamp recvtime){
    std::shared_ptr<void> guard;
    if(m_tied){
        guard = m_tie.lock();
        if(guard){
            handleEventWithGuard(recvtime);
        }
    }else{
        handleEventWithGuard(recvtime);
    }
}

void Channel::handleEventWithGuard(TimeStamp recvtime){
    if((m_revents & EPOLLHUP) && !(m_revents & EPOLLIN)){
        if(m_closeCallback) m_closeCallback();
    }
    if(m_revents & EPOLLERR){
        if(m_errorCallback)m_errorCallback();
    }
    if(m_revents & (EPOLLIN | EPOLLPRI)){
        if(m_readCallback)m_readCallback(recvtime);
    }
    if(m_revents & EPOLLOUT){
        if(m_writeCallback)m_writeCallback();
    }
}