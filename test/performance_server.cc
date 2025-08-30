// #include <muduo/net/TcpServer.h>
// #include <muduo/net/EventLoop.h>
// using namespace muduo::net;
// using namespace muduo;

#include "TcpServer.hpp"

#include <iostream>
#include <spdlog/spdlog.h>

static int countms = 0;
static int counts = 0;

void funcms(){
    // std::cout<<"click ms "<< countms++ <<std::endl;
}

void funcs(){
    // std::cout<<"click s "<< counts++ <<std::endl;
}

void onMessage(const TcpConnectionPtr& conn, Buffer* buf, TimeStamp time){
    auto message = buf->retriveAllAsString();
    conn->send(message);
}

int main(){
    spdlog::set_level(spdlog::level::debug);
    
    InetAddress addr("127.0.0.1", 6666);
    EventLoop loop;
    TcpServer server(&loop, addr, "server");
    server.setThreadNum(4);
    server.setMessageCallback(onMessage);
    server.timer(funcms, 100, true, -1, 100);
    server.timer(funcs, 1000, true, -1, 1000);
    spdlog::info("start");
    server.start();
    loop.loop();
}