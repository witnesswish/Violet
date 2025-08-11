#ifndef MARIADBHELPER_H
#define MARIADBHELPER_H

#include <mariadb/conncpp.hpp>
#include <string>
#include <memory>

class MariadbHelper
{
public:
    MariadbHelper(const std::string &user, const std::string &password, const std::string &database, const std::string &host="127.0.0.1" unsigned int port=3306);
private:
    std::string m_user, m_password, m_databases, m_host;
    unsigned int m_port;
    std::shared_ptr<sql::Connection> m_conn;
};

#endif // MARIADBHELPER_H
