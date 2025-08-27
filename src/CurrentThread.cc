#include "CurrentThread.hpp"

#include <unistd.h>
#include <sys/syscall.h>


void CurrentThread::cachedTid(){
    if(t_cachedTid == 0){
        t_cachedTid = static_cast<pid_t>(syscall(SYS_gettid));
    }
}