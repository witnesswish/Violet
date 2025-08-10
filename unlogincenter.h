#ifndef UNLOGINCENTER_H
#define UNLOGINCENTER_H

#include <iostream>
#include <list>
#include <sys/socket.h>

#include "protocol.h"

class UnloginCenter
{
public:
    UnloginCenter();
private:
    int sock;
    SRHelper sr;
    static std::list<int> onlineUnlogin;
public:
    void sendBordcast(int, std::string);
    void addNewUnlogin(int fd);
    void removeUnlogin(int fd);
    void privateChate(int fd, uint8_t mto, std::string &);
};

#endif // UNLOGINCENTER_H
