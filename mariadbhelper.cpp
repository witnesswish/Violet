#include "mariadbhelper.h"

MariadbHelper::MariadbHelper(const std::string &user, const std::string &password, const std::string &database, const std::string &host="127.0.0.1" unsigned int port=3306)
    : m_user(user), m_password(password), m_database(database), m_host(host), m_port(port){}


