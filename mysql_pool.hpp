#pragma once
#include <queue>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "mysql_conn.hpp"

class MysqlPool
{
private:
    // 数据库连接配置信息
    std::string host_;
    std::string user_name_;
    std::string password_;
    std::string db_name_;
    unsigned int port_ = 3306;
    //这个队列用于存放连接，获取连接会从这个队列中pop
    std::queue<MysqlConn *> mysql_conns_;

    // 连接池配置信息
    int min_size_;
    int max_size_;
    int timeout_ms_;
    int max_idle_time_ms_;

    // 连接池状态信息
    bool is_quit = false;
    int busy_num_;
    int idle_num_;
    int request_num_;

    //steady_clock相对时钟，记录时间点
    std::chrono::steady_clock::time_point last_alive_time_ms_;

    // 管理线程
    std::mutex mutex_;
    std::condition_variable produce_cond_;
    std::condition_variable consume_cond_;
    std::thread *producer_ = nullptr;
    std::thread *recycler_ = nullptr;

private:
    MysqlPool();
    bool create_conn();
    bool parse_config();
    void produce();
    void recycle();
    void update_last_alive_time_ms();
    long long get_alive_time_ms();

public:
    ~MysqlPool();
    static MysqlPool &instance();
    std::shared_ptr<MysqlConn> get_conn();
};