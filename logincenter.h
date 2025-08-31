#ifndef LOGINCENTER_H
#define LOGINCENTER_H

#include <cstring>
#include <arpa/inet.h>
#include <string>
#include <map>
#include <set>
#include <sys/socket.h>
#include <list>
#include <openssl/ssl.h>
#include <openssl/err.h>

#include "protocol.h"
#include "redishelper.h"
#include "common.h"

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
    int vlogin(int fd, std::string username, std::string password, std::string &userInfo, SSL *ssl);
    int vaddFriend(std::string requestName, std::string firName, SSL *ssl);
    int vaddGroup(std::string requestName, std::string groupName, int fd, SSL *ssl);
    int vcreateGroup(std::string reqName, std::string groupName, int fd);
    int vprivateChat(std::string firName);
    void vgroupChat(int fd, std::string requestName, std::string groupName, std::string content, SSL *ssl);
    void vofflineHandle(int fd, SSL *ssl);
    void vhandleVbulre(int fd, std::string requestName, std::string friName, SSL *ssl);

private:
    SRHelper sr;
    RedisHelper redis;
    /**
     * @brief onlineGUMap 群组的在线用户，登录的时候将ConnectionInfo结构体加入进去，下线的时候删除结构体，结构体里有fd和SSL指针
     * @brief onlineUserFriend 用户的所有在线好友，存储SSL指针
     * 用户上线-》广播登录-》存入在线好友列表
     * 用户离线-》广播下线-》删除好友列表
     * 很烦，加密了之后不能直接用fd了，好多结构体要改，要改用SSL*来收发数据，我要思量一下，该怎么改
     * 在新建项目之前，要思量多一些，当然要很多经验
     */
    static std::map<std::string, std::vector<ConnectionInfo>> onlineGUMap;
    static std::map<int, std::list<ConnectionInfo>> onlineUserFriend;
    static std::map<int, std::string> onlineUser;
private:
    std::string serializeTwoVector(const std::vector<std::string> &vec1, const std::vector<std::string> &vec2);
    std::pair<std::vector<std::string>, std::vector<std::string>> deserializeVectors(const std::string &data);
    void updateOnlineGUMap(const std::string& key, int value);
};

#endif // LOGINCENTER_H
