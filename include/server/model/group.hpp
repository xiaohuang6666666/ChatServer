#pragma once
#include <string>
#include <vector>
#include "groupuser.hpp"
using namespace std;

class Group
{
public:
    Group(int id = -1, string name = " ", string desc = " "):
        id(id), name(name), desc(desc) {}

    void setId(int id) { this->id = id; }
    void setName(const string &name) { this->name = name; }
    void setDesc(const string &desc) { this->desc = desc; }

    int getId() const { return id; }
    string getName() const { return name; }
    string getDesc() const { return desc; }
    vector<GroupUser> &getUsers() { return this->users; }

private:
    int id;         // 群id
    string name;    // 群名称
    string desc;    // 群描述
    vector<GroupUser> users;    // 群成员列表

};
