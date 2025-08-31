#include <chrono>
#include <optional>
#include <iostream>
#include <netinet/tcp.h>
#include <string.h>

#include <errno.h>

#include "server.h"
#include "common.h"

std::unordered_map<int, UserRecvBuffer> Server::userRecvBuffMap;

Server::Server()
{
    sock = 0;
    epfd = 0;
    memset(&serAddr, 0, sizeof(serAddr));
    serAddr.sin_family = AF_INET;
    serAddr.sin_port = htons(46836);
    serAddr.sin_addr.s_addr = INADDR_ANY;
    redis.connectRedis("127.0.0.1", 6379);
    emailreg = std::regex("(^[a-zA-Z0-9-_]+@[a-zA-Z0-9-_]+(\\.[a-zA-Z0-9-_]+)+)");
    //30个字节，最多10个中文
    namereg = std::regex("[a-zA-z0-9\u4e00-\u9fa5]{1,30}");
    running = true;
}
Server::~Server()
{
    closeServer();
    redis.disconnectRedis();
}
/**
 * @brief Server::getRecvSize 防止意外发生，内存有东西没有读全，做个检查
 * @param fd
 * @return
 */
int Server::getRecvSize(int fd)
{
    int bytes;
    if (ioctl(fd, FIONREAD, &bytes) < 0)
    {
        perror("read socket recv buffer error");
        return -1;
    }
    return bytes;
}

