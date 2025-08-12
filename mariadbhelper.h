#ifndef MARIADBHELPER_H
#define MARIADBHELPER_H

#include <mariadb/conncpp.hpp>
#include <memory>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <string.h>

class MariadbHelper
{
public:
    MariadbHelper(const std::string &user,
                  const std::string &password,
                  const std::string &database,
                  const std::string &host = "127.0.0.1",
                  unsigned int port = 3306,
                  bool autoConnect = true);
    ~MariadbHelper();
    int connectMariadb();
    void disconnectMariadb();
    bool isConnected();
    std::vector<std::map<std::string, sql::SQLString>> query(const std::string sql, const std::vector<sql::SQLString> &params);
    bool execute(const std::string &sql, const std::vector<sql::SQLString> &params);
    uint64_t getLastInsertId() const;
    std::string getLastError() const;
    void beginTransaction();
    void commit();
    void rollback();

private:
    std::string m_user, m_password, m_database, m_host;
    unsigned int m_port;
    bool m_autoConnect;
    bool m_connected;
    std::string m_lastError;
    std::unique_ptr<sql::Connection> m_conn;
};

#endif // MARIADBHELPER_H
