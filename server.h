#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/epoll.h>

#include "common.h"

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
    struct sockaddr_in serAddr;
    void init();
};

#endif // SERVER_H
