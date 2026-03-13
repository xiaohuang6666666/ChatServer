#pragma once
#include <string>
#include <vector>
#include "group.hpp"
#include "groupuser.hpp"
using namespace std;
// 群组模型类
class GroupModel
{
public:
    // 创建群组
    bool createGroup(Group &group);
    // 加入群组
    void addGroup(int userid, int groupid, string role);
    // 查询用户所在的群组信息
    vector<Group> queryGroups(int userid);
    // 查询群组用户的id和角色信息
    vector<int> queryGroupUsers(int userid, int groupid);

private:
    
};