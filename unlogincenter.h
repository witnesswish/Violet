#ifndef UNLOGINCENTER_H
#define UNLOGINCENTER_H

#include <list>
#include <sys/socket.h>
#include <openssl/ssl.h>
#include <openssl/err.h>

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
    void sendBordcast(int, std::string, SSL *ssl);
    void addNewUnlogin(int fd, SSL *ssl);
    void removeUnlogin(int fd);
    void privateChate(int fd, uint8_t mto, std::string &, SSL *ssl);
};

#endif // UNLOGINCENTER_H
