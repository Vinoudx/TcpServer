#include "Poller.hpp"

#include "Channel.hpp"

Poller::Poller(EventLoop* loop):m_ownerLoop(loop){}

bool Poller::hasChannel(Channel* channel) const{
    auto it = m_channels.find(channel->fd());
    return it != m_channels.end() && it->second == channel;
}