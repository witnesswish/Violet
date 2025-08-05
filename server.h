#ifndef SERVER_H
#define SERVER_H

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include <fcntl.h>

class Server
{
public:
    Server();
    ~Server();
    void start();

private:
    int sock;
    struct sockaddr_in serAddr;
    void init();
};

#endif // SERVER_H
