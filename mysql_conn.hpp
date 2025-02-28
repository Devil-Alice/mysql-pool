#pragma once
#include <iostream>
#include <mysql/mysql.h>

class MysqlConn
{
private:
    MYSQL *mysql_ = nullptr;
    // result_ 需要手动释放
    MYSQL_RES *result_ = nullptr;
    // 一行结果，指向result中的数据，当result_释放，row_也会自动释放
    MYSQL_ROW row_ = nullptr;

public:
    MysqlConn(/* args */);
    ~MysqlConn();

    bool connect(const std::string &host,const  std::string &user_name,const  std::string &password,const  std::string &db_name, unsigned int port = 3306);
    bool insert(std::string sql);
    bool query(std::string sql);
    bool next_row();
    std::string field_value(int field_idx);

    // 开启事务
    bool transaction();
    // 提交事务
    bool commit();
    // 回滚事务
    bool rollback();

private:
    void free_result();
};
