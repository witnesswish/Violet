#ifndef COMMON_H
#define COMMON_H

#include <asm-generic/fcntl.h>
#include <iostream>

#define EPOLL_SIZE 5000

struct Msg
{
    int magic;
    int type;
    int plateform;
    std::string content;
};

inline void addfd(int fd, int epfd, bool enable_et)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if(enable_et)
        ev.events |= EPOLLET;
    epoll_ctl( epfd, EPOLL_CTL_ADD, fd, &ev );
    fcntl( fd, F_SETFL, fcntl( fd, F_GETFL ) | O_NONBLOCK );
}

#endif // COMMON_H
