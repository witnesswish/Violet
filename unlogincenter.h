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
    std::list<int> onlineUnlogin;
public:
    void sendBordcast(int, std::string);
    void addNewUnlogin(int fd);
    void removeUnlogin(int fd);
};

#endif // UNLOGINCENTER_H
