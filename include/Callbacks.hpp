#ifndef _CALLBACKS_
#define _CALLBACKS_

#include <memory>
#include <functional>

#include "Timestamp.hpp"

class Buffer;
class TcpConnection;

using TcpConnectionPtr = std::shared_ptr<TcpConnection>;
using ConnectionCallback = std::function<void(const TcpConnectionPtr&)>;
using WriteCompleteCallback = std::function<void(const TcpConnectionPtr&)>;
using CloseCallback = std::function<void(const TcpConnectionPtr&)>;
using HighWaterMarkCallback = std::function<void(const TcpConnectionPtr&, int)>;

using MessageCallback = std::function<void(const TcpConnectionPtr&, Buffer*, TimeStamp)>;

inline void defaultConnectionCallback(const TcpConnectionPtr& conn){};
inline void defaultMessageCallback(const TcpConnectionPtr& conn, Buffer* buf, TimeStamp time){};

#endif