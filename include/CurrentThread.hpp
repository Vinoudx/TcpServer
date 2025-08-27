#ifndef _CURRENT_THREAD_
#define _CURRENT_THREAD_

namespace CurrentThread{

    // threadlocal变量
    inline thread_local int t_cachedTid = 0;

    // 通过linux系统调用获得当前线程的tid值
    void cachedTid();

    // 获得当前线程的tid，注意inline和unlikely
    inline int tid(){
        if(t_cachedTid == 0) [[unlikely]]{
            cachedTid();
        }
        return t_cachedTid;
    }
}

#endif