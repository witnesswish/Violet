#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <string.h>
#include <optional>
#include <errno.h>
#include <list>
#include <sys/ioctl.h>
#include <unistd.h>
#include <chrono>

#include "common.h"
#include "protocol.h"
#include "unlogincenter.h"
#include "logincenter.h"
#include "redishelper.h"

/**
 * @brief The Server class
 *
 */
class Server
{
public:
    Server();
    ~Server();
    void startServer();
    void closeServer();

private:
    int sock;
    int epfd;
    Msg msg;
    SRHelper sr;
    UnloginCenter unlogin;
    LoginCenter loginCenter;
    struct sockaddr_in serAddr;
    User u;
    RedisHelper redis;

private:
    void init();
    int getRecvSize(int fd);
    void vlogin(int fd, std::string username, std::string password);
    void vregister(int fd, std::string username, std::string password, std::string email);
    void vaddFriend(int fd, std::string reqName, std::string friName);
    void vaddGroup(int fd, std::string reqName, std::string groupName);
    void vcreateGroup(int fd, std::string reqName, std::string groupName);
    void vprivateChat(int fd, std::string reqName, std::string friName, std::string content);
    void vgroupChat(int fd, std::string reqName, std::string gname, std::string content);
    void vofflineHandle(int fd);
};

#endif // SERVER_H
