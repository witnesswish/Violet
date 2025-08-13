#ifndef LOGINCENTER_H
#define LOGINCENTER_H

#include <cstdint>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <string>

#include "mariadbhelper.h"
#include "protocol.h"

//mariadb 连接信息
struct Madb
{
    std::string m_user;
    std::string m_password;
    std::string m_database;
    std::string m_host;
    unsigned int m_port;
};

//用户信息结构体
struct User
{
    std::string username;
    std::string password;
    std::string salt;
    std::vector<std::string> friends;
    std::vector<std::string> groups;
};

class LoginCenter
{
public:
    LoginCenter();
public:
    int vregister(std::string username, std::string password, std::string email, std::string salt);
    int vlogin(std::string username, std::string password, std::string &userInfo);
    Madb setMariadb();
private:
    MariadbHelper mariadb;
    std::string serializeTwoVector(const std::vector<std::string>& vec1, const std::vector<std::string>& vec2);
    std::pair<std::vector<std::string>, std::vector<std::string>> deserializeVectors(const std::string& data);
};

#endif // LOGINCENTER_H
