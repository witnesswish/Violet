#include <string>
#include <iostream>
#include <string>

#include <vector>
#include <map>
#include <memory>

#include "mariadbhelper.h"


MariadbHelper::~MariadbHelper()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    while (!m_connQueue.empty()) {
        // unique_ptr在出栈时会自动关闭连接，很牛逼，很方便
        m_connQueue.pop();
    }
}

void MariadbHelper::init(const std::string &user, const std::string &password, const std::string &host, unsigned int port, const std::string datebase, int poolSize)
{
    std::lock_guard<std::mutex> lock(m_mutex);
    m_user = user;
    m_password = password;
    m_host = host;
    m_port = port;
    m_database = datebase;
    sql::SQLString url = "jdbc:mariadb://" + m_host + ":" + std::to_string(m_port) + "/" + m_database;
    sql::Properties properties({{"user", m_user}, {"password", m_password}});
    for (int i = 0; i < poolSize; ++i)
    {
        sql::Driver* driver = sql::mariadb::get_driver_instance();
        std::unique_ptr<sql::Connection> conn(driver->connect(url, properties));
        if(!conn)
        {
            continue;
        }
        else
        {
            m_connQueue.push(std::move(conn));
            std::cout<< "mariadb connected, add to mariadb pool" <<std::endl;
        }
        // 按照官方文档，应该是下面这样写，但是我实在是不太习惯这种错误处理，而且如果这样用前面也有代码要改动，先这样吧
        // 等版本大动的时候再统一改
        // try {
        //     sql::Driver* driver = sql::mariadb::get_driver_instance();
        //     std::unique_ptr<sql::Connection> conn(driver->connect(properties));
        //     m_connQueue.push(std::move(conn));
        // } catch (sql::SQLException& e) {
        //     std::cerr << "Error creating connection: " << e.what() << std::endl;
        // }
    }
    m_poolSize = poolSize;
}

sql::Connection *MariadbHelper::createNewConnection()
{
    sql::SQLString url = "jdbc:mariadb://" + m_host + ":" + std::to_string(m_port) + "/" + m_database;
    sql::Properties properties({{"user", m_user}, {"password", m_password}});
    sql::Driver* driver = sql::mariadb::get_driver_instance();
    return driver->connect(url, properties);
}

void MariadbHelper::releaseConnection(sql::Connection *conn)
{
    if(conn == nullptr) return;
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!conn->isValid()) {
        delete conn;
        conn = createNewConnection();
    }
    m_connQueue.push(std::move(std::unique_ptr<sql::Connection>(conn)));
    //通过notify_one()发送的信号有wait接受，有点类似qt的信号槽？
    m_condition.notify_one();
}

size_t MariadbHelper::getFreeConnectionCount()
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_connQueue.size();
}


sql::Connection *MariadbHelper::getConnection()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    // 会等到有连接可以用，通过notify_one发送
    m_condition.wait(lock, [this]() { return !m_connQueue.empty(); });
    std::unique_ptr<sql::Connection> conn = std::move(m_connQueue.front());
    m_connQueue.pop();
    if (conn != nullptr && conn->isValid())
    {
        return conn.release();
    }
    else if(conn != nullptr && !conn->isValid())
    {
        conn.reset(createNewConnection());
        std::cout<< "get conn error,not nullptr but not valid, trying to get new one" <<std::endl;
        return conn.release();
    }
    else
    {
        std::cout<< "get conn error" <<std::endl;
        return nullptr;
    }
    return conn.release();
}

std::vector<std::map<std::string, sql::SQLString>> MariadbHelper::query(sql::Connection *conn, const std::string sql, const std::vector<sql::SQLString> &params)
{
    std::vector<std::map<std::string, sql::SQLString>> result;
    if (conn != nullptr && conn->isValid())
    {
        std::cout << "connection is good on query" << std::endl;
    }
    else
    {
        std::cout << "not connected detected while query" << std::endl;
        return result;
    }
    std::unique_ptr<sql::PreparedStatement> stmnt(conn->prepareStatement(sql));
    for (size_t i = 0; i < params.size(); ++i)
    {
        stmnt->setString(i + 1, params[i]);
    }
    std::unique_ptr<sql::ResultSet> res(stmnt->executeQuery());
    sql::ResultSetMetaData *meta = res->getMetaData();
    unsigned int columns = meta->getColumnCount();
    while (res->next())
    {
        std::map<std::string, sql::SQLString> row;
        for (unsigned int i = 1; i <= columns; ++i)
        {
            std::string columnName = meta->getColumnName(i).c_str();
            row[columnName] = res->getString(i);
        }
        result.push_back(row);
    }
    return result;
}

bool MariadbHelper::execute(sql::Connection *conn, const std::string &sql, const std::vector<sql::SQLString> &params)
{
    if (conn != nullptr && conn->isValid())
    {
        std::cout << "connection is good on query" << std::endl;
    }
    else
    {
        std::cout << "not connected detected while query" << std::endl;
        return false;
    }
    std::unique_ptr<sql::PreparedStatement> stmnt(conn->prepareStatement(sql));
    for (size_t i = 0; i < params.size(); ++i)
    {
        stmnt->setString(i + 1, params[i]);
    }
    stmnt->execute();
    return true;
}

uint64_t MariadbHelper::getLastInsertId(sql::Connection *conn) const
{
    if (conn == nullptr)
    {
        std::cout<< "error on get last insert id " <<std::endl;
        return 0;
    }
    try
    {
        std::unique_ptr<sql::Statement> stmt(conn->createStatement());
        std::unique_ptr<sql::ResultSet> res(stmt->executeQuery("SELECT LAST_INSERT_ID()"));
        if (res->next())
        {
            return res->getUInt64(1);
        }
    }
    catch (sql::SQLException &)
    {
        // 忽略错误
    }
    return 0;
}
std::string MariadbHelper::getLastError() const
{
    return m_lastError;
}

void MariadbHelper::beginTransaction(sql::Connection *conn)
{
    if (conn == nullptr)
    {
        conn->setAutoCommit(false);
    }
    else
    {
        std::cout << "begin transaction error, nothing happened" << std::endl;
    }
}

void MariadbHelper::commit(sql::Connection *conn)
{
    if (conn == nullptr)
    {
        conn->commit();
        conn->setAutoCommit(true);
    }
    else
    {
        std::cout << "commit transaction error, nothing happened" << std::endl;
    }
}

void MariadbHelper::rollback(sql::Connection *conn)
{
    if (conn == nullptr)
    {
        conn->rollback();
        conn->setAutoCommit(true);
    }
    else
    {
        std::cout << "rollback transaction error, nothing happened" << std::endl;
    }
}
