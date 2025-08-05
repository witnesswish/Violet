#include "server.h"

Server::Server() 
{
    sock = 0;
    serAddr.sin_port = htons(3434);
    serAddr.sin_addr.s_addr = INADDR_ANY;
}
Server::~Server(){}

void Server::init()
{
    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock < 0)
    {
        perror("socket error");
        exit(-1);
    }
    int optval = 1;
    if(setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)))
    {
        perror("setsockopt error");
        exit(-1);
    }
    if(bind(sock, (const struct sockaddr*)&serAddr, sizeof(serAddr)) < 0)
    {
        perror("bind error");
        exit(-1);
    }
    if(listen(sock, 10) < 0)
    {
        perror("listen error");
        exit(-1);
    }
}

void Server::start()
{
    init();
   // fcntl(sock, ) 
    while(1)
    {
        
    }
}
