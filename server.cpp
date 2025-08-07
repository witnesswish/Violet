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
                std::cout << "read from client(clientID = #" << fd << ")" << std::endl;
                auto ret = sr.recvMsg(fd);
                if(ret == std::nullopt)
                {
                    std::cout<< "sr return null opt" <<std::endl;
                }
                else
                {
                    ret->neck.mfrom = fd;
                    if(ret->neck.unlogin)
                    {
                        std::string command(ret->neck.command);
                        if(command == std::string("nonreq"))
                        {
                            VioletProtNeck neck = {};
                            strcpy(neck.command, "nonsucc");
                            unlogin.addNewUnlogin(fd);
                            std::string tmp = std::string("violet");
                            sr.sendMsg(fd, neck, tmp);
                        }
                        if(command == std::string("nong"))
                        {
                            std::cout<< "nong content to string: " << std::string(ret->content.begin(), ret->content.end()) <<std::endl;
                            unlogin.sendBordcast(fd, std::string(ret->content.begin(), ret->content.end()));
                        }
                        if(command == std::string("nonp"))
                        {
                            //tmp
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
