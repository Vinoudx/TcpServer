// #include <muduo/net/TcpServer.h>
// #include <muduo/net/EventLoop.h>
// using namespace muduo::net;
// using namespace muduo;

#include "TcpServer.hpp"

#include <iostream>
#include <spdlog/spdlog.h>



void onMessage(const TcpConnectionPtr& conn, Buffer* buf, TimeStamp time){
    auto message = buf->retriveAllAsString();
    conn->send(message);
}

int main(){
    spdlog::set_level(spdlog::level::info);
    
    InetAddress addr("127.0.0.1", 6666);
    EventLoop loop;
    TcpServer server(&loop, addr, "server");
    server.setThreadNum(4);
    server.setMessageCallback(onMessage);
    spdlog::info("start");
    server.start();
    loop.loop();
}