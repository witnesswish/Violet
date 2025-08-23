#ifndef COMMON_H
#define COMMON_H

#include <iostream>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>

#define SERVER_WELCOME "Welcome you join to the chat room! Your chat ID is: Client #%d"
#define BUFFER_SIZE 0xFFFF
#define EPOLL_SIZE 5000

inline void addfd(int fd, int epfd, bool enable_et = true)
{
    struct epoll_event ev;
    ev.data.fd = fd;
    ev.events = EPOLLIN;
    if (enable_et)
        ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    std::cout << "fd #" << fd << " added to epoll" << std::endl;
}
inline void removefd(int fd, int epfd)
{
    epoll_ctl(epfd, EPOLL_CTL_DEL, fd, NULL);
}

#endif // COMMON_H
