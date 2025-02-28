#include "mysql_conn.hpp"

MysqlConn::MysqlConn()
{
    // 这个mysql_init的参数不需要指定，nullptr就行
    mysql_ = mysql_init(nullptr);
    if (mysql_ == nullptr)
    {
        std::cerr << "mysql_init failed: " << mysql_error(mysql_) << std::endl;
        exit(1);
    }
}

MysqlConn::~MysqlConn()
{

    free_result();

    if (mysql_ != nullptr)
        mysql_close(mysql_);
}

bool MysqlConn::connect(const std::string &host, const std::string &user_name, const std::string &password, const std::string &db_name, unsigned int port)
{
    mysql_ = mysql_real_connect(mysql_, host.data(), user_name.data(), password.data(), db_name.data(), port, nullptr, 0);
    if (mysql_ == nullptr)
    {
        std::cerr << "mysql_real_connect error: " << mysql_error(mysql_) << std::endl;
        mysql_close(mysql_);
        return false;
    }
    mysql_set_character_set(mysql_, "utf8");
    return true;
}

bool MysqlConn::insert(std::string sql)
{
    int ret = mysql_query(mysql_, sql.data());
    if (ret != 0)
        return false;
    return true;
}

bool MysqlConn::query(std::string sql)
{
    free_result();
    int ret = mysql_query(mysql_, sql.data());
    if (ret != 0)
        return false;
    // mysql_store_result将查询的结果报存在MYSQL_RES *result_中
    result_ = mysql_store_result(mysql_);

    return true;
}

bool MysqlConn::next_row()
{
    if (mysql_ == nullptr)
        return false;
    // mysql_fetch_row将结果中的一行结果返回存在MYSQL_ROW(实际上是字符串数组) row_中，每次调用会自动返回下一行
    row_ = mysql_fetch_row(result_);
    if (row_ == nullptr)
    {
        // row为空，说明查询完了
        free_result();
        return false;
    }
    return true;
}

std::string MysqlConn::field_value(int field_idx)
{
    // 这里mysql_num_fields获取一行结果的列数，也就是字段数
    int fields_cnt = mysql_num_fields(result_);
    if (row_ == nullptr || field_idx >= fields_cnt || field_idx < 0)
        return "";
    // mysql_fetch_lengths获取的是子段的长度，用于防治结果中含有\0导致的误判长度
    int field_len = mysql_fetch_lengths(result_)[field_idx];
    return std::string(row_[field_idx], field_len);
}

bool MysqlConn::transaction()
{
    return mysql_autocommit(mysql_, false);
}

bool MysqlConn::commit()
{
    return mysql_commit(mysql_);
}

bool MysqlConn::rollback()
{
    return mysql_rollback(mysql_);
}

void MysqlConn::free_result()
{
    if (result_ != nullptr)
    {
        mysql_free_result(result_);
        result_ = nullptr;
    }
}