void Server::init()
{
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("socket error");
        exit(-1);
    }
    int optval = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval)))
    {
        perror("setsockopt error");
        exit(-1);
    }
    int enable_keepalive = 1;
    setsockopt(sock, SOL_SOCKET, SO_KEEPALIVE, &enable_keepalive, sizeof(enable_keepalive));
    int idle_time = 300;     //10秒无活动后开始发送探测包，测试阶段，后面可以改长一点
    int probe_interval = 10;     //5秒间隔
    int probe_count = 3;        //3次
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPIDLE, &idle_time, sizeof(idle_time));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPINTVL, &probe_interval, sizeof(probe_interval));
    setsockopt(sock, IPPROTO_TCP, TCP_KEEPCNT, &probe_count, sizeof(probe_count));
    if (bind(sock, (const struct sockaddr *)&serAddr, sizeof(serAddr)) < 0)
    {
        perror("bind error");
        exit(-1);
    }
    if (listen(sock, 2048) < 0)
    {
        perror("listen error");
        exit(-1);
    }
    epfd = epoll_create(EPOLL_SIZE);
    if (epfd < 0)
    {
        perror("epfd error");
        exit(-1);
    }
    addfd(sock, epfd);
    std::cout<< "on for lop ..." << sock << ": " << epfd <<std::endl;

    // 下面是ssl的配置
    SSL_library_init();                  // 载入所有SSL算法
    OpenSSL_add_all_algorithms();        // 载入所有加密算法
    SSL_load_error_strings();            // 载入所有SSL错误信息
    ERR_load_BIO_strings();
    ERR_load_crypto_strings();
    const SSL_METHOD *method = TLS_server_method();
    ctx = SSL_CTX_new(method);
    if (!ctx)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    if (SSL_CTX_use_certificate_file(ctx, "/home/ubuntu/Violet/cert.pem", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    if (SSL_CTX_use_PrivateKey_file(ctx, "/home/ubuntu/Violet/key.pem", SSL_FILETYPE_PEM) <= 0)
    {
        ERR_print_errors_fp(stderr);
        exit(EXIT_FAILURE);
    }
    if (!SSL_CTX_check_private_key(ctx))
    {
        fprintf(stderr, "Private key does not match the certificate public key\n");
        exit(EXIT_FAILURE);
    }
    //SSL_CTX_set_options(ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_COMPRESSION);    //只能使用较新的版本
    std::cout<< "end  of init function" <<std::endl;
}

void Server::vread_cb(int fd, SSL *ssl)
{
    std::cout << "read from client(clientID = #" << fd << ")" << std::endl;
    int bytesReady = getRecvSize(fd);
    while (bytesReady > 41 || bytesReady == 0)
    {
        std::optional<Msg> ret=Msg{};
        auto ixt = userRecvBuffMap.find(fd);
        if(ixt != userRecvBuffMap.end())
        {
            UserRecvBuffer &murb = ixt->second;
            if(murb.fd != fd)
            {
                std::cout<< "something error i don't know if this shown, just tag it, location 1" <<std::endl;
                continue;
            }
            else
            {
                ret = sr.recvMsg(fd, murb.expectLen - murb.actuaLen, ssl);
                if(ret != std::nullopt)
                {
                    murb.actuaLen += ret->header.checksum;
                    murb.recvBuffer.insert(murb.recvBuffer.end(), ret->content.begin(), ret->content.end());
                    if(ret->header.checksum == 0)
                    {
                        std::cout<< "read special byte, get 0, knicking user out" <<std::endl;
                        unlogin.removeUnlogin(fd);
                        vofflineHandle(fd, ssl);
                        bytesReady = 1;
                    }
                    if(murb.actuaLen < murb.expectLen)
                    {
                        continue;
                        bytesReady = 1;
                    }
                    else
                    {
                        //ret = Msg::deserialize(murb.recvBuffer.data(), murb.expectLen+sizeof(ret->header)+sizeof(ret->neck));
                        ret = Msg::deserialize(murb.recvBuffer.data(), murb.recvBuffer.size());
                        userRecvBuffMap.erase(fd);
                        bytesReady = 1;
                    }
                }
                else
                {
                    std::cout<< "read special byte error, contunie" <<std::endl;
                    bytesReady = 1;
                    continue;
                }
            }
        }
        else
        {
            ret = sr.recvMsg(fd, -1, ssl);
            std::cout<< "content length: " << ret->header.length << "--" << ret->header.checksum << "--" << ntohl(ret->header.length) <<std::endl;
        }
        if((ssize_t)ret->header.checksum < ntohl(ret->header.length))
        {
            if(!ret.has_value())
            {
                std::cout<< "ret empty" <<std::endl;
            }
            else
                std::cout<< "ret not empty" <<std::endl;
            UserRecvBuffer urbf;
            urbf.fd = fd;
            urbf.expectLen = ntohl(ret->header.length);
            urbf.actuaLen = ret->header.checksum;
            urbf.recvBuffer = ret.value().serialize();
            //memcpy(urbf.recvBuffer.data(), &(ret->header), sizeof(ret->header));
            //memcpy(urbf.recvBuffer.data()+sizeof(ret->header), &(ret->neck), sizeof(ret->neck));
            //urbf.recvBuffer.insert(urbf.recvBuffer.end(), ret->content.begin(), ret->content.end());
            userRecvBuffMap[fd] = urbf;
            std::cout<< "not recving all data, stash to violet recv cache, continue, total: " << ntohl(ret->header.length)
                     <<" recv: " << ret->header.checksum <<std::endl;
            bytesReady = 1;
            continue;
        }
        if (ret == std::nullopt)
        {
            std::cout << "sr return null opt" << std::endl;
        }
        else
        {
            if (ntohl(ret->header.length) == 1)
            {
                bytesReady = 1;
            }
            // 前面已经close过一次了，所以直接处理剩余的步骤，从list移除等行为
            else if (ret->header.length == 0)
            {
                //HMSET %s fd %s id %s stat %s
                unlogin.removeUnlogin(fd);
                vofflineHandle(fd, ssl);
                bytesReady = 1;
            }
            else
            {
                bytesReady = getRecvSize(fd);
                ret->neck.mfrom = fd;
                if (!ret->neck.unlogin)
                {
                    std::string command(ret->neck.command);
                    std::string username(ret->neck.name);
                    std::string password(ret->neck.pass);
                    std::string ccdemail(ret->neck.email);
                    std::string content;
                    content.reserve(ret->content.size()+10);  // 预分配内存
                    content.assign(ret->content.begin(), ret->content.end());
                    std::cout << command << "-" << username << "-" << password << "-" << content << std::endl;
                    if (command == "vreg")
                    {
                        vregister(fd, username, password, ccdemail, ssl);
                    }
                    if (command == "vlogin")
                    {
                        vlogin(fd, username, password, ssl);
                    }
                    if (command == "vbulre")
                    {
                        loginCenter.vhandleVbulre(fd, username, password, ssl);
                    }
                    if (command == "vaddf")
                    {
                        vaddFriend(fd, username, content, ssl);
                    }
                    if (command == "vaddg")
                    {
                        vaddGroup(fd, username, content, ssl);
                    }
                    if (command == "vcrtg")
                    {
                        vcreateGroup(fd, username, content, ssl);
                    }
                    if(command == "vpc")
                    {
                        vprivateChat(fd, username, password, content, ssl);
                    }
                    if(command == "vgc")
                    {
                        vgroupChat(fd, username, password, content, ssl);
                    }
                    if(command == "vtfs")
                    {
                        vuploadFile(fd, username, password, ssl);
                    }
                    if(command == "vtfr")
                    {
                        //recv file here
                    }
                }
                if (ret->neck.unlogin)
                {
                    std::string text = std::string(ret->content.begin(), ret->content.end());
                    std::string command(ret->neck.command);
                    std::cout << "command: " << command << std::endl;
                    if (command == std::string("nonreq"))
                    {
                        VioletProtNeck neck = {};
                        strcpy(neck.command, "nonsucc");
                        strcpy(neck.name, std::to_string(fd).c_str());
                        std::string tmp = std::string("violet");
                        sr.sendMsg(fd, neck, tmp, ssl);
                    }
                    if (command == std::string("nong"))
                    {
                        std::cout << "nong content to string: " << std::string(ret->content.begin(), ret->content.end()) << std::endl;
                        unlogin.sendBordcast(fd, std::string(ret->content.begin(), ret->content.end()), ssl);
                    }
                    if (command == std::string("nonig"))
                    {
                        unlogin.addNewUnlogin(fd, ssl);
                    }
                    if (command == std::string("nonqg"))
                    {
                        unlogin.removeUnlogin(fd);
                    }
                    if (command == std::string("nonp"))
                    {
                        unlogin.privateChate(fd, ret->neck.mto, text, ssl);
                    }
                }
            }
        }
    }
}

void Server::startServer()
{
    static struct epoll_event events[EPOLL_SIZE];
    init();
    while (running)
    {
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        //std::cout<< "on running ..." << epoll_events_count <<std::endl;
        if (epoll_events_count < 0)
        {
            perror("epoll event count error");
            break;
        }
        for (int i = 0; i < epoll_events_count; ++i)
        {
            int fd = events[i].data.fd;
            auto ptry = events[i].data.ptr;
            if(fd<0 || fd>100)
            {
                auto* ptrz = static_cast<std::shared_ptr<ConnectionInfo>*>(events[i].data.ptr);
                std::shared_ptr<ConnectionInfo> conn = *ptrz;
                std::cout<< "ptr value: fd: " << conn->fd <<std::endl;
            }
            else
            {
                std::cout<< "read nullptr from ptrz" <<std::endl;
            }
            SSL *fssl = nullptr;
            std::cout<< "on  server for lop ..." << fd <<std::endl;
            if (events[i].events & (EPOLLERR | EPOLLHUP))
            {
                auto* ptrz = static_cast<std::shared_ptr<ConnectionInfo>*>(events[i].data.ptr);
                std::shared_ptr<ConnectionInfo> conn = *ptrz;
                fssl = conn->ssl;
                if(fssl != nullptr)
                {
                    fd = conn->fd;
                }
                std::cout<< "错误或挂起，调用关闭程序" <<std::endl;
                unlogin.removeUnlogin(fd);
                vofflineHandle(fd, fssl);
                removefd(fd, epfd, fssl);
                close(fd);
                continue;
            }
            if (events[i].events & EPOLLRDHUP)
            {
                auto* ptrz = static_cast<std::shared_ptr<ConnectionInfo>*>(events[i].data.ptr);
                std::shared_ptr<ConnectionInfo> conn = *ptrz;
                fssl = conn->ssl;
                if(fssl != nullptr)
                {
                    fd = conn->fd;
                }
                std::cout<< "对端关闭连接，调用关闭程序" <<std::endl;
                unlogin.removeUnlogin(fd);
                vofflineHandle(fd, fssl);
                removefd(fd, epfd, fssl);
                close(fd);
                continue;
            }
            if (fd == sock)
            {
                std::cout<< "new client connecting..." <<std::endl;
                while(true)
                {
                    struct sockaddr_in clientAddr;
                    socklen_t clientAddrLength = sizeof(struct sockaddr_in);
                    int client = accept(sock, (struct sockaddr *)&clientAddr, &clientAddrLength);
                    if(client > 0)
                    {
                        std::cout << "client connection from: " << inet_ntoa(clientAddr.sin_addr) << ":" << ntohs(clientAddr.sin_port) << ", clientfd = #" << client << std::endl;
                        //pool.enqueue([this, client](){this->vsayWelcome(client);});
                        vsayWelcome(client);
                    }
                    else
                    {
                        if(errno == EAGAIN || errno == EWOULDBLOCK)
                        {
                            std::cout<< "read eagain or ewouldblock, break" <<std::endl;
                            break;
                        }
                        else
                        {
                            perror("something  happend unexpected");
                            break;
                        }
                    }
                }
            }
            else
            {
                auto* ptrz = static_cast<std::shared_ptr<ConnectionInfo>*>(events[i].data.ptr);
                std::shared_ptr<ConnectionInfo> conn = *ptrz;
                std::cout<< "ptr value: fd: " << conn->fd <<std::endl;
                fssl = conn->ssl;
                int ffd = conn->fd;
                if(fssl != nullptr)
                {
                    vread_cb(ffd, fssl);
                }
            }
        }
    }
    closeServer();
}
void Server::closeServer()
{
    running = false;
    if (epfd)
        close(epfd);
    if (sock)
        close(sock);
}

void Server::vregister(int fd, std::string username, std::string password, std::string email, SSL *ssl)
{
    if(!regex_match(email, emailreg))
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vregerr");
        std::string tmp("email type error");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    if(!regex_match(username, namereg))
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vregerr");
        std::string tmp("name type error");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    int ret = loginCenter.vregister(username, password, email, "salt");
    if (ret < 0)
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vregerr");
        std::string tmp("violet");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    if(ret == 1)
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vregerr");
        std::string tmp("username exists");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    if(ret == 2)
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vregerr");
        std::string tmp("email exists");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    if(ret == 0)
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vregsucc");
        std::string tmp("violet");
        sr.sendMsg(fd, neck, tmp, ssl);
    }
    VioletProtNeck neck = {};
    strcpy(neck.command, (const char *)"vregerr");
    std::string tmp("violet");
    sr.sendMsg(fd, neck, tmp, ssl);
}

