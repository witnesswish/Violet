#ifndef SERVER_H
#define SERVER_H

#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdlib.h>
#include <sys/epoll.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <regex>
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <memory>
#include <openssl/x509.h>
#include <openssl/evp.h>

#include "protocol.h"
#include "unlogincenter.h"
#include "logincenter.h"
#include "redishelper.h"
#include "threadpoolcenter.h"
#include "filecenter.h"

/**
 * @brief The UserRecvBuffer class 当用户发送的消息过长，不能一次传完的时候，先把用户的消息存起来，读完了再进行转发
 */
struct  UserRecvBuffer
{
     int fd;
     ssize_t expectLen;
     ssize_t actuaLen;
     std::vector<char> recvBuffer;
};

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
    SRHelper sr;
    UnloginCenter unlogin;
    LoginCenter loginCenter;
    struct sockaddr_in serAddr;
    User u;
    RedisHelper redis;
    std::regex emailreg;
    std::regex namereg;
    bool running;
    ThreadPoolCenter pool;
    FileCenter file;
    PortPoolCenter ppl;
    SSL_CTX *ctx;
    static std::unordered_map<int, UserRecvBuffer> userRecvBuffMap;
    const std::string pemFinger = ("C2D69078B46A737D73D044BB493B9EA3734B2FD52278D25F82BF1C47704FB67D");

private:
    void init();
    int getRecvSize(int fd);
    void vread_cb(int fd, SSL *ssl);
    void vlogin(int fd, std::string username, std::string password, SSL *ssl);
    void vregister(int fd, std::string username, std::string password, std::string email, SSL *ssl);
    void vaddFriend(int fd, std::string reqName, std::string friName, SSL *ssl);
    void vaddGroup(int fd, std::string reqName, std::string groupName, SSL *ssl);
    void vcreateGroup(int fd, std::string reqName, std::string groupName, SSL *ssl);
    void vprivateChat(int fd, std::string reqName, std::string friName, std::string content, SSL *ssl);
    void vgroupChat(int fd, std::string reqName, std::string gname, std::string content, SSL *ssl);
    void vofflineHandle(int fd, SSL *ssl);
    void vuploadFile(int fd, std::string reqName, std::string friName, SSL *ssl);
    void vdownloadFile(int fd, std::string fileName, uint32_t fileSize, SSL *ssl, uint32_t chunk=0, uint32_t chunkSize=512000);
    void vsayWelcome(int fd);
    std::string calculateFingerprint(X509 *cert);
    int verifyClientPem(int isPreVerifyGood, X509_STORE_CTX *x509_ctx);
    static int staticVerifyClientPem(int isPreVerifyGood, X509_STORE_CTX *x509_ctx);
};

#endif // SERVER_H
