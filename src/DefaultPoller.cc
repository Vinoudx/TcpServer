#include "Poller.hpp"
#include "EpollPoller.hpp"

class EventLoop;

// 为什么这个函数需要在这里实现而不是在Poller.cc中实现
// 语法上没有问题，主要是考虑到Poller.h是一个抽象类
// 不应该在这个抽象类的实现中包含具体类的头文件
// 按理应该是具体类包含抽象类的头文件
Poller* Poller::getDefaultPoller(EventLoop* loop){
    return new EpollPoller(loop);
}