#include "Timestamp.hpp"

TimeStamp::TimeStamp():m_miliseconds(0){}

TimeStamp::TimeStamp(int64_t miliseconds):m_miliseconds(miliseconds){}

std::string TimeStamp::toString(){
    tm* t = localtime(&m_miliseconds);
    char buf[128];
    snprintf(buf, 128, "%4d/%02d/%02d %02d:%02d:%02d",
        t->tm_year + 1900,
        t->tm_mon + 1,
        t->tm_mday,
        t->tm_hour,
        t->tm_min,
        t->tm_sec
    );
    return buf;
}

TimeStamp TimeStamp::now(){

    return TimeStamp(time(nullptr));
}