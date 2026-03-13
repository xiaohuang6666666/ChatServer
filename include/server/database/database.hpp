#pragma once
#include <string>
#include <mysql/mysql.h>
#include <muduo/base/Logging.h>

using namespace std;

// 数据库配置信息
static string server = "127.0.0.1";
static string user = "root";
static string password = "20031216";
static string dbname = "chat";

// 数据库操作类
class MySQL
{
public:
    // 初始化数据库连接
    MySQL()
    {
        _conn = mysql_init(nullptr);
    }
    // 释放数据库连接资源
    ~MySQL()
    {
        if (_conn != nullptr)
        mysql_close(_conn);
    }
    // 连接数据库
    bool connect()
    {
        MYSQL *p = mysql_real_connect(_conn, server.c_str(), user.c_str(),
            password.c_str(), dbname.c_str(), 3306, nullptr, 0);
        if (p != nullptr)
        {
            // C和C++代码默认编码字符是ASCII码，如果不设置，从MYSQL拉下来的中文会产生问号
            mysql_query(_conn, "set names gbk");
            LOG_INFO << "连接 mysql 成功！";
        }else
        {
            LOG_INFO << "连接 mysql 失败！";
        }
        return p;
    }
    // 更新操作
    // 更新包括增加，删减，替换等，主要是看 sql 是什么命令
    // mysql_query 函数是让 MYSQL 数据库执行 sql 命令
    bool update(string sql)
    {
        int ret = mysql_query(_conn, sql.c_str());
        if (ret != 0)
        {
            // 核心：打印 MySQL 具体错误信息（错误码 + 描述）
            LOG_INFO << __FILE__ << ":" << __LINE__ 
                 << " SQL执行失败！错误码：" << ret 
                 << " 错误信息：" << mysql_error(_conn)
                 << " SQL语句：" << sql;
            return false;
        }
        return true;
    }
    // 查询操作
    // 同上，只不过查询需要返回 MYSQL_RES
    MYSQL_RES* query(string sql)
    {
        if (mysql_query(_conn, sql.c_str()))
        {
            LOG_INFO << __FILE__ << ":" << __LINE__ << ":"<< sql << "查询失败!";
            return nullptr;
        }
        return mysql_use_result(_conn);
    }
    // 获取连接
    MYSQL* getConnection(){return _conn;}
private:
    MYSQL *_conn;
};