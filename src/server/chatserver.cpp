#include "chatserver.hpp"

ChatServer::ChatServer(EventLoop *loop, const InetAddress &addr, const string &name)
    :server_(loop,addr,name),loop_(loop)
{
    // 给服务器注册用户用户连接的创建和断开回调
    server_.setConnectionCallback(bind(&ChatServer::onConnection, this, _1));
    // 给服务器注册用户读写事件回调
    server_.setMessageCallback(bind(&ChatServer::onMessage, this, _1, _2, _3));
    server_.setThreadNum(4);
}

void ChatServer::start()
{
    server_.start();
}

// 连接建立或断开的回调函数
void ChatServer::onConnection(const TcpConnectionPtr &conn)
{
    if(!conn->connected())
    {
        ChatService::instance()->clientCloseException(conn);
        conn->shutdown();
    }
}
// 处理用户的读写事件
void ChatServer::onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp time)
{
    string msg = buf->retrieveAllAsString();
    // 数据的反序列化
    json js = json::parse(msg);
    // 通过js["msgid"]获取业务handler
    auto msgHandler = ChatService::instance()->getHandler(js["msgid"].get<int>());
    msgHandler(conn,js,time);
}