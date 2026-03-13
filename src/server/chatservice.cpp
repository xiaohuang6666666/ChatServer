#include "chatservice.hpp"

// 注册消息以及对应的Handle回调操作
ChatService::ChatService()
{
    _msgHandlerMap.insert({LOGIN_MSG,std::bind(&ChatService::login,this,_1,_2,_3)});
    _msgHandlerMap.insert({REG_MSG,std::bind(&ChatService::reg,this,_1,_2,_3)});
    _msgHandlerMap.insert({ONE_CHAT_MSG,std::bind(&ChatService::oneChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_FRIEND_MSG,std::bind(&ChatService::addFriend,this,_1,_2,_3)});
    _msgHandlerMap.insert({CREATE_GROUP_MSG,std::bind(&ChatService::createGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({ADD_GROUP_MSG,std::bind(&ChatService::addGroup,this,_1,_2,_3)});
    _msgHandlerMap.insert({GROUP_CHAT_MSG,std::bind(&ChatService::groupChat,this,_1,_2,_3)});
    _msgHandlerMap.insert({LOGINOUT_MSG,std::bind(&ChatService::loginout,this,_1,_2,_3)});

    // 初始化redis
    if(redis_.connect())
    {
        // 设置上报消息的回调函数
        redis_.init_notify_handler(std::bind(&ChatService::handleRedisSubscribeMessage,this,_1,_2));
    }
}
ChatService* ChatService::instance()
{
    // 静态变量保证后续调用都返回同一个对象
    static ChatService service;
    return &service;
}
void ChatService::login(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int id = js["id"];
    string pwd = js["password"];
    User user = usermodel_.query(id);
    if(user.getId() == id && user.getPwd() == pwd)
    {
        if(user.getState() == "online")
        {
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 2;  
            response["errmsg"] = "该账号已经登陆，请重新登陆其他账号！";
            conn->send(response.dump());    //将回应信息发送给客户端
        }else
        {
            // 1、登陆成功
            {
                lock_guard<mutex> lock(_connMutex);
                _userConnMap.insert({id,conn}); //记录连接信息
            }
            // 登录成功后，订阅id对应的channel
            redis_.subscribe(id);

            user.setState("online");        //更新状态
            usermodel_.updateState(user);
            json response;
            response["msgid"] = LOGIN_MSG_ACK;
            response["errno"] = 0;  
            response["id"] = user.getId();
            response["name"] = user.getName();
            // 2、查询是否有离线消息
            vector<string> vec = offlinemsgmodel_.query(id);
            if(!vec.empty())
            {
                // 容器内的消息可以直接传给 json
                response["offlinemsg"] = vec;
                offlinemsgmodel_.remove(id);
            }
            // 3、查询该用户好友信息并返回
            vector<User> friends = friendmodel_.query(id);
            if(!friends.empty())
            {
                vector<string> vec2;
                for(User &user : friends)
                {
                    json js;
                    js["id"] = user.getId();
                    js["name"] = user.getName();
                    js["state"] = user.getState();
                    vec2.push_back(js.dump());
                }
                response["friends"] = vec2;
            }
            // 4、查询该用户群组信息并返回
            vector<Group> groups = groupmodel_.queryGroups(id);
            if(!groups.empty())
            {
                vector<string> vec2;
                for(Group &group : groups)
                {
                    json js;
                    js["id"] = group.getId();
                    js["groupname"] = group.getName();
                    js["groupdesc"] = group.getDesc();
                    vector<string> vec3;
                    for(GroupUser &user : group.getUsers())
                    {
                        json js2;
                        js2["id"] = user.getId();
                        js2["name"] = user.getName();
                        js2["state"] = user.getState();
                        js2["role"] = user.getRole();
                        vec3.push_back(js2.dump());
                    }
                    js["users"] = vec3;
                    vec2.push_back(js.dump());
                }
                response["groups"] = vec2;
            }
            conn->send(response.dump()); 
        } 
    }else
    {
        json response;
        response["msgid"] = LOGIN_MSG_ACK;
        response["errno"] = 1;  
        response["errmsg"] = "用户名或者密码错误！";
        conn->send(response.dump());    
    }
}

void ChatService::reg(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    // 两个字段：name，password
    string name = js["name"];
    string pwd = js["password"];
    User user;
    user.setName(name);
    user.setPwd(pwd);
    if(usermodel_.insert(user))
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 0;  //可以检测errno是否为0,来判断注册是否成功
        response["id"] = user.getId();
        conn->send(response.dump());    //将回应信息发送给客户端
    }else
    {
        json response;
        response["msgid"] = REG_MSG_ACK;
        response["errno"] = 1;
        conn->send(response.dump()); 
    }
}

void ChatService::oneChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int toid = js["to"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(toid);
        if(it != _userConnMap.end())
        {
            //toid 对于本服务器在线,转发消息
            it->second->send(js.dump());
            return;
        }
    }
    // 查询toid是否在线
    User user = usermodel_.query(toid);
    if(user.getState() == "online")
    {
        redis_.publish(toid,js.dump());
        return;
    }

    //离线,储存消息(一整条消息)
    offlinemsgmodel_.insert(toid,js.dump());  
}
void ChatService::addFriend(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["userid"].get<int>();
    int friendid = js["friendid"].get<int>();
    // 存储好友信息
    friendmodel_.insert(userid,friendid);
}

// 创建群组业务(只是建了一个群，群里只有创建者)
void ChatService::createGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    string groupname = js["groupname"];
    string groupdesc = js["groupdesc"];
    Group group(-1,groupname,groupdesc);
    if(groupmodel_.createGroup(group))
    {
        // 储存建群者信息
        groupmodel_.addGroup(userid,group.getId(),"creator");
        LOG_INFO << "用户:" << userid << "创建群组:" << group.getId() << "成功！";
    }
    else
    {
        LOG_ERROR << "用户:" << userid << "创建群组失败！";
    }
}
// 加入群组业务
void ChatService::addGroup(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    groupmodel_.addGroup(userid,groupid,"normal");
    LOG_INFO << "用户:" << userid << "加入群组:" << groupid << "成功！";
}
// 群组聊天业务
void ChatService::groupChat(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    int groupid = js["groupid"].get<int>();
    vector<int> useridVec = groupmodel_.queryGroupUsers(userid,groupid);
    {
        lock_guard<mutex> lock(_connMutex);
        for(int id : useridVec)
        {
            auto it = _userConnMap.find(id);
            if(it != _userConnMap.end())
            {
                // 转发消息
                it->second->send(js.dump());
            }else
            {
                if(usermodel_.query(id).getState() == "online")
                {
                    // 订阅了id频道但不在_userConnMap中，说明用户在其他服务器上在线
                    redis_.publish(id,js.dump());
                    continue;
                }else
                {
                    // 离线,储存消息(一整条消息)
                    offlinemsgmodel_.insert(id,js.dump());
                }
            }
        }
    }
}

// 注销业务
void ChatService::loginout(const TcpConnectionPtr &conn, json &js, Timestamp time)
{
    int userid = js["id"].get<int>();
    {
        lock_guard<mutex> lock(_connMutex);
        auto it = _userConnMap.find(userid);
        if(it->second == conn && it != _userConnMap.end())
        {
            _userConnMap.erase(it);
        }
    }
    // 注销成功后，取消订阅id对应的channel
    redis_.unsubscribe(userid);

    User user = usermodel_.query(userid);
    user.setState("offline");
    usermodel_.updateState(user);
        
}

// 处理redis订阅消息
void ChatService::handleRedisSubscribeMessage(int channel, string message)
{
    // 处理redis订阅消息的逻辑
    lock_guard<mutex> lock(_connMutex);
    auto it = _userConnMap.find(channel);
    if(it != _userConnMap.end())
    {
        // 转发消息
        it->second->send(message);
        return;
    }

    // 储存离线消息
    offlinemsgmodel_.insert(channel,message);
}

MsgHandler ChatService::getHandler(int msgid)
{
    auto it = _msgHandlerMap.find(msgid);
    if(it == _msgHandlerMap.end())
    {
        // 返回默认的处理器(用来打印错误的日志)
        return [=](const TcpConnectionPtr &conn, json &js, Timestamp time){
            LOG_ERROR << "msgid:" << msgid << "can not find handler!";
        };
    }
    return _msgHandlerMap[msgid];
}
void ChatService::clientCloseException(const TcpConnectionPtr &conn)
{
    User user;
    {
        lock_guard<mutex> lock(_connMutex);
        for(auto it = _userConnMap.begin();it != _userConnMap.end();it++)
        {
            if(it->second == conn)
            {
                user.setId(it->first);
                _userConnMap.erase(it);
                break;
            }
        }
    }
    // 客户端异常退出后，取消订阅id对应的channel
    redis_.unsubscribe(user.getId());

    if(user.getId() != -1)
    {
        user.setState("offline");
        usermodel_.updateState(user);
    }
}

void ChatService::reset()
{
    // 重置所有状态信息
    usermodel_.resetState();
}