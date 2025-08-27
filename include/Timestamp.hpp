#ifndef _TIME_STAMP_
#define _TIME_STAMP_

#include <iostream>
#include <string>

#include "noncopyable.hpp"

class TimeStamp{
public:
    TimeStamp();
    explicit TimeStamp(int64_t miliseconds);
    std::string toString();
    static TimeStamp now();
private:
    int64_t m_miliseconds;
};

#endif