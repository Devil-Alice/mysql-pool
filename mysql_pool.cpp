#include "mysql_pool.hpp"
#include <fstream>
#include "json/json.h"

MysqlPool::MysqlPool()
{
    // 解析json配置
    bool flag = parse_config();
    if (flag == false)
        std::cout << "parse failed" << std::endl;

    // 解析后，创建min_size个链接，并将配置应用到mysql_conn上
    for (int i = 0; i < min_size_; i++)
    {
        bool flag = create_conn();
    }

    // 初始化生产和回收的线程
    producer_ = new std::thread(&MysqlPool::produce, this);
    recycler_ = new std::thread(&MysqlPool::recycle, this);
    producer_->detach();
    recycler_->detach();
}

bool MysqlPool::create_conn()
{
    MysqlConn *mysql_conn = new MysqlConn();
    bool flag = mysql_conn->connect(host_, user_name_, password_, db_name_, port_);
    if (flag == false)
        return false;

    mysql_conns_.push(mysql_conn);

    update_last_alive_time_ms();

    //  生产完了一个链接，要通知其他阻塞在cond的方法，将其唤醒
    consume_cond_.notify_all();

    return true;
}

MysqlPool::~MysqlPool()
{
    while (!mysql_conns_.empty())
    {
        MysqlConn *mysql_conn = mysql_conns_.front();
        mysql_conns_.pop();
        delete mysql_conn;
    }

    is_quit= true;
    consume_cond_.notify_all();
    produce_cond_.notify_all();
}

MysqlPool &MysqlPool::instance()
{
    static MysqlPool mysql_pool;
    return mysql_pool;
}

std::shared_ptr<MysqlConn> MysqlPool::get_conn()
{
    std::unique_lock<std::mutex> locker(mutex_);
    std::chrono::milliseconds timeout(timeout_ms_);
    while (mysql_conns_.empty())
    {
        if (std::cv_status::timeout == consume_cond_.wait_for(locker, timeout))
            continue;
    }

    // 智能指针，第一个参数时要管理的指针，第二个参数时智能指针的删除器，需要一个函数，此处使用匿函数，并且在智能指针删除时，自动将conn归还至mysql_conns
    std::shared_ptr<MysqlConn> conn_prt(mysql_conns_.front(), [this](MysqlConn *conn)
                                        {
        // std::lock_guard<std::mutex> guard(mutex_);
        mutex_.lock();
        mysql_conns_.push(conn);
        update_last_alive_time_ms(); 
        mutex_.unlock(); });

    mysql_conns_.pop();

    // 取出一个连接后，需要通知生产者生产
    produce_cond_.notify_all();

    return conn_prt;
}

bool MysqlPool::parse_config()
{
    // 创建一个input file stream
    std::ifstream ifs("./db_pool_config.json");
    Json::Reader reader;
    Json::Value root;
    // parse将读取的json数据存在root中
    bool flag = reader.parse(ifs, root);
    if (flag == false || !root.isObject())
        return false;

    host_ = root["host"].asString();
    user_name_ = root["user_name"].asString();
    password_ = root["password"].asString();
    db_name_ = root["db_name"].asString();
    port_ = root["port"].asInt();
    min_size_ = root["min_size"].asInt();
    max_size_ = root["max_size"].asInt();
    timeout_ms_ = root["timeout_ms"].asInt();
    max_idle_time_ms_ = root["max_idle_time_ms"].asInt();

    return true;
}

void MysqlPool::produce()
{
    while (is_quit)
    {
        // 包装mutex
        std::unique_lock<std::mutex> locker(mutex_);
        // 判断现在连接池中的链接是否超过了最大容量，超过就等待
        while (mysql_conns_.size() >= max_size_)
        {
            produce_cond_.wait(locker);
        }

        create_conn();
        printf("produced a connection, current size: %d\r\n", (int)mysql_conns_.size());
    }
}

void MysqlPool::recycle()
{
    while (is_quit)
    {
        // 每三秒执行一次检测
        std::this_thread::sleep_for(std::chrono::milliseconds(3000));
        while (get_alive_time_ms() >= max_idle_time_ms_)
        {
            if (mysql_conns_.size() >= min_size_)
            {
                printf("recycling, current size: %d\r\n", (int)mysql_conns_.size());
                mutex_.lock();
                MysqlConn *mysql_conn = mysql_conns_.front();
                mysql_conns_.pop();
                delete mysql_conn;
                mutex_.unlock();
            }
        }
    }
}

void MysqlPool::update_last_alive_time_ms()
{
    last_alive_time_ms_ = std::chrono::steady_clock::now();
}

long long MysqlPool::get_alive_time_ms()
{
    // 对于steady_clock::time_point的计算返回的是chrono::nanoseconds
    std::chrono::nanoseconds time_ns = std::chrono::steady_clock::now() - last_alive_time_ms_;
    // 如果希望将计算的纳秒结果转换就需要用到duration_cast
    std::chrono::milliseconds time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(time_ns);
    // 最后转为int类型，需要用count返回这个对象中的计数，也就是毫秒的数量
    return time_ms.count();
}
