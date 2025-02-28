#include <iostream>
#include "mysql_pool.hpp"
#include <vector>
using namespace std;

void clear()
{
    MysqlConn conn;
    bool flag = conn.connect("127.0.0.1", "alice", "alice", "test_db");
    conn.query("delete from test_table where name like 'test%';");
}

// 这个测试函数测试数据库在单线程环境下的耗时
void test1(int test_num)
{
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();

    for (int i = 0; i < test_num; i++)
    {
        MysqlConn mysql_conn;
        bool flag = mysql_conn.connect("127.0.0.1", "alice", "alice", "test_db");
        string test_str = "insert into test_table (name, num) values (\'";
        test_str.append("test1_user" + to_string(i + 1));
        test_str.append("\', '10');");
        // cout << test_str << endl;
        flag = mysql_conn.insert(test_str);
    }

    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    long long time_ns = (end - begin).count();
    long long time_ms = chrono::duration_cast<chrono::milliseconds>((end - begin)).count();
    cout << "time ms: " << time_ms << endl;
    cout << "time ns " << time_ns << endl;
    return;
}

// 这个函数用来测试数据库 连接池 在单线程下的耗时
void test2(int test_num)
{
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();
    MysqlPool &pool = MysqlPool::instance();
    for (int i = 0; i < test_num; i++)
    {
        shared_ptr<MysqlConn> conn = pool.get_conn();

        string test_str = "insert into test_table (name, num) values (\'";
        test_str.append("test2_user" + to_string(i + 1));
        test_str.append("\', '10');");
        // cout << test_str << endl;
        bool flag = conn->insert(test_str);
    }
    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    long long time_ns = (end - begin).count();
    long long time_ms = chrono::duration_cast<chrono::milliseconds>((end - begin)).count();
    cout << "time ms: " << time_ms << endl;
    cout << "time ns " << time_ns << endl;
}

// 这个函数用来测试数据库 连接池 在多线程下的耗时
void test3(int test_num)
{
    chrono::steady_clock::time_point begin = chrono::steady_clock::now();
    int thread_num = 5;
    vector<thread *> threads;
    for (int i = 0; i < thread_num; i++)
    {
        thread *t = new thread(test2, test_num / thread_num);
        threads.push_back(t);
    }

    for (int i = 0; i < thread_num; i++)
    {
        threads[i]->join();
    }

    chrono::steady_clock::time_point end = chrono::steady_clock::now();
    long long time_ns = (end - begin).count();
    long long time_ms = chrono::duration_cast<chrono::milliseconds>((end - begin)).count();
    cout << "test3 time ms: " << time_ms << endl;
    cout << "test3 time ns " << time_ns << endl;
}

int main()
{

    // MysqlPool::instance();
    clear();

    /*
    test1 time ms: 274161
    test1 time ns 274161496119
    
    test2 time ms: 34224
    test2 time ns 34224623048

    test3 time ms: 8859
    test3time ns 8859214160
    */
    // test1(5000);
    // test2(5000);
    test3(5000);

    cout << "end" << endl;

    return 0;
}