#ifndef _NON_COPYABLE_
#define _NON_COPYABLE_

class noncopyable{
public:
    noncopyable() = default;
    noncopyable(const noncopyable&) = delete;
    noncopyable& operator=(const noncopyable&) = delete;
    ~noncopyable() = default;
};

#endif