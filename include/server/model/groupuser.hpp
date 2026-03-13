#pragma once
#include <string>
#include <vector>
#include "user.hpp"
using namespace std;

// 群组用户，多了一个role属性，表示用户在群中的角色
class GroupUser : public User
{
public:
    void setRole(string role) { this->role = role; }
    string getRole() const { return this->role; }
private:
    string role;    // 用户在群中的角色：creator, normal
};