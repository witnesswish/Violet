#include "server.h"

Server::Server() 
{
    sock = 0;
    epfd = 0;
    serAddr.sin_port = htons(3434);
    serAddr.sin_addr.s_addr = INADDR_ANY;
}
Server::~Server(){}

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

void Server::sendMsg(int sock, uint16_t msgType, const std::string &content)
{
    Msg msg;
    msg.header.magic = htonl(0x43484154); // "CHAT"
    msg.header.version = htons(1);
    msg.header.type = htons(msgType);
    msg.header.length = htonl(content.size());
    msg.header.timestamp = htonl(static_cast<uint32_t>(time(nullptr)));
    msg.content.assign(content.begin(), content.end());
    auto packet = msg.serialize();
    if(send(sock, packet.data(), packet.size(), 0) < 0)
    {
        perror("send msg error");
    }
}

std::optional<Msg> Server::recvMsg(int sock)
{
    VioletProtHeader header;
    ssize_t len = recv(sock, &header, sizeof(header), MSG_PEEK);
    if(len != sizeof(header))
    {
        return std::nullopt;
    }
    uint32_t contentLen = ntohl(header.length);
    uint32_t totaLen = contentLen + sizeof(header);
    std::vector<char> recvBuffer(totaLen);
    len = recv(sock, recvBuffer.data(), recvBuffer.size(), 0);
    if(len != static_cast<ssize_t>(recvBuffer.size()))
    {
        return std::nullopt;
    }
    return Msg::deserialize(recvBuffer.data(), recvBuffer.size());
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
                welcome += client;
                welcome += ", enjoy yourself";
                msg.header.magic = htonl(0x43484154);    //"CHAT"，暂时用这个，后续可能的话做私有化
                msg.header.version = htons(1);
                msg.header.type = htons(0);
                msg.header.length = htons(welcome.size());
                msg.header.timestamp = htonl(static_cast<uint32_t>(time(nullptr)));
                msg.content.assign(welcome.begin(), welcome.end());
                auto packet = msg.serialize();
                int ret = send(client, packet.data(), BUFFER_SIZE, 0);
                if(ret < 0)
                {
                    perror("send welcome error");
                    closeServer();
                    exit(-1);
                }
            }
            else
            {
                //已经握过手，发送了欢迎消息
                char recv_buf[BUFFER_SIZE];
                bzero(recv_buf, BUFFER_SIZE);
                std::cout << "read from client(clientID = #" << fd << ")" << std::endl;
                int len = recv(fd, recv_buf, BUFFER_SIZE, 0);
                std::cout<< "read: " << len << " bytes, content: " << recv_buf <<std::endl;
                if(len == 0)
                {
                    close(fd);
                    std::cout<< "client #" << fd << " closed" <<std::endl;
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
