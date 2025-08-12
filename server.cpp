#include "server.h"

Server::Server() 
{
    sock = 0;
    epfd = 0;
    serAddr.sin_port = htons(3434);
    serAddr.sin_addr.s_addr = INADDR_ANY;
}
Server::~Server(){
    closeServer();
}
int Server::getRecvSize(int fd)
{
    int bytes;
    if(ioctl(fd, FIONREAD, &bytes) < 0)
    {
        perror("read socket recv buffer error");
        return -1;
    }
    return bytes;
}

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
    epfd = epoll_create (EPOLL_SIZE);
    if(epfd < 0)
    {
        perror("epfd error");
        exit(-1);
    }
    addfd(sock, epfd);
}

void Server::startServer()
{
    static struct epoll_event events[EPOLL_SIZE];
    init();
    while(1)
    {
        int epoll_events_count = epoll_wait(epfd, events, EPOLL_SIZE, -1);
        if(epoll_events_count < 0)
        {
            perror("epoll event count error");
            break;
        }
        for(int i = 0; i < epoll_events_count; ++i)
        {
            int fd = events[i].data.fd;
            if(fd == sock)
            {
                struct sockaddr_in clientAddr;
                socklen_t clientAddrLength = sizeof(struct sockaddr_in);
                int client = accept( sock, ( struct sockaddr* )&clientAddr, &clientAddrLength );
                std::cout << "client connection from: "
                    << inet_ntoa(clientAddr.sin_addr) << ":"
                    << ntohs(clientAddr.sin_port) << ", clientfd = #"
                    << client << std::endl;
                addfd(client, epfd);
                //clients_list.push_back(clientfd);一些操作
                std::string welcome = "welcome, your id is #";
                welcome += std::to_string(client);
                welcome += ", enjoy yourself";
                std::cout<< "welcome: " << welcome <<std::endl;
                sr.sendMsg(client, 0, welcome);
            }
            else
            {
                int bytesReady = getRecvSize(fd);
                while(bytesReady > 41 || bytesReady == 0)
                {
                    std::cout << "read from client(clientID = #" << fd << ")" << std::endl;
                    auto ret = sr.recvMsg(fd);
                    if(ret == std::nullopt)
                    {
                        std::cout<< "sr return null opt" <<std::endl;
                    }
                    else
                    {
                        if(ret->header.length == 1)
                        {
                            bytesReady = 1;
                        }
                        // 这里取值判断是因为要对fd进行一系列处理，这样虽然有点麻烦，但是后面看看机会再修改一下
                        // 前面已经close过一次了，所以直接处理剩余的步骤，从list移除等行为
                        else if(ret->header.length == 0)
                        {
                            unlogin.removeUnlogin(fd);
                            bytesReady = 1;
                        }
                        else
                        {
                            bytesReady = getRecvSize(fd);
                            ret->neck.mfrom = fd;
                            if(!ret->neck.unlogin)
                            {
                                std::string command(ret->neck.command);
                                std::string username(ret->neck.username);
                                std::string password(ret->neck.password);
                                std::string ccdemail(ret->neck.email);
                                if(command == "vreg")
                                {
                                    vregister(fd, username, password, ccdemail);
                                }
                            }
                            if(ret->neck.unlogin)
                            {
                                std::string text = std::string(ret->content.begin(), ret->content.end());
                                std::string command(ret->neck.command);
                                std::cout<<  "command: " << command <<std::endl;
                                if(command == std::string("nonreq"))
                                {
                                    VioletProtNeck neck = {};
                                    strcpy(neck.command, "nonsucc");
                                    strcpy(neck.username, std::to_string(fd).c_str());
                                    std::string tmp = std::string("violet");
                                    sr.sendMsg(fd, neck, tmp);
                                }
                                if(command == std::string("nong"))
                                {
                                    std::cout<< "nong content to string: " << std::string(ret->content.begin(), ret->content.end()) <<std::endl;
                                    unlogin.sendBordcast(fd, std::string(ret->content.begin(), ret->content.end()));
                                }
                                if(command == std::string("nonig"))
                                {
                                    unlogin.addNewUnlogin(fd);
                                }
                                if(command == std::string("nonqg"))
                                {
                                    unlogin.removeUnlogin(fd);
                                }
                                if(command == std::string("nonp"))
                                {
                                    unlogin.privateChate(fd, ret->neck.mto, text);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    closeServer();
}
void Server::closeServer()
{
    if(epfd)
        close(epfd);
    if(sock)
        close(sock);
}

void Server::vregister(int fd, std::string username, std::string password, std::string email) {
    int ret = loginCenter.vregister(username, password, email, "salt");
    if(ret < 0)
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char*)"vregerr");
        std::string tmp("violet");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    VioletProtNeck neck = {};
    strcpy(neck.command, (const char*)"vregsucc");
    std::string tmp("violet");
    sr.sendMsg(fd, neck, tmp);
}

void Server::vlogin(int fd, std::string username, std::string password) {
    int ret = loginCenter.vregister(username, password, "vu1@elves.asia", "salt");
    if(ret < 0)
    {
        VioletProtNeck neck = {};
        strcpy(neck.command, (const char*)"vregerr");
        std::string tmp("violet");
        sr.sendMsg(fd, neck, tmp);
        return;
    }
    VioletProtNeck neck = {};
    strcpy(neck.command, (const char*)"vregsucc");
    std::string tmp("violet");
    sr.sendMsg(fd, neck, tmp);
}
