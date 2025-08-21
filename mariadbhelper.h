#ifndef MARIADBHELPER_H
#define MARIADBHELPER_H

#include <mariadb/conncpp.hpp>
#include <string.h>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <exception>

class MariadbHelper
{
public:
    ~MariadbHelper();
    static MariadbHelper& getInstance()
    {
        static MariadbHelper instance;
        return instance;
    }
    // 单例函数常规操作，删除赋值和构造
    MariadbHelper(const MariadbHelper&) = delete;
    MariadbHelper &operator=(const MariadbHelper&) = delete;
    void init(const std::string& user, const std::string& password, const std::string& host, unsigned int m_port=3306, std::string database="violet", int poolSize = 7);
    sql::Connection* getConnection();
    sql::Connection* createNewConnection();
    void releaseConnection(sql::Connection *conn);
    size_t getFreeConnectionCount();
    std::vector<std::map<std::string, sql::SQLString>> query(sql::Connection *conn, const std::string sql, const std::vector<sql::SQLString> &params);
    bool execute(sql::Connection *conn, const std::string &sql, const std::vector<sql::SQLString> &params);
    uint64_t getLastInsertId(sql::Connection *conn) const;
    std::string getLastError() const;
    void beginTransaction(sql::Connection *conn);
    void commit(sql::Connection *conn);
    void rollback(sql::Connection *conn);

private:
    MariadbHelper() = default;
    std::string m_user, m_password, m_database, m_host;
    unsigned int m_port;
    std::string m_lastError;
    std::queue<std::unique_ptr<sql::Connection>> m_connQueue;
    std::mutex m_mutex;
    std::condition_variable m_condition;
    int m_poolSize;
};

#endif // MARIADBHELPER_H
