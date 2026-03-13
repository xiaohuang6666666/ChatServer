#pragma once
#include <muduo/net/TcpServer.h>    
#include <muduo/net/EventLoop.h>
#include <functional>   //绑定器 bind
#include <nlohmann/json.hpp>
#include "chatservice.hpp"

using namespace std;
using namespace std::placeholders;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;

// 聊天服务器类
class ChatServer
{
public:
    ChatServer(EventLoop *loop, const InetAddress &addr, const string &name);
    // 启动服务
    void start();
private:
    // 连接建立或断开的回调函数
    void onConnection(const TcpConnectionPtr &conn);
    // 处理用户的读写事件
    void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time);

    TcpServer server_;
    EventLoop *loop_;
};