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

#include "common.h"
#include "protocol.h"

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
    struct sockaddr_in serAddr;
    void init();
    void sendMsg(int sock, uint16_t msgType, const std::string &content);
    std::optional<Msg> recvMsg(int sock);
};

#endif // SERVER_H
