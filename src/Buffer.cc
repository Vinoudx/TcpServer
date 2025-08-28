#include "Buffer.hpp"

#include <sys/uio.h>

ssize_t Buffer::readFd(int fd, int* savederrno){
    // buffer有大小，但从fd上读不知道多少字节
    // 所以使用readv，用两个缓冲区
    char extraBuf[65536] = {0};
    struct iovec vec[2];
    // 缓冲区剩余可写空间大小
    const size_t writable = writableBytes();
    vec[0].iov_base = begin() + m_writerIndex;
    vec[0].iov_len = writable;

    vec[1].iov_base = extraBuf;
    vec[1].iov_len = sizeof extraBuf;

    // 最多一次读128k-1，但为什么要这样比一下
    const int iovcnt = (writable < sizeof extraBuf) ? 2 : 1;
    const ssize_t n = ::readv(fd, vec, iovcnt);
    if(n < 0){
        *savederrno = errno;
    }else if(n <= writable){
        m_writerIndex += n;
    }else{
        m_writerIndex = m_buffer.size();
        append(extraBuf, n - writable);
    }
    return n;
}