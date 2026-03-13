#pragma once
#include <hiredis/hiredis.h>
#include <functional>
#include <thread>
#include <string>

using namespace std;

class Redis 
{
public:
    Redis();
    ~Redis();

    // 连接redis服务器
    bool connect();

    // 发布消息
    bool publish(int channel, string message);

    // 订阅消息
    bool subscribe(int channel);

    // 取消订阅消息
    bool unsubscribe(int channel);

    //在独立线程里面接收订阅的消息
    void observer_channel_message();

    // 设置订阅回调函数
    void init_notify_handler(function<void(int,string)> fn);

private:
    // hiredis同步上下文对象，发布上下文
    redisContext* _publish_context;
    // 订阅上下文
    redisContext* _subscribe_context;
    // 回调，当订阅频道有消息时，会调用该回调函数
    function<void(int,string)> _notify_message_handler;


};