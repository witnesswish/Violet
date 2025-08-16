#ifndef LOGINCENTER_H
#define LOGINCENTER_H

#include <cstdint>
#include <cstring>
#include <arpa/inet.h>
#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <set>
#include <sys/socket.h>
#include <list>

#include "mariadbhelper.h"
#include "protocol.h"
#include "redishelper.h"

// mariadb 连接信息
struct Madb
{
    std::string m_user;
    std::string m_password;
    std::string m_database;
    std::string m_host;
    unsigned int m_port;
};

// 用户信息结构体
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
    ~LoginCenter();

public:
    int vregister(std::string username, std::string password, std::string email, std::string salt);
    int vlogin(int fd, std::string username, std::string password, std::string &userInfo);
    int vaddFriend(std::string requestName, std::string firName);
    int vaddGroup(std::string requestName, std::string groupName);
    int vcreateGroup(std::string reqName, std::string groupName);
    int vprivateChat(std::string firName);
    void vgroupChat(int fd, std::string requestName, std::string groupName, std::string content);
    void vofflineHandle(int fd);
    Madb setMariadb();

private:
    SRHelper sr;
    MariadbHelper mariadb;
    RedisHelper redis;
    /**
     * @brief onlineGUMap 群组在线用户，登录的时候加入fd，下线的时候删除fd，
     * @brief onlineUserFriend 用户的在线好友，
     * 用户上线-》广播登录-》存入在线好友列表
     * 用户离线-》广播下线-》删除好友列表
     */
    static std::map<std::string, std::set<int>> onlineGUMap;
    static std::map<std::string, std::list<int>> onlineUserFriend;
    static std::map<int, std::string> onlineUser;
private:
    std::string serializeTwoVector(const std::vector<std::string> &vec1, const std::vector<std::string> &vec2);
    std::pair<std::vector<std::string>, std::vector<std::string>> deserializeVectors(const std::string &data);
    void updateOnlineGUMap(const std::string& key, int value);
};

#endif // LOGINCENTER_H
