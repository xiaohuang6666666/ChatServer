#pragma once
#include <muduo/net/TcpConnection.h>
#include <unordered_map>
#include <functional>
#include <muduo/base/Logging.h>     //muduo库自带的日志类
#include <nlohmann/json.hpp>
#include "public.hpp"
#include "usermodel.hpp"            //包含user.hpp
#include "friendmodel.hpp"
#include "offlinemessagemodel.hpp"
#include "groupmodel.hpp"
#include "redis.hpp"
#include <mutex>

using namespace std;
using namespace muduo;
using namespace muduo::net;
using json = nlohmann::json;
using MsgHandler = std::function<void(const TcpConnectionPtr &conn, json &js, Timestamp time)>;
// 聊天服务器业务类
class ChatService
{
public:
    ChatService();
    // 获取单例对象的接口函数
    static ChatService* instance();
    // 处理登陆业务
    void login(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 处理注册业务
    void reg(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 一对一聊天业务
    void oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 添加好友业务
    void addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 创建群组业务
    void createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 加入群组业务
    void addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 群组聊天业务
    void groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time);
    // 注销业务
    void loginout(const TcpConnectionPtr &conn, json &js, Timestamp time);

    // 处理redis订阅消息
    void handleRedisSubscribeMessage(int channel, string message);

    // 获取消息对应的处理器
    MsgHandler getHandler(int msgid);
    // 处理客户端异常退出
    void clientCloseException(const TcpConnectionPtr &conn);
    // 处理服务器异常,重置操作
    void reset();

private:
    // 存储消息id和对应的业务处理方法
    unordered_map<int,MsgHandler> _msgHandlerMap;

    // 存储在线用户的通信连接(线程安全)
    unordered_map<int,TcpConnectionPtr> _userConnMap;

    // 互斥锁，保证_userConnMap线程安全
    mutex _connMutex;

    // 数据操作类对象(这里只是依赖UserModel类，不涉及数据库操作)
    UserModel usermodel_;
    OfflineMsgModel offlinemsgmodel_;
    FriendModel friendmodel_;
    GroupModel groupmodel_;

    // redis操作对象
    Redis redis_;


};