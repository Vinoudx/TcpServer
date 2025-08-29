#include "TcpConnection.hpp"

#include <errno.h>

#include <spdlog/spdlog.h>

#include "Socket.hpp"
#include "Channel.hpp"
#include "EventLoop.hpp"

static EventLoop* checkNull(EventLoop* loop){
    if(loop == nullptr){
        spdlog::critical("loop is null");
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop* loop, const std::string& name, int sockfd, const InetAddress& localAddr, const InetAddress& peerAddr):
    m_loop(checkNull(loop)),
    m_name(name),
    m_state(KDisconnectd),
    m_reading(true),
    m_socket(std::make_unique<Socket>(sockfd)),
    m_channel(std::make_unique<Channel>(loop, m_socket->fd())),
    m_localAddr(localAddr),
    m_peerAddr(peerAddr),
    // 高水位64MB
    m_highWaterMark(64*1024*1024){

    m_channel->setReadCallback(std::bind(&TcpConnection::handleRead, this, std::placeholders::_1));
    m_channel->setWriteCallback(std::bind(&TcpConnection::handleWrite, this));
    m_channel->setCloseCallback(std::bind(&TcpConnection::handleClose, this));
    m_channel->setErrorCallback(std::bind(&TcpConnection::handleError, this));
    spdlog::info("TcpConnection::ctor[{}] at fd={}", name.c_str(), sockfd);
    m_socket->setKeepAlive(true);
}

TcpConnection::~TcpConnection(){
    spdlog::info("TcpConnection::dtor[{}] at fd={} state={}", m_name.c_str(), m_socket->fd(), (int)m_state);
}

void TcpConnection::handleRead(TimeStamp recvTime){
    int savedError = 0;
    ssize_t n = m_inputBuffer.readFd(m_channel->fd(), &savedError);
    if(n > 0){
        m_messageCallback(shared_from_this(), &m_inputBuffer, recvTime);
    }else if(n == 0){
        handleClose();
    }else{
        errno = savedError;
        spdlog::error("TcpConnection::handleRead");
        handleError();
    }
}
void TcpConnection::handleWrite(){
    if(m_channel->isWriting()){
        size_t n = ::write(m_channel->fd(), m_outputBuffer.peek(), m_outputBuffer.readableBytes());
        if(n > 0){
            m_outputBuffer.retrive(n);
            if(m_outputBuffer.readableBytes() == 0){
                // 发送完马上关闭连接
                m_channel->disableWriting();
                if(m_writeCallback){
                    // 写操作一定不在loop所在线程中
                    m_loop->queueInLoop(std::bind(m_writeCallback, shared_from_this()));
                }
                if(m_state == KDisconnecting){
                    // 表示正在等待写操作完成后进行shutdown
                    shutdownInLoop();
                }
            }
        }else{
            spdlog::error("TcpConnection::handleWrite");
        }
    }else{
        spdlog::error("TcpConnection::handleWrite");
    }
}
void TcpConnection::handleClose(){
    spdlog::info("fd={} state={}", m_channel->fd(), (int)m_state);
    setState(KDisconnectd);
    m_channel->disableAll();
    // 防止在调用回调时自己被析构
    TcpConnectionPtr guardThis(shared_from_this());
    m_connectionCallback(guardThis);
    if(m_closeCallback){
        m_closeCallback(guardThis);
    }
}
void TcpConnection::handleError(){
    int optval;
    socklen_t optlen = sizeof optval;
    if(::getsockopt(m_channel->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0){
        optval = errno;
    }
    spdlog::error("TcpConnection::handleError [{}] error: {}", m_name.c_str(), optval);
}

void TcpConnection::send(const std::string& buf){
    if(m_state == KConnected){
        if(m_loop->isInLoopThread()){
            sendInLoop(buf.c_str(), buf.size());
        }else{
            m_loop->queueInLoop(std::bind(&TcpConnection::sendInLoop, this, buf.c_str(), buf.size()));
        }
    }
}

void TcpConnection::sendInLoop(const void* message, size_t len){
    ssize_t nwrote = 0;
    ssize_t remaining = len;
    bool faultError = false;

    // 调用过shutdown就不能在调用send
    if(m_state == KDisconnectd){
        spdlog::error("disconnected give up writing");
        return;
    }

    if(!m_channel->isWriting() && m_outputBuffer.readableBytes() == 0){
        // 表示channel第一次开始写数据而且缓冲区没有待发送数据
        nwrote = ::write(m_channel->fd(), message, len);
        if(nwrote >= 0){
            remaining = len - nwrote;
            if(remaining == 0 && m_writeCallback){
                // 表示发送完成了
                m_loop->queueInLoop(std::bind(m_writeCallback, shared_from_this()));
            }
        }else{
            // 发送出错
            nwrote = 0;
            if(errno != EWOULDBLOCK){
                spdlog::error("TcpConnection::sendInLoop");
                if(errno == EPIPE || errno == ECONNRESET){
                    faultError = true;
                }
            }
        }
    }

    if(!faultError && remaining > 0){
        // 一次没发送完成，就把写任务注册到epoll上，让epoll去监听可写事件后再去写
        size_t oldlen = m_outputBuffer.readableBytes();
        if(oldlen + remaining >= m_highWaterMark && oldlen < m_highWaterMark && m_highWaterCallback){
            // 要发送的事情超过高水位了
            m_loop->queueInLoop(std::bind(m_highWaterCallback, shared_from_this(), oldlen + remaining));
        }
        // 将没写的东西写入缓冲区
        m_outputBuffer.append(static_cast<const char*>(message)+ nwrote, remaining);
        if(!m_channel->isWriting()){
            // 开启可写事件监听
            // 在handleWrite中继续发送数据
            m_channel->enableWriting();
        }
    }
}

void TcpConnection::connectEstablished(){
    setState(KConnected);
    m_channel->tie(shared_from_this());
    m_channel->enableReading();
    m_connectionCallback(shared_from_this());
}

void TcpConnection::connectDestoried(){
    if(m_state == KConnected){
        setState(KDisconnectd);
        m_channel->disableAll();
        m_connectionCallback(shared_from_this());
    }
    m_channel->remove();
}

void TcpConnection::shutdown(){
    if(m_state == KConnected){
        setState(KDisconnecting);
        m_loop->runInLoop(std::bind(&TcpConnection::shutdownInLoop, this));
    }
}

void TcpConnection::shutdownInLoop(){
    if(m_channel->isWriting()){
        // 当前outputbuffer中的全部数据都发送完成
        // 会触发EPOLLHUP事件，触发channel中的closeCallback
        // 最终到handleClose
        m_socket->shutDownWrite();
    }
}