void Server::vaddFriend(int fd, std::string reqName, std::string friName, SSL *ssl)
{
    if(!regex_match(friName, namereg))
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vaddferr");
        std::string tmp("type error");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    if(!regex_match(reqName, namereg))
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vaddferr");
        std::string tmp("type error");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    VioletProtNeck neck = {};
    int ret = loginCenter.vaddFriend(reqName, friName, ssl);
    if (ret == 0)
    {
        strcpy(neck.command, "vaddfsucc");
        memcpy(neck.name, friName.c_str(), sizeof(neck.name));
        sr.sendMsg(fd, neck, friName, ssl);
        return;
    }
    if(ret == 1)
    {
        std::cout<< "add friend error, function return: " << ret <<std::endl;
        strcpy(neck.command, "vaddferr");
        memcpy(neck.name, friName.c_str(), sizeof(neck.name));
        std::string tmp = std::string("friend you add is not exists: ") + friName;
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    std::cout<< "add friend error, function return: " << ret <<std::endl;
    strcpy(neck.command, "vaddferr");
    std::string tmp("violet");
    sr.sendMsg(fd, neck, tmp, ssl);
}

void Server::vaddGroup(int fd, std::string reqName, std::string groupName, SSL *ssl)
{
    if(!regex_match(groupName, namereg))
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vaddgerr");
        std::string tmp("type error");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    if(!regex_match(reqName, namereg))
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vaddgerr");
        std::string tmp("type error");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    VioletProtNeck neck = {};
    int ret = loginCenter.vaddGroup(reqName, groupName, fd, ssl);
    std::cout << "add g ret: " << ret << std::endl;
    if (ret == 0)
    {
        strcpy(neck.command, "vaddgsucc");
        memcpy(neck.name, groupName.c_str(), sizeof(neck.name));
        sr.sendMsg(fd, neck, groupName, ssl);
        return;
    }
    if(ret == 1)
    {
        strcpy(neck.command, "vaddgerr");
        memcpy(neck.name, groupName.c_str(), sizeof(neck.name));
        std::string tmp("group not exists");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    strcpy(neck.command, "vaddgerr");
    std::string tmp("violet");
    sr.sendMsg(fd, neck, tmp, ssl);
}

void Server::vcreateGroup(int fd, std::string reqName, std::string groupName, SSL *ssl)
{
    if(!regex_match(groupName, namereg))
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, "vcrtgerr");
        std::string tmp("type error");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    if(!regex_match(reqName, namereg))
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, "vcrtgerr");
        std::string tmp("type error");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    VioletProtNeck neck = {};
    int ret = loginCenter.vcreateGroup(reqName, groupName, fd);
    if (ret < 0)
    {
        strcpy(neck.command, "vcrtgerr");
        std::string tmp("violet");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    if (ret == 0)
    {
        strcpy(neck.command, "vcrtgsucc");
        sr.sendMsg(fd, neck, groupName, ssl);
        return;
    }
    if (ret == 1)
    {
        strcpy(neck.command, "vcrtgerr");
        std::string tmp("exists");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
}

void Server::vprivateChat(int fd, std::string reqName, std::string friName, std::string content, SSL *ssl)
{
    std::cout<< "vpc params: " << reqName << "-" << friName << "-" << content <<std::endl;
    VioletProtNeck neck = {};
    int friId = loginCenter.vprivateChat(friName);
    std::cout<< "login return fid: " << friId <<std::endl;
    if(friId < 0)
    {
        strcpy(neck.command, "vpcerr");
        std::string tmp("user not online, message will be sent after user online");
        sr.sendMsg(fd, neck, tmp, ssl);

        // 下面是做缓存，缓冲用redis的sorted set来做，zadd 用户名 时间戳 拼接后的消息
        // 拼接消息的格式：发送者|消息内容
        // 发送的时候按次取出消息，做解拼接处理，一次取20条，取完之后，zcard key做判断还有没有数据，循环
        auto c = std::chrono::system_clock::now();
        std::time_t t = std::chrono::system_clock::to_time_t(c);
        std::string tmptime = std::to_string(t);
        const std::string tmpstr = reqName + "|" + content;
        std::string tmpname = friName + "msgcache";
        auto ret = redis.execute("ZADD %s %s %s", tmpname.c_str(), tmptime.c_str(), tmpstr.c_str());
        if(ret == std::nullopt)
        {
            std::cout<< "redis execute error on prichat offline" <<std::endl;
        }
        return;
    }
    strcpy(neck.command, "vpcb");
    memcpy(neck.name, reqName.c_str(), sizeof(reqName));
    {
        std::lock_guard<std::mutex> lock(fdSslMapMutex);
        auto iterator = fdSslMap.find(friId);
        if (iterator != fdSslMap.end())
        {
            sr.sendMsg(friId, neck, content, iterator->second);
        }
    }
}

void Server::vgroupChat(int fd, std::string reqName, std::string gname, std::string content, SSL *ssl)
{
    loginCenter.vgroupChat(fd, reqName, gname, content, ssl);
}

void Server::vofflineHandle(int fd, SSL *ssl)
{
    loginCenter.vofflineHandle(fd, ssl);
}

void Server::vuploadFile(int fd, std::string reqName, std::string friName, SSL *ssl)
{
    VioletProtNeck neck = {};
    int tmpPort = ppl.getPort();
    if(tmpPort < 0)
    {
        strcpy(neck.command, "vtfserr");
        memcpy(neck.name, friName.c_str(), sizeof(neck.name));
        memcpy(neck.pass, friName.c_str(), sizeof(neck.pass));
        std::string tmp("file system not avaliable right now, try it later");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    pool.enqueue([this, tmpPort](){return this->file.vuploadFile(tmpPort);});
    strcpy(neck.command, "vtfspot");
    std::string tmp(std::to_string(tmpPort));
    sr.sendMsg(fd, neck, tmp, ssl);
    // 这部分要到那边去发送，不关线程失败还是成功。
    // if(ret.get() < 0)
    // {
    //     strcpy(neck.command, "vtfserr");
    //     std::string tmp("file system not avaliable right now, try it later");
    //     sr.sendMsg(fd, neck, tmp, ssl);
    //     return;
    // }
    // else
    // {
    //     // 这里发消息给接收方，提醒他好友发文件，并且给他一个端口
    //     strcpy(neck.command, "vtfspot");
    //     std::string tmp(std::to_string(tmpPort));
    //     sr.sendMsg(fd, neck, tmp, ssl);
    //     return;
    // }
}

void Server::vsayWelcome(int fd)
{
    std::cout<< "into say hello function" <<std::endl;
    // 当连接到来，先进行ssl握手，再sayhello，如果握手不成功，直接关闭
    SSL *sslWelcome = SSL_new(ctx);
    if (!sslWelcome)
    {
        ERR_print_errors_fp(stderr);
        close(fd); // 绑定失败的话关闭TCP连接，先关闭，如果要协商，后续再继续添加
        return;
    }

    // 将SSL对象与fd关联起来
    if (SSL_set_fd(sslWelcome, fd) != 1)
    {
        ERR_print_errors_fp(stderr);
        SSL_free(sslWelcome);
        close(fd);  //关联失败，关闭tcp，先关闭
        return;
    }
    // 在TCP连接之上进行SSL握手
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
    int acceptRet = SSL_accept(sslWelcome);
    constexpr long long INTERVAL_MS = 5000;
    auto last_time = std::chrono::steady_clock::now();
    while(acceptRet <= 0)
    {
        char tmp[1024];
        int err = SSL_get_error(sslWelcome, acceptRet);
        fprintf(stderr, "SSL_accept failed with error %d\n", err);
        // 如果ssl握手过程中，非阻塞会立即返回，要做判断
        if(err == SSL_ERROR_WANT_READ)
        {
            //std::cout<< "more things to read, trying..." <<std::endl;
            acceptRet = SSL_accept(sslWelcome);
        }
        else if(err ==  SSL_ERROR_WANT_WRITE)
        {
            //std::cout<< "more things to write, trying..." <<std::endl;
            acceptRet = SSL_accept(sslWelcome);
        }
        else
        {
            ERR_print_errors_fp(stderr);
            SSL_shutdown(sslWelcome);
            SSL_free(sslWelcome);
            std::cout<< "if you read this, it mens ssl con error happened, close fd #" << fd <<std::endl;
            close(fd);
            return;
        }
        auto now = std::chrono::steady_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(now - last_time).count() >= INTERVAL_MS) 
        {
            ERR_print_errors_fp(stderr);
            SSL_shutdown(sslWelcome);
            SSL_free(sslWelcome);
            close(fd);
            std::cout<< "if you read this, it mens ssl con not compelete in 5s, close fd #" << fd <<std::endl;
            last_time = now; // 更新时间
            return;
        }
    }
    printf("if you read this, it means SSL connection established with client. Using cipher: %s\n", SSL_get_cipher(sslWelcome));
    auto connInfo = std::make_shared<ConnectionInfo>();
    connInfo->fd = fd;
    connInfo->ssl = sslWelcome;
    // ssl握手已经完成，下面sayhello
    struct sockaddr_in clientAddr;
    std::cout<< "re confirm fd is #" << fd <<std::endl;
    addfd(fd, epfd, connInfo);
    // clients_list.push_back(clientfd);一些操作
    std::string welcome = "welcome, your id is #";
    welcome += std::to_string(fd);
    welcome += ", enjoy yourself";
    std::cout << "welcome: " << welcome << std::endl;
    sr.sendMsg(fd, 0, welcome, sslWelcome);
}

void Server::vlogin(int fd, std::string username, std::string password, SSL *ssl)
{
    //只需要拦截用户名
    if(!regex_match(username, namereg))
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, "vloginerr");
        std::string tmp("type error");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    memset(&u, 0, sizeof(u));
    std::string userinfo;
    int ret = loginCenter.vlogin(fd, username, password, userinfo, ssl);
    if (ret < 0)
    {
        std::cout<< "login failure, function return: " << ret <<std::endl;
        VioletProtNeck neck = {};
        strcpy(neck.command, "vloginerr");
        std::string tmp("violet");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    if(ret == 1)
    {
        std::cout<< "login failure, function return: " << ret <<std::endl;
        VioletProtNeck neck = {};
        strcpy(neck.command, "vloginerr");
        std::string tmp("incorrect");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    if (ret == 2)
    {
        std::cout<< "login failure, function return: " << ret <<std::endl;
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vloginerr");
        std::string tmp("incorrect");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    if(ret == 3)
    {
        std::cout<< "login failure, function return: " << ret <<std::endl;
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char *)"vloginerr");
        std::string tmp("incorrect");
        sr.sendMsg(fd, neck, tmp, ssl);
        return;
    }
    // for (const auto &it : u.friends)
    // {
    //     std::cout << it << std::endl;
    // }
    // for (const auto &it : u.groups)
    // {
    //     std::cout << it << std::endl;
    // }
    VioletProtNeck neck = {};
    strcpy(neck.command, (const char *)"vloginsucc");
    memcpy(neck.name, username.c_str(), sizeof(neck.name));
    sr.sendMsg(fd, neck, userinfo, ssl);
    std::cout<< "login success, user: " << username <<std::endl;

    //get cache and send
    std::string tmpnum;
    std::string tmpname = username + "msgcache";
    auto ret1 = redis.execute("ZCARD %s", tmpname.c_str());
    if(ret1 == std::nullopt)
    {
        std::cout<< "redis execute error on login get offline msg" <<std::endl;
    }
    else
    {
        //optional<vector<string>>
        for(auto &it : ret1.value())
        {
            std::cout<< "read num of msgcache on login: " << it <<std::endl;
            tmpnum = it;
        }
    }
    auto tmpx = std::stoi(tmpnum);
    std::cout<< "tmpx: " << tmpx <<std::endl;
    while(tmpx > 0)
    {
        if(tmpx-20 >= 0)
        {
            //auto ret2 = redis.execute("ZRANG %s 0 19 WITHSCORES", username.c_str()); //if you need timestamp, with option withscores
            auto ret2 = redis.execute("ZRANGE %s 0 19", tmpname.c_str());
            if(ret2 == std::nullopt)
            {
                std::cout<< "redis execute error on login get offline msg" <<std::endl;
            }
            else
            {
                //optional<vector<string>>
                for(auto &it : ret2.value())
                {
                    auto pos = it.find('|');
                    if(pos > 0 && pos < it.size())
                    {
                        std::string from = it.substr(0, pos);
                        std::string content = it.substr(pos + 1);
                        VioletProtNeck neck = {};
                        strcpy(neck.command, "vpcache");
                        memcpy(neck.name, from.c_str(), sizeof(neck.name));
                        //std::cout << "ser recv: " << userinfo << std::endl;
                        sr.sendMsg(fd, neck, content, ssl);
                    }
                }
            }
            auto ret = redis.execute("ZREMRANGEBYRANK %s 0 19", tmpname.c_str());
            if(ret == std::nullopt)
            {
                std::cout<< "redis execute error on login at del 20" <<std::endl;
            }
            tmpx = tmpx - 20;
        }
        else if(tmpx-10 >= 0)
        {
            auto ret2 = redis.execute("ZRANGE %s 0 9", tmpname.c_str());
            if(ret2 == std::nullopt)
            {
                std::cout<< "redis execute error on login get offline msg" <<std::endl;
            }
            else
            {
                //optional<vector<string>>
                for(auto &it : ret2.value())
                {
                    auto pos = it.find('|');
                    if(pos > 0 && pos < it.size())
                    {
                        std::string from = it.substr(0, pos);
                        std::string content = it.substr(pos+1);
                        VioletProtNeck neck = {};
                        strcpy(neck.command, "vpcache");
                        memcpy(neck.name, from.c_str(), sizeof(neck.name));
                        //std::cout << "ser recv: " << userinfo << std::endl;
                        sr.sendMsg(fd, neck, content, ssl);
                    }
                }
            }
            auto ret = redis.execute("ZREMRANGEBYRANK %s 0 9", tmpname.c_str());
            if(ret == std::nullopt)
            {
                std::cout<< "redis execute error on login  at del 10" <<std::endl;
            }
            tmpx = tmpx - 10;
        }
        else if(tmpx-5 >= 0)
        {
            auto ret2 = redis.execute("ZRANGE %s 0 4", tmpname.c_str());
            if(ret2 == std::nullopt)
            {
                std::cout<< "redis execute error on login get offline msg" <<std::endl;
            }
            else
            {
                //optional<vector<string>>
                for(auto &it : ret2.value())
                {
                    auto pos = it.find('|');
                    if(pos > 0 && pos < it.size())
                    {
                        std::string from = it.substr(0, pos);
                        std::string content = it.substr(pos+1);
                        VioletProtNeck neck = {};
                        strcpy(neck.command, "vpcache");
                        memcpy(neck.name, from.c_str(), sizeof(neck.name));
                        //std::cout << "ser recv: " << userinfo << std::endl;
                        sr.sendMsg(fd, neck, content, ssl);
                    }
                }
            }
            auto ret = redis.execute("ZREMRANGEBYRANK %s 0 4", tmpname.c_str());
            if(ret == std::nullopt)
            {
                std::cout<< "redis execute error on login  at del 5" <<std::endl;
            }
            tmpx = tmpx - 5;
        }
        else
        {
            auto ret2 = redis.execute("ZRANGE %s 0 0", tmpname.c_str());
            if(ret2 == std::nullopt)
            {
                std::cout<< "redis execute error on login get offline msg" <<std::endl;
            }
            else
            {
                //optional<vector<string>>
                for(auto &it : ret2.value())
                {
                    auto pos = it.find('|');
                    if(pos > 0 && pos < it.size())
                    {
                        std::string from = it.substr(0, pos);
                        std::string content = it.substr(pos+1);
                        VioletProtNeck neck = {};
                        strcpy(neck.command, "vpcache");
                        memcpy(neck.name, from.c_str(), sizeof(neck.name));
                        //std::cout << "ser recv: " << userinfo << std::endl;
                        sr.sendMsg(fd, neck, content, ssl);
                    }
                }
            }
            auto ret = redis.execute("ZREMRANGEBYRANK %s 0 0", tmpname.c_str());
            if(ret == std::nullopt)
            {
                std::cout<< "redis execute error on login  at del 1" <<std::endl;
            }
            tmpx = tmpx - 1;
        }
    }
    auto ret2 = redis.execute("DEL %s", tmpname.c_str());
    if(ret2 == std::nullopt)
    {
        std::cout<< "redis execute error on login at endline" <<std::endl;
    }
}
