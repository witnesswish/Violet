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

#include "common.h"
#include "protocol.h"
#include "unlogincenter.h"
#include "mariadbhelper.h"

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
    struct sockaddr_in serAddr;
    MariadbHelper mariadb("violet", "violet@1admin", "violet", "127.0.0.1", 3306, false);
private:
    void init();
    int getRecvSize(int fd);
};

#endif // SERVER_H
