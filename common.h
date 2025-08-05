#ifndef COMMON_H
#define COMMON_H

#include <asm-generic/fcntl.h>
#include <iostream>

#define SERVER_WELCOME "Welcome you join to the chat room! Your chat ID is: Client #%d"
#define BUFFER_SIZE 0xFFFF
#define EPOLL_SIZE 5000

struct Msg
{
    int magic;
    int type;
    int plateform;
    std::string content;
};

inline void addfd(int fd, int epfd, bool enable_et=true)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if(enable_et)
        ev.events = EPOLLIN | EPOLLET;
    epoll_ctl( epfd, EPOLL_CTL_ADD, fd, &ev );
    fcntl( fd, F_SETFL, fcntl( fd, F_GETFL, 0 ) | O_NONBLOCK );
    std::cout<< "fd #" << fd << " added to epoll" <<std::endl;
}

#endif // COMMON_H
