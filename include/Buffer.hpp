#ifndef _BUFFER_
#define _BUFFER_

#include <vector>
#include <string>

/*
    一个缓冲区，包含三段
    数据长度    读缓冲区    写缓冲区
            
        readerIndex  writerIndex

    所有数据都在readerIndex和writerInder中间
*/
class Buffer{
public:
    // 数据长度字段占8字节
    static const size_t KCheapPrepend = 8;
    // 初始整个缓冲区占1024字节
    static const size_t KInitSize = 1024;

    explicit Buffer(size_t initSize = KInitSize):
        m_buffer(KCheapPrepend + initSize),
        m_readerIndex(KCheapPrepend),
        m_writerIndex(KCheapPrepend){}
    
    ~Buffer() = default;

    size_t readableBytes() const{return m_writerIndex - m_readerIndex;}
    size_t writableBytes() const{return m_buffer.size() - m_writerIndex;}
    size_t prependableBytes() const{return m_readerIndex;}
    
    // 返回可读数据缓冲区的起始地址
    const char* peek() const{
        return begin() + m_readerIndex;
    }

    void retrive(size_t len){
        if(len < readableBytes()){
            m_readerIndex += len;
        }else{
            retriveAll();
        }
    }

    void retriveAll(){
        m_readerIndex = m_writerIndex = KCheapPrepend;
    }

    std::string retriveAllAsString(){
        return retriveAsString(readableBytes());
    }

    std::string retriveAsString(size_t len){
        std::string res(peek(), len);
        retrive(len);
        return res;
    }

    void ensureWritableBytes(size_t len){
        if(writableBytes() < len){
            makeSpace(len);
        }
    }

    void makeSpace(size_t len){
        if(writableBytes() + prependableBytes() < len + KCheapPrepend){
            // 剩余空间不足
            m_buffer.resize(len + m_writerIndex);
        }else{
            // 剩余空间够，将数据移动到开头
            size_t readable = readableBytes();
            std::copy(begin() + m_readerIndex, begin() + m_writerIndex, begin() + KCheapPrepend);
            m_readerIndex = KCheapPrepend;
            m_writerIndex = KCheapPrepend + readable;
        }
    }

    // 将数据写到buffer中
    void append(const char* data, size_t len){
        ensureWritableBytes(len);
        std::copy(data, data + len, begin() + m_writerIndex);
        m_writerIndex += len;
    }

    // 从fd上读取数据
    ssize_t readFd(int fd, int* savederrno);

private:

    char* begin(){
        return &*m_buffer.begin();
    }

    const char* begin() const{
        return &*m_buffer.begin();
    }

    std::vector<char> m_buffer;
    size_t m_writerIndex;
    size_t m_readerIndex;
};

#endif