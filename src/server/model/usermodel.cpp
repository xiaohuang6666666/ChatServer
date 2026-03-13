#include "usermodel.hpp"
#include "database.hpp"

bool UserModel::insert(User &user)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql,"insert into user(name,password,state) values('%s','%s','%s')",
        user.getName().c_str(),user.getPwd().c_str(),user.getState().c_str());
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            LOG_INFO << "add User success => sql:" << sql;
            //类外无法获取private成员
            user.setId(mysql_insert_id(mysql.getConnection()));
            return true;
        }
        LOG_INFO << "add User error => sql:" << sql;
    }
    LOG_INFO << "connect and add User error => sql:" << sql;
    return false;
}

User UserModel::query(int id)
{
    // 组装sql语句
    char sql[1024] = {0};
    sprintf(sql,"select * from user where id = %d",id);
    MySQL mysql;
    if(mysql.connect())
    {
        MYSQL_RES *res = mysql.query(sql);
        if(res != nullptr)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            if(row != nullptr)
            {
                User user;
                user.setId(atoi(row[0]));
                user.setName(row[1]);
                user.setPwd(row[2]);
                user.setState(row[3]);
                mysql_free_result(res);     // 释放资源
                return user;
            }
        }
    }  
    return User();  //返回默认构造的空User 
}

bool UserModel::updateState(User user)
{
    char sql[1024] = {0};
    sprintf(sql,"update user set state = '%s' where id = %d",user.getState().c_str(),user.getId());
    MySQL mysql;
    if(mysql.connect())
    {
        if(mysql.update(sql))
        {
            return true;
        }
    }
    return false;
}

void UserModel::resetState()
{
    char sql[1024] = "update user set state = 'offline' where state = 'online'";
    MySQL mysql;
    if(mysql.connect())
    {
        mysql.update(sql);
    }
}