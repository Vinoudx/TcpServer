#include "EpollPoller.hpp"

#include <spdlog/spdlog.h>

#include "Channel.hpp"

// 表示未添加到poller中，或者是被彻底删除过
const int KNew = -1; 
// 表示已添加到poller中
const int KAdded = 1; 
// 表示已经从poller中删除
// 注意这个删除表示自己不对任何事件感兴趣，已经从epoll监听列表中删除
// 但是还未从EpollPoller中删除
// 真正从EpollPoller中删除后状态变成KNew
const int KDeleted = 2; 

EpollPoller::EpollPoller(EventLoop* loop):Poller(loop),
    m_epollfd(epoll_create1(EPOLL_CLOEXEC)),
    m_events(initialEventsSize){
    
    if(m_epollfd < 0)spdlog::critical("EpollPoller::EpollPoller");
}

EpollPoller::~EpollPoller(){
    close(m_epollfd);
}

void EpollPoller::updateChannel(Channel* channel){
    int index = channel->index();
    if(index == KNew || index == KDeleted){
        int fd = channel->fd();
        if(index == KNew){
            // 从未添加过，将channel添加到poll中
            m_channels[fd] = channel;
        }else{
            // 之前添加过，只是后来被删除过了
            // 这里什么都不做
        }
        channel->setIndex(KAdded);
        update(EPOLL_CTL_ADD, channel);
    }else{
        // 已经被添加过，修改感兴趣事件
        if(channel->isNoneEvent()){
            // 如果channel上没有感兴趣事件，则删除
            update(EPOLL_CTL_DEL, channel);
            channel->setIndex(KDeleted);
        }else{
            // 感兴趣事件改变，则进行修改
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EpollPoller::removeChannel(Channel* channel){
    // 这里进行的是彻底的删除，将channel从EpollPoller中删除掉
    int index = channel->index();
    m_channels.erase(channel->fd());
    if(index == KAdded){
        update(EPOLL_CTL_DEL, channel);
    }
    channel->setIndex(KNew);
}

void EpollPoller::update(int operation, Channel* channel){
    struct epoll_event event;
    memset(&event, 0, sizeof(struct epoll_event));
    event.events = channel->events();
    event.data.ptr = channel;
    int fd = channel->fd();
    if(epoll_ctl(m_epollfd, operation, fd, &event)){
        if(operation == EPOLL_CTL_DEL){
            spdlog::error("epoll_ctl delete fail %d", fd);
        }else{
            spdlog::critical("epoll_ctl fail fd%d", fd);
        }
    }
}

TimeStamp EpollPoller::poll(int timeoutMs, ChannelList* activeChannel){
    int numEvents = epoll_wait(m_epollfd, &*m_events.begin(), m_events.size(), timeoutMs);
    TimeStamp now(TimeStamp::now());    
    int savedError = 0;
    if(numEvents > 0){
        fillActiveChannels(numEvents, activeChannel);
        // 如果事件太多，已经到事件量上上限了，就取两倍扩容
        if(numEvents == m_events.size()){
            m_events.resize(numEvents * 2);
        }
    }else if(numEvents < 0){
        savedError = errno;
        spdlog::error("EpollPoller::poll(), error: %d", savedError);
    }
    // else if(numEvents == 0) 超时没有事件，无事发生
    return now;
}

void EpollPoller::fillActiveChannels(int numEvents, ChannelList* activeChannels) const{
    for(int i=0; i<numEvents; ++i){
        Channel* channel = static_cast<Channel*>(m_events[i].data.ptr);
        int events = m_events[i].events;
        channel->setRevents(events);
        activeChannels->emplace_back(channel);
    }
